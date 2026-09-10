#include <gtest/gtest.h>

#include <space_rhythm/core/timeline.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace core = space_rhythm::core;

core::RhythmEvent event(std::string id, core::TimeNs time_ns, bool locked = false)
{
    core::RhythmEvent value;
    value.id = core::EventId{std::move(id)};
    value.track_id = core::TrackId{"track-main"};
    value.time_ns = time_ns;
    value.kind = core::EventKind::manual;
    value.source.origin = core::EventOrigin::user;
    value.source.producer_id = "T-021-public-contract";
    value.source.producer_version = "1";
    value.strength_ppm = 500'000;
    value.locked = locked;
    value.user_edited = true;
    return value;
}

core::TransactionRequest transaction(std::string id,
                                     core::TimelineRevision revision,
                                     std::vector<core::Operation> operations,
                                     core::TransactionOrigin origin =
                                         core::TransactionOrigin::user)
{
    return {core::TransactionId{std::move(id)},
            revision,
            origin,
            std::move(operations),
            std::nullopt};
}

TEST(T021CoreTimeContract, UsesPublishedLiteralOraclesForRoundingAndBoundaries)
{
    struct Oracle {
        const char* vector_id;
        std::int64_t ticks;
        core::TimeBase base;
        core::RoundingMode mode;
        core::TimeNs expected;
    };

    // These are literal A-012 0.1 contract values.  The test deliberately does
    // not contain a second implementation of the scaling or rounding algorithm.
    constexpr std::array oracles{
        Oracle{"TV-TIME-002", 30, {1, 30}, core::RoundingMode::nearest_ties_to_even,
               1'000'000'000},
        Oracle{"TV-TIME-004", 1, {1001, 30000},
               core::RoundingMode::nearest_ties_to_even, 33'366'667},
        Oracle{"TV-TIME-005-floor", 1, {1, 2'000'000'000},
               core::RoundingMode::floor, 0},
        Oracle{"TV-TIME-006-ties-even", 3, {1, 2'000'000'000},
               core::RoundingMode::nearest_ties_to_even, 2},
        Oracle{"TV-TIME-007-floor", -1, {1, 2'000'000'000},
               core::RoundingMode::floor, -1},
        Oracle{"TV-TIME-008-ties-even", -3, {1, 2'000'000'000},
               core::RoundingMode::nearest_ties_to_even, -2},
    };

    for (const auto& oracle : oracles) {
        SCOPED_TRACE(oracle.vector_id);
        const auto actual = core::scale_ticks(oracle.ticks, oracle.base, oracle.mode);
        ASSERT_TRUE(actual);
        EXPECT_EQ(actual.value(), oracle.expected);
    }

    const auto invalid_base = core::scale_ticks(
        1, {1, 0}, core::RoundingMode::nearest_ties_to_even);
    ASSERT_FALSE(invalid_base);
    EXPECT_EQ(invalid_base.error().code, core::ErrorCode::invalid_time_base)
        << "TV-TIME-009";

    const auto add_overflow =
        core::checked_add(std::numeric_limits<core::TimeNs>::max(), 1);
    const auto subtract_overflow =
        core::checked_subtract(std::numeric_limits<core::TimeNs>::min(), 1);
    ASSERT_FALSE(add_overflow);
    ASSERT_FALSE(subtract_overflow);
    EXPECT_EQ(add_overflow.error().code, core::ErrorCode::time_overflow)
        << "TV-TIME-010 add";
    EXPECT_EQ(subtract_overflow.error().code, core::ErrorCode::time_overflow)
        << "TV-TIME-010 subtract";
}

TEST(T021CoreTransactionContract, LocksAtomicityIdempotencyAndRevisionsAreObservable)
{
    core::Timeline timeline{core::ProjectId{"t021-project"}};
    const auto setup = timeline.submit(transaction(
        "setup",
        0,
        {core::AddTrack{{core::TrackId{"track-main"}, 0, std::string{"Main"}, {}}},
         core::AddEvent{event("locked-event", 100, true)}}));
    ASSERT_EQ(setup.status, core::TransactionStatus::committed);
    ASSERT_EQ(timeline.snapshot()->timeline_revision, 1U);

    const auto before_locked_attempt = timeline.snapshot();
    const auto locked_attempt = timeline.submit(transaction(
        "analysis-move-locked",
        1,
        {core::MoveEvent{core::EventId{"locked-event"}, 200, std::nullopt}},
        core::TransactionOrigin::analysis));
    ASSERT_EQ(locked_attempt.status, core::TransactionStatus::rejected);
    ASSERT_TRUE(locked_attempt.error);
    EXPECT_EQ(locked_attempt.error->code, core::ErrorCode::locked_event)
        << "TV-TXN-011";
    EXPECT_EQ(*timeline.snapshot(), *before_locked_attempt);

    const auto unlock_and_move = transaction(
        "user-unlock-and-move",
        1,
        {core::SetEventLocked{core::EventId{"locked-event"}, false},
         core::MoveEvent{core::EventId{"locked-event"}, 200, std::nullopt}});
    const auto committed = timeline.submit(unlock_and_move);
    ASSERT_EQ(committed.status, core::TransactionStatus::committed);
    ASSERT_EQ(timeline.snapshot()->timeline_revision, 2U);
    ASSERT_EQ(timeline.snapshot()->events.size(), 1U);
    EXPECT_FALSE(timeline.snapshot()->events.front().locked);
    EXPECT_EQ(timeline.snapshot()->events.front().time_ns, 200)
        << "TV-TXN-012";

    const auto replay = timeline.submit(unlock_and_move);
    EXPECT_EQ(replay, committed) << "TV-TXN-008";
    EXPECT_EQ(timeline.snapshot()->timeline_revision, 2U);

    const auto stable = timeline.snapshot();
    const auto stale = timeline.submit(transaction(
        "stale",
        1,
        {core::AddEvent{event("must-not-appear", 300)}}));
    ASSERT_EQ(stale.status, core::TransactionStatus::rejected);
    ASSERT_TRUE(stale.error);
    EXPECT_EQ(stale.error->code, core::ErrorCode::stale_revision)
        << "TV-TXN-002";
    EXPECT_EQ(*timeline.snapshot(), *stable);

    const auto atomic_failure = timeline.submit(transaction(
        "atomic-failure",
        2,
        {core::AddEvent{event("partial-add", 400)},
         core::MoveEvent{core::EventId{"missing"}, 500, std::nullopt}}));
    ASSERT_EQ(atomic_failure.status, core::TransactionStatus::rejected);
    ASSERT_TRUE(atomic_failure.error);
    EXPECT_EQ(atomic_failure.error->code, core::ErrorCode::event_not_found)
        << "TV-TXN-004";
    EXPECT_EQ(*timeline.snapshot(), *stable);
}

TEST(T021CoreRevisionContract, RefusesSemanticCommitAfterRevisionExhaustion)
{
    core::TimelineSnapshot loaded;
    loaded.project_id = core::ProjectId{"exhausted-project"};
    loaded.timeline_revision = std::numeric_limits<core::TimelineRevision>::max();
    loaded.tracks.push_back(
        {core::TrackId{"track-main"}, 0, std::string{"Main"}, {}});
    core::Timeline timeline{std::move(loaded)};

    const auto result = timeline.submit(transaction(
        "must-not-wrap",
        std::numeric_limits<core::TimelineRevision>::max(),
        {core::AddEvent{event("overflow-event", 1)}}));
    ASSERT_EQ(result.status, core::TransactionStatus::rejected);
    ASSERT_TRUE(result.error);
    EXPECT_EQ(result.error->code, core::ErrorCode::revision_exhausted)
        << "TV-TXN-010";
    EXPECT_EQ(timeline.snapshot()->timeline_revision,
              std::numeric_limits<core::TimelineRevision>::max());
    EXPECT_TRUE(timeline.snapshot()->events.empty());
}

} // namespace
