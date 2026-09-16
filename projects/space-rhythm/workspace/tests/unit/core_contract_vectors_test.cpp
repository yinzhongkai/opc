#include <gtest/gtest.h>

#include <space_rhythm/core/timeline.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace core = space_rhythm::core;

namespace {

#define A012_VECTOR(id) SCOPED_TRACE(id)

core::EventSource user_source()
{
    core::EventSource source;
    source.origin = core::EventOrigin::user;
    source.producer_id = "user";
    source.producer_version = "1";
    return source;
}

core::EventSource analysis_source(std::string revision = "analysis-a")
{
    core::EventSource source;
    source.origin = core::EventOrigin::analysis;
    source.producer_id = "test-analyzer";
    source.producer_version = "1.0";
    source.input_fingerprint = "input-sha256";
    source.parameters_digest = "params-sha256";
    source.analysis_revision = core::AnalysisRevision{std::move(revision)};
    return source;
}

core::RhythmEvent manual_event(std::string id = "manual-1",
                               core::TimeNs time_ns = 100,
                               std::string track_id = "track-0")
{
    return core::RhythmEvent{core::EventId{std::move(id)},
                             core::TrackId{std::move(track_id)},
                             time_ns,
                             0,
                             core::EventKind::manual,
                             user_source(),
                             500'000,
                             std::nullopt,
                             false,
                             true,
                             std::nullopt,
                             {}};
}

core::RhythmEvent analysis_event(std::string id,
                                 core::TimeNs time_ns,
                                 bool locked = false,
                                 std::string revision = "analysis-old")
{
    return core::RhythmEvent{core::EventId{std::move(id)},
                             core::TrackId{"track-0"},
                             time_ns,
                             0,
                             core::EventKind::beat,
                             analysis_source(std::move(revision)),
                             600'000,
                             700'000,
                             locked,
                             false,
                             std::nullopt,
                             {}};
}

core::AnalysisCandidate candidate(std::string id,
                                  core::TimeNs time_ns,
                                  std::string revision = "analysis-a",
                                  core::NormPpm strength = 600'000,
                                  core::NormPpm confidence = 700'000)
{
    auto source = analysis_source(revision);
    source.candidate_ids = {core::CandidateId{id}};
    return core::AnalysisCandidate{core::CandidateId{std::move(id)},
                                   core::TrackId{"track-0"},
                                   time_ns,
                                   0,
                                   core::EventKind::beat,
                                   std::move(source),
                                   strength,
                                   confidence,
                                   core::VersionedOpaqueObject{
                                       "test.candidate", 1, {}, {{"feature", "1"}}},
                                   {}};
}

core::TransactionRequest request(std::string id,
                                 core::TimelineRevision revision,
                                 std::vector<core::Operation> operations,
                                 core::TransactionOrigin origin =
                                     core::TransactionOrigin::user,
                                 std::optional<std::string> coalescing_key = std::nullopt)
{
    return core::TransactionRequest{core::TransactionId{std::move(id)},
                                    revision,
                                    origin,
                                    std::move(operations),
                                    std::move(coalescing_key)};
}

core::TransactionResult add(core::Timeline& timeline,
                            core::RhythmEvent event,
                            std::string transaction_id)
{
    return timeline.submit(request(std::move(transaction_id),
                                   timeline.snapshot()->timeline_revision,
                                   {core::AddEvent{std::move(event)}}));
}

template <typename Result>
void expect_error(const Result& result,
                  core::ErrorCategory category,
                  core::ErrorCode code)
{
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().category, category);
    EXPECT_EQ(result.error().code, code);
}

void expect_error(const core::TransactionResult& result,
                  core::ErrorCategory category,
                  core::ErrorCode code)
{
    ASSERT_TRUE(result.error.has_value());
    EXPECT_EQ(result.error->category, category);
    EXPECT_EQ(result.error->code, code);
}

core::MergeAnalysisCandidates merge_operation(
    std::string revision,
    std::vector<core::AnalysisCandidate> candidates,
    core::MergeParameters parameters = {})
{
    return core::MergeAnalysisCandidates{core::AnalysisRevision{std::move(revision)},
                                         std::move(candidates),
                                         std::move(parameters)};
}

std::vector<std::string> event_ids(const core::TimelineSnapshot& snapshot)
{
    std::vector<std::string> ids;
    for (const auto& event : snapshot.events) {
        ids.push_back(event.id.value);
    }
    return ids;
}

} // namespace

TEST(A012TimeVectors, TV_TIME_001_015)
{
    {
        A012_VECTOR("TV-TIME-001");
        const auto result = core::scale_ticks(
            0, {1, 1}, core::RoundingMode::nearest_ties_to_even);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 0);
    }
    {
        A012_VECTOR("TV-TIME-002");
        const auto result = core::scale_ticks(
            30, {1, 30}, core::RoundingMode::nearest_ties_to_even);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 1'000'000'000);
    }
    {
        A012_VECTOR("TV-TIME-003");
        const auto result = core::scale_ticks(
            48'000, {1, 48'000}, core::RoundingMode::nearest_ties_to_even);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 1'000'000'000);
    }
    {
        A012_VECTOR("TV-TIME-004");
        const auto result = core::scale_ticks(
            1, {1001, 30'000}, core::RoundingMode::nearest_ties_to_even);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 33'366'667);
    }
    {
        A012_VECTOR("TV-TIME-005");
        constexpr std::array modes{core::RoundingMode::floor,
                                   core::RoundingMode::ceil,
                                   core::RoundingMode::toward_zero,
                                   core::RoundingMode::nearest_ties_to_even};
        constexpr std::array<core::TimeNs, 4> expected{0, 1, 0, 0};
        for (std::size_t index = 0; index < modes.size(); ++index) {
            const auto result = core::scale_ticks(1, {1, 2'000'000'000}, modes[index]);
            ASSERT_TRUE(result);
            EXPECT_EQ(result.value(), expected[index]);
        }
    }
    {
        A012_VECTOR("TV-TIME-006");
        constexpr std::array modes{core::RoundingMode::floor,
                                   core::RoundingMode::ceil,
                                   core::RoundingMode::toward_zero,
                                   core::RoundingMode::nearest_ties_to_even};
        constexpr std::array<core::TimeNs, 4> expected{1, 2, 1, 2};
        for (std::size_t index = 0; index < modes.size(); ++index) {
            const auto result = core::scale_ticks(3, {1, 2'000'000'000}, modes[index]);
            ASSERT_TRUE(result);
            EXPECT_EQ(result.value(), expected[index]);
        }
    }
    {
        A012_VECTOR("TV-TIME-007");
        constexpr std::array modes{core::RoundingMode::floor,
                                   core::RoundingMode::ceil,
                                   core::RoundingMode::toward_zero,
                                   core::RoundingMode::nearest_ties_to_even};
        constexpr std::array<core::TimeNs, 4> expected{-1, 0, 0, 0};
        for (std::size_t index = 0; index < modes.size(); ++index) {
            const auto result = core::scale_ticks(-1, {1, 2'000'000'000}, modes[index]);
            ASSERT_TRUE(result);
            EXPECT_EQ(result.value(), expected[index]);
        }
    }
    {
        A012_VECTOR("TV-TIME-008");
        constexpr std::array modes{core::RoundingMode::floor,
                                   core::RoundingMode::ceil,
                                   core::RoundingMode::toward_zero,
                                   core::RoundingMode::nearest_ties_to_even};
        constexpr std::array<core::TimeNs, 4> expected{-2, -1, -1, -2};
        for (std::size_t index = 0; index < modes.size(); ++index) {
            const auto result = core::scale_ticks(-3, {1, 2'000'000'000}, modes[index]);
            ASSERT_TRUE(result);
            EXPECT_EQ(result.value(), expected[index]);
        }
    }
    {
        A012_VECTOR("TV-TIME-009");
        expect_error(core::scale_ticks(1, {0, 1}, core::RoundingMode::floor),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_time_base);
        expect_error(core::scale_ticks(1, {1, 0}, core::RoundingMode::floor),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_time_base);
        expect_error(core::scale_ticks(1, {1, -1}, core::RoundingMode::floor),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_time_base);
    }
    {
        A012_VECTOR("TV-TIME-010");
        expect_error(core::checked_add(std::numeric_limits<core::TimeNs>::max(), 1),
                     core::ErrorCategory::validation,
                     core::ErrorCode::time_overflow);
        expect_error(core::checked_subtract(std::numeric_limits<core::TimeNs>::min(), 1),
                     core::ErrorCategory::validation,
                     core::ErrorCode::time_overflow);
    }
    {
        A012_VECTOR("TV-TIME-011");
        const auto range = core::make_range(0, 0);
        ASSERT_TRUE(range);
        EXPECT_EQ(range.value(), (core::TimeRange{0, 0}));
    }
    {
        A012_VECTOR("TV-TIME-012");
        const auto range = core::make_range(10, 5);
        ASSERT_TRUE(range);
        EXPECT_TRUE(range.value().contains(10));
        EXPECT_FALSE(range.value().contains(15));
    }
    {
        A012_VECTOR("TV-TIME-013");
        const core::TimeRange first{0, 10};
        const core::TimeRange second{10, 20};
        EXPECT_FALSE(first.overlaps(second));
    }
    {
        A012_VECTOR("TV-TIME-014");
        expect_error(core::make_range(std::numeric_limits<core::TimeNs>::max(), 1),
                     core::ErrorCategory::validation,
                     core::ErrorCode::time_overflow);
    }
    {
        A012_VECTOR("TV-TIME-015");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto event = manual_event();
        event.time_ns = -1;
        const auto result = add(timeline, event, "time-015");
        EXPECT_EQ(result.status, core::TransactionStatus::rejected);
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
    }
}

TEST(A012EventVectors, TV_EVENT_001_014)
{
    {
        A012_VECTOR("TV-EVENT-001");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto result = add(timeline, manual_event(), "event-001");
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
        ASSERT_EQ(timeline.snapshot()->events.size(), 1);
        EXPECT_EQ(timeline.snapshot()->events.front(), manual_event());
    }
    {
        A012_VECTOR("TV-EVENT-002");
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"same"}, 0, {}, {}},
                           {core::TrackId{"same"}, 1, {}, {}}};
        expect_error(core::validate_and_normalize_snapshot(std::move(snapshot)),
                     core::ErrorCategory::validation,
                     core::ErrorCode::duplicate_track_id);
    }
    {
        A012_VECTOR("TV-EVENT-003");
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}}};
        snapshot.events = {manual_event(), manual_event()};
        expect_error(core::validate_and_normalize_snapshot(std::move(snapshot)),
                     core::ErrorCategory::validation,
                     core::ErrorCode::duplicate_event_id);
    }
    {
        A012_VECTOR("TV-EVENT-004");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto result = add(timeline,
                                manual_event("manual-1", 100, "track-missing"),
                                "event-004");
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::track_not_found);
    }
    {
        A012_VECTOR("TV-EVENT-005");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto event = manual_event();
        event.strength_ppm = 1'000'001;
        expect_error(add(timeline, event, "event-005a"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
        event = analysis_event("auto", 100);
        event.confidence_ppm = 1'000'001;
        expect_error(add(timeline, event, "event-005b"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
    }
    {
        A012_VECTOR("TV-EVENT-006");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto event = manual_event();
        event.confidence_ppm = 1;
        expect_error(add(timeline, event, "event-006"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
    }
    {
        A012_VECTOR("TV-EVENT-007");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto event = analysis_event("auto", 100);
        event.source.analysis_revision.reset();
        expect_error(add(timeline, event, "event-007a"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
        event = analysis_event("auto", 100);
        event.source.input_fingerprint.reset();
        expect_error(add(timeline, event, "event-007b"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
        event = analysis_event("auto", 100);
        event.source.parameters_digest.reset();
        expect_error(add(timeline, event, "event-007c"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
    }
    {
        A012_VECTOR("TV-EVENT-008");
        expect_error(core::parse_event_kind("future_kind"),
                     core::ErrorCategory::compatibility,
                     core::ErrorCode::unknown_enum);
    }
    {
        A012_VECTOR("TV-EVENT-009");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto event = manual_event("edge", std::numeric_limits<core::TimeNs>::max());
        event.duration_ns = 1;
        expect_error(add(timeline, event, "event-009"),
                     core::ErrorCategory::validation,
                     core::ErrorCode::time_overflow);
    }
    {
        A012_VECTOR("TV-EVENT-010");
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}},
                           {core::TrackId{"track-1"}, 1, {}, {}}};
        snapshot.events = {manual_event("event-b", 100, "track-0"),
                           manual_event("event-a", 100, "track-1"),
                           manual_event("event-a0", 100, "track-0")};
        const auto normalized = core::validate_and_normalize_snapshot(std::move(snapshot));
        ASSERT_TRUE(normalized);
        EXPECT_EQ(event_ids(normalized.value()),
                  (std::vector<std::string>{"event-a0", "event-b", "event-a"}));
    }
    {
        A012_VECTOR("TV-EVENT-011");
        std::array<core::RhythmEvent, 3> events{
            manual_event("event-b", 100, "track-0"),
            manual_event("event-a", 100, "track-1"),
            manual_event("event-a0", 100, "track-0")};
        for (int iteration = 0; iteration < 100; ++iteration) {
            std::rotate(events.begin(), events.begin() + 1, events.end());
            core::TimelineSnapshot snapshot;
            snapshot.project_id = core::ProjectId{"project"};
            snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}},
                               {core::TrackId{"track-1"}, 1, {}, {}}};
            snapshot.events.assign(events.begin(), events.end());
            const auto normalized =
                core::validate_and_normalize_snapshot(std::move(snapshot));
            ASSERT_TRUE(normalized);
            EXPECT_EQ(event_ids(normalized.value()),
                      (std::vector<std::string>{"event-a0", "event-b", "event-a"}));
        }
    }
    {
        A012_VECTOR("TV-EVENT-012");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "event-012-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "event-012", 1, {core::RemoveTrack{core::TrackId{"track-0"}}}));
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::track_not_empty);
        EXPECT_EQ(timeline.snapshot()->events.size(), 1);
        EXPECT_EQ(timeline.snapshot()->tracks.size(), 1);
    }
    {
        A012_VECTOR("TV-EVENT-013");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "event-013-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "event-013",
            1,
            {core::RemoveTrack{core::TrackId{"track-0"},
                               core::RemoveTrackPolicy::delete_events}}));
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        EXPECT_TRUE(timeline.snapshot()->tracks.empty());
        EXPECT_TRUE(timeline.snapshot()->events.empty());
        EXPECT_EQ(timeline.undo(2).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->tracks.size(), 1);
        EXPECT_EQ(timeline.snapshot()->events.size(), 1);
    }
    {
        A012_VECTOR("TV-EVENT-014");
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}},
                           {core::TrackId{"track-1"}, 2, {}, {}}};
        expect_error(core::validate_and_normalize_snapshot(std::move(snapshot)),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_track);
    }
}

TEST(A012TransactionVectors, TV_TXN_001_012)
{
    {
        A012_VECTOR("TV-TXN-001");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto result = add(timeline, manual_event(), "txn-001");
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        EXPECT_EQ(result.before_revision, 0);
        EXPECT_EQ(result.after_revision, 1);
        EXPECT_EQ(timeline.undo_depth(), 1);
    }
    {
        A012_VECTOR("TV-TXN-002");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "txn-002-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "txn-002", 0, {core::AddEvent{manual_event("manual-2", 200)}}));
        expect_error(result,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::stale_revision);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
        EXPECT_EQ(timeline.snapshot()->events.size(), 1);
    }
    {
        A012_VECTOR("TV-TXN-003");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "txn-003-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "txn-003", 1, {core::PatchEvent{core::EventId{"manual-1"}}}));
        EXPECT_EQ(result.status, core::TransactionStatus::no_change);
        EXPECT_EQ(result.before_revision, 1);
        EXPECT_EQ(result.after_revision, 1);
        EXPECT_EQ(timeline.undo_depth(), 1);
    }
    {
        A012_VECTOR("TV-TXN-004");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto result = timeline.submit(request(
            "txn-004",
            0,
            {core::AddEvent{manual_event()},
             core::MoveEvent{core::EventId{"event-missing"}, 200, std::nullopt}}));
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::event_not_found);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 0);
        EXPECT_TRUE(timeline.snapshot()->events.empty());
    }
    {
        A012_VECTOR("TV-TXN-005");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto maximum = std::numeric_limits<core::TimeNs>::max();
        ASSERT_EQ(timeline.submit(request(
                      "txn-005-add",
                      0,
                      {core::AddEvent{manual_event("one", 10)},
                       core::AddEvent{manual_event("two", maximum)}}))
                      .status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "txn-005",
            1,
            {core::BatchOffsetEvents{{core::EventId{"one"}, core::EventId{"two"}}, 1}}));
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::time_overflow);
        EXPECT_EQ(timeline.snapshot()->events[0].time_ns, 10);
        EXPECT_EQ(timeline.snapshot()->events[1].time_ns, maximum);
    }
    {
        A012_VECTOR("TV-TXN-006");
        core::Timeline timeline{core::ProjectId{"project"}};
        core::CancellationToken cancellation;
        cancellation.cancel();
        const auto result = timeline.submit(
            request("txn-006", 0, {core::AddEvent{manual_event()}}), &cancellation);
        EXPECT_EQ(result.status, core::TransactionStatus::cancelled);
        expect_error(result,
                     core::ErrorCategory::cancelled,
                     core::ErrorCode::transaction_cancelled);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 0);
        EXPECT_EQ(timeline.undo_depth(), 0);
    }
    {
        A012_VECTOR("TV-TXN-007");
        core::Timeline timeline{core::ProjectId{"project"}};
        core::CancellationToken cancellation;
        const auto result = timeline.submit(
            request("txn-007", 0, {core::AddEvent{manual_event()}}), &cancellation);
        cancellation.cancel();
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
    }
    {
        A012_VECTOR("TV-TXN-008");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto transaction = request("txn-008", 0, {core::AddEvent{manual_event()}});
        const auto first = timeline.submit(transaction);
        const auto replay = timeline.submit(transaction);
        EXPECT_EQ(first, replay);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
        EXPECT_EQ(timeline.undo_depth(), 1);
    }
    {
        A012_VECTOR("TV-TXN-009");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(timeline.submit(
                      request("txn-009", 0, {core::AddEvent{manual_event()}}))
                      .status,
                  core::TransactionStatus::committed);
        const auto reused = timeline.submit(request(
            "txn-009", 1, {core::AddEvent{manual_event("manual-2", 200)}}));
        expect_error(reused,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::idempotency_conflict);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
    }
    {
        A012_VECTOR("TV-TXN-010");
        core::TimelineSnapshot loaded;
        loaded.project_id = core::ProjectId{"project"};
        loaded.timeline_revision = std::numeric_limits<core::TimelineRevision>::max();
        loaded.tracks = {{core::TrackId{"track-0"}, 0, {}, {}}};
        core::Timeline timeline{std::move(loaded)};
        const auto result = timeline.submit(request(
            "txn-010", timeline.snapshot()->timeline_revision, {core::AddEvent{manual_event()}}));
        expect_error(result,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::revision_exhausted);
        EXPECT_TRUE(timeline.snapshot()->events.empty());
    }
    {
        A012_VECTOR("TV-TXN-011");
        const auto exercise = [](core::Operation operation, std::string id) {
            core::Timeline timeline{core::ProjectId{"project"}};
            ASSERT_EQ(add(timeline, analysis_event("locked", 100, true), id + "-add").status,
                      core::TransactionStatus::committed);
            const auto result = timeline.submit(
                request(std::move(id), 1, {std::move(operation)},
                        core::TransactionOrigin::analysis));
            expect_error(result,
                         core::ErrorCategory::conflict,
                         core::ErrorCode::locked_event);
            EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
        };
        exercise(core::MoveEvent{core::EventId{"locked"}, 200, std::nullopt},
                 "txn-011-move");
        exercise(core::PatchEvent{core::EventId{"locked"},
                                  std::nullopt,
                                  std::nullopt,
                                  std::nullopt,
                                  std::nullopt,
                                  200'000},
                 "txn-011-patch");
        exercise(core::RemoveEvent{core::EventId{"locked"}}, "txn-011-remove");
    }
    {
        A012_VECTOR("TV-TXN-012");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, analysis_event("locked", 100, true), "txn-012-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "txn-012",
            1,
            {core::SetEventLocked{core::EventId{"locked"}, false},
             core::MoveEvent{core::EventId{"locked"}, 200, std::nullopt}}));
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        ASSERT_EQ(timeline.snapshot()->events.size(), 1);
        EXPECT_FALSE(timeline.snapshot()->events.front().locked);
        EXPECT_EQ(timeline.snapshot()->events.front().time_ns, 200);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 2);
    }
}

TEST(A012MergeVectors, TV_MERGE_001_007)
{
    {
        A012_VECTOR("TV-MERGE-001");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "merge-001-add").status,
                  core::TransactionStatus::committed);
        core::MergeParameters parameters;
        parameters.protection_window_ns = 10;
        const auto result = timeline.submit(request(
            "merge-001",
            1,
            {merge_operation("analysis-a", {candidate("candidate-1", 100)}, parameters)},
            core::TransactionOrigin::analysis));
        EXPECT_EQ(result.status, core::TransactionStatus::no_change);
        ASSERT_EQ(result.suppression_report.size(), 1);
        EXPECT_EQ(result.suppression_report.front().reason,
                  core::SuppressionReason::protected_manual);
        EXPECT_EQ(timeline.snapshot()->events.front(), manual_event());
    }
    {
        A012_VECTOR("TV-MERGE-002");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto original = analysis_event("locked", 100, true);
        ASSERT_EQ(add(timeline, original, "merge-002-add").status,
                  core::TransactionStatus::committed);
        core::MergeParameters parameters;
        parameters.protection_window_ns = 10;
        const auto result = timeline.submit(request(
            "merge-002",
            1,
            {merge_operation("analysis-a", {candidate("candidate-2", 100)}, parameters)},
            core::TransactionOrigin::analysis));
        EXPECT_EQ(result.status, core::TransactionStatus::no_change);
        ASSERT_EQ(result.suppression_report.size(), 1);
        EXPECT_EQ(result.suppression_report.front().reason,
                  core::SuppressionReason::protected_locked);
        EXPECT_EQ(timeline.snapshot()->events.front(), original);
    }
    {
        A012_VECTOR("TV-MERGE-003");
        const auto execute = [](std::vector<core::AnalysisCandidate> candidates,
                                std::string transaction_id) {
            core::Timeline timeline{core::ProjectId{"project"}};
            core::MergeParameters parameters;
            parameters.minimum_spacing_ns = 20;
            parameters.seed = 42;
            const auto result = timeline.submit(request(
                std::move(transaction_id),
                0,
                {merge_operation("analysis-a", std::move(candidates), parameters)},
                core::TransactionOrigin::analysis));
            EXPECT_EQ(result.status, core::TransactionStatus::committed);
            return *timeline.snapshot();
        };
        auto first = execute({candidate("candidate-a", 100, "analysis-a", 700'000),
                              candidate("candidate-b", 110, "analysis-a", 600'000),
                              candidate("candidate-c", 200, "analysis-a", 500'000)},
                             "merge-003a");
        auto second = execute({candidate("candidate-c", 200, "analysis-a", 500'000),
                               candidate("candidate-b", 110, "analysis-a", 600'000),
                               candidate("candidate-a", 100, "analysis-a", 700'000)},
                              "merge-003b");
        first.timeline_revision = 0;
        second.timeline_revision = 0;
        EXPECT_EQ(first, second);
    }
    {
        A012_VECTOR("TV-MERGE-004");
        core::Timeline timeline{core::ProjectId{"project"}};
        auto invalid = candidate("candidate-bad", 200);
        invalid.strength_ppm = 1'000'001;
        const auto result = timeline.submit(request(
            "merge-004",
            0,
            {merge_operation("analysis-a", {candidate("candidate-good", 100), invalid})},
            core::TransactionOrigin::analysis));
        expect_error(result,
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_event);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 0);
        EXPECT_TRUE(timeline.snapshot()->events.empty());
        EXPECT_EQ(timeline.undo_depth(), 0);
    }
    {
        A012_VECTOR("TV-MERGE-005");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event("manual-2", 500), "merge-005-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "merge-005",
            0,
            {merge_operation("analysis-a", {candidate("candidate-1", 100)})},
            core::TransactionOrigin::analysis));
        expect_error(result,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::stale_revision);
        EXPECT_EQ(timeline.snapshot()->events.size(), 1);
    }
    {
        A012_VECTOR("TV-MERGE-006");
        core::Timeline timeline{core::ProjectId{"project"}};
        const auto result = timeline.submit(request(
            "merge-006",
            0,
            {merge_operation("analysis-a", {candidate("candidate-1", 100)})},
            core::TransactionOrigin::analysis));
        EXPECT_EQ(result.status, core::TransactionStatus::committed);
        const auto& source = timeline.snapshot()->events.front().source;
        ASSERT_TRUE(source.analysis_revision);
        EXPECT_EQ(source.analysis_revision->value, "analysis-a");
        EXPECT_EQ(source.producer_version, "1.0");
        EXPECT_EQ(source.input_fingerprint, "input-sha256");
        EXPECT_EQ(source.parameters_digest, "params-sha256");
        EXPECT_EQ(source.candidate_ids,
                  (std::vector<core::CandidateId>{core::CandidateId{"candidate-1"}}));
    }
    {
        A012_VECTOR("TV-MERGE-007");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(timeline.submit(request(
                      "merge-007a",
                      0,
                      {merge_operation("analysis-a", {candidate("candidate-1", 100)})},
                      core::TransactionOrigin::analysis))
                      .status,
                  core::TransactionStatus::committed);
        const auto result = timeline.submit(request(
            "merge-007b",
            1,
            {merge_operation("analysis-a", {candidate("candidate-2", 200)})},
            core::TransactionOrigin::analysis));
        expect_error(result,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::analysis_revision_conflict);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
    }
}

TEST(A012HistoryVectors, TV_HISTORY_001_012)
{
    {
        A012_VECTOR("TV-HISTORY-001");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-001-add").status,
                  core::TransactionStatus::committed);
        EXPECT_EQ(timeline.undo(1).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 2);
        EXPECT_TRUE(timeline.snapshot()->events.empty());
        EXPECT_EQ(timeline.redo_depth(), 1);
    }
    {
        A012_VECTOR("TV-HISTORY-002");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-002-add").status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.undo(1).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.redo(2).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 3);
        ASSERT_EQ(timeline.snapshot()->events.size(), 1);
        EXPECT_EQ(timeline.snapshot()->events.front(), manual_event());
    }
    {
        A012_VECTOR("TV-HISTORY-003");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-003-add").status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.undo(1).status, core::TransactionStatus::committed);
        ASSERT_EQ(add(timeline, manual_event("manual-2", 200), "history-003-branch").status,
                  core::TransactionStatus::committed);
        EXPECT_EQ(timeline.redo_depth(), 0);
        EXPECT_EQ(timeline.redo(3).status, core::TransactionStatus::no_change);
    }
    {
        A012_VECTOR("TV-HISTORY-004");
        core::Timeline timeline{core::ProjectId{"project"}};
        EXPECT_EQ(timeline.undo(0).status, core::TransactionStatus::no_change);
        EXPECT_EQ(timeline.redo(0).status, core::TransactionStatus::no_change);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 0);
    }
    {
        A012_VECTOR("TV-HISTORY-005");
        core::Timeline timeline{core::ProjectId{"project"}};
        std::vector<core::Operation> additions;
        std::vector<core::EventId> ids;
        for (int index = 0; index < 20; ++index) {
            const auto id = "event-" + std::to_string(index);
            ids.push_back(core::EventId{id});
            additions.push_back(core::AddEvent{manual_event(id, index * 100)});
        }
        ASSERT_EQ(timeline.submit(request("history-005-add", 0, std::move(additions))).status,
                  core::TransactionStatus::committed);
        const auto before = timeline.snapshot()->events;
        ASSERT_EQ(timeline.submit(request(
                      "history-005-offset", 1, {core::BatchOffsetEvents{ids, 50}}))
                      .status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.undo(2).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->events, before);
        EXPECT_EQ(timeline.snapshot()->timeline_revision, 3);
    }
    {
        A012_VECTOR("TV-HISTORY-006");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(timeline.submit(request(
                      "history-006-merge",
                      0,
                      {merge_operation("analysis-a",
                                       {candidate("candidate-1", 100),
                                        candidate("candidate-2", 200)})},
                      core::TransactionOrigin::analysis))
                      .status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.snapshot()->events.size(), 2);
        EXPECT_EQ(timeline.undo(1).status, core::TransactionStatus::committed);
        EXPECT_TRUE(timeline.snapshot()->events.empty());
    }
    {
        A012_VECTOR("TV-HISTORY-007");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-007-add").status,
                  core::TransactionStatus::committed);
        const auto result = timeline.undo(0);
        expect_error(result,
                     core::ErrorCategory::conflict,
                     core::ErrorCode::stale_revision);
        EXPECT_EQ(timeline.undo_depth(), 1);
        EXPECT_EQ(timeline.redo_depth(), 0);
    }
    {
        A012_VECTOR("TV-HISTORY-008");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-008-add").status,
                  core::TransactionStatus::committed);
        timeline.mark_saved();
        ASSERT_EQ(timeline.undo(1).status, core::TransactionStatus::committed);
        EXPECT_TRUE(timeline.is_dirty());
        ASSERT_EQ(timeline.redo(2).status, core::TransactionStatus::committed);
        EXPECT_FALSE(timeline.is_dirty());
    }
    {
        A012_VECTOR("TV-HISTORY-009");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-009-add").status,
                  core::TransactionStatus::committed);
        timeline.mark_saved();
        ASSERT_EQ(timeline.submit(request(
                      "history-009-move",
                      1,
                      {core::MoveEvent{core::EventId{"manual-1"}, 200, std::nullopt}}))
                      .status,
                  core::TransactionStatus::committed);
        EXPECT_TRUE(timeline.is_dirty());
        ASSERT_EQ(timeline.undo(2).status, core::TransactionStatus::committed);
        EXPECT_FALSE(timeline.is_dirty());
    }
    {
        A012_VECTOR("TV-HISTORY-010");
        core::Timeline original{core::ProjectId{"project"}};
        ASSERT_EQ(add(original, manual_event(), "history-010-add").status,
                  core::TransactionStatus::committed);
        core::Timeline reopened{*original.snapshot()};
        EXPECT_FALSE(reopened.is_dirty());
        EXPECT_EQ(reopened.undo_depth(), 0);
        EXPECT_EQ(reopened.redo_depth(), 0);
        ASSERT_EQ(add(reopened, manual_event("manual-2", 200), "history-010-next").status,
                  core::TransactionStatus::committed);
        EXPECT_EQ(reopened.snapshot()->timeline_revision, 2);
    }
    {
        A012_VECTOR("TV-HISTORY-011");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-011-add").status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.submit(request(
                      "history-011-a",
                      1,
                      {core::MoveEvent{core::EventId{"manual-1"}, 101, std::nullopt}},
                      core::TransactionOrigin::user,
                      "drag-manual"))
                      .status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.submit(request(
                      "history-011-b",
                      2,
                      {core::MoveEvent{core::EventId{"manual-1"}, 102, std::nullopt}},
                      core::TransactionOrigin::user,
                      "drag-manual"))
                      .status,
                  core::TransactionStatus::committed);
        EXPECT_EQ(timeline.undo_depth(), 2);
        ASSERT_EQ(timeline.undo(3).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->events.front().time_ns, 100);
    }
    {
        A012_VECTOR("TV-HISTORY-012");
        core::Timeline timeline{core::ProjectId{"project"}};
        ASSERT_EQ(add(timeline, manual_event(), "history-012-add").status,
                  core::TransactionStatus::committed);
        ASSERT_EQ(timeline.submit(request(
                      "history-012-a",
                      1,
                      {core::MoveEvent{core::EventId{"manual-1"}, 101, std::nullopt}},
                      core::TransactionOrigin::user,
                      "drag-manual"))
                      .status,
                  core::TransactionStatus::committed);
        timeline.mark_saved();
        ASSERT_EQ(timeline.submit(request(
                      "history-012-b",
                      2,
                      {core::MoveEvent{core::EventId{"manual-1"}, 102, std::nullopt}},
                      core::TransactionOrigin::user,
                      "drag-manual"))
                      .status,
                  core::TransactionStatus::committed);
        EXPECT_EQ(timeline.undo_depth(), 3);
        ASSERT_EQ(timeline.undo(3).status, core::TransactionStatus::committed);
        EXPECT_EQ(timeline.snapshot()->events.front().time_ns, 101);
    }
}

TEST(A012DtoVectors, TV_DTO_001_012)
{
    {
        A012_VECTOR("TV-DTO-001");
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}}};
        snapshot.events = {manual_event()};
        const auto normalized = core::validate_and_normalize_snapshot(snapshot);
        ASSERT_TRUE(normalized);
        EXPECT_EQ(normalized.value(), snapshot);
    }
    {
        A012_VECTOR("TV-DTO-002");
        core::SchemaEnvelope envelope;
        envelope.schema_version = 2;
        expect_error(core::validate_schema_envelope(std::move(envelope)),
                     core::ErrorCategory::compatibility,
                     core::ErrorCode::unsupported_schema);
    }
    {
        A012_VECTOR("TV-DTO-003");
        for (const auto shape : {core::DecodedDtoShape{true, false, false, {}},
                                 core::DecodedDtoShape{false, true, false, {}},
                                 core::DecodedDtoShape{false, false, true, {}}}) {
            expect_error(core::validate_decoded_dto_shape(shape),
                         core::ErrorCategory::validation,
                         core::ErrorCode::invalid_dto);
        }
    }
    {
        A012_VECTOR("TV-DTO-004");
        expect_error(core::validate_decoded_dto_shape(
                         core::DecodedDtoShape{false, false, false, {"futureField"}}),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_dto);
    }
    {
        A012_VECTOR("TV-DTO-005");
        core::SchemaEnvelope envelope;
        envelope.extensions.emplace(
            "vendor.future",
            core::VersionedOpaqueObject{"vendor.future", 1, {}, {{"opaque", "value"}}});
        const auto result = core::validate_schema_envelope(envelope);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), envelope);
    }
    {
        A012_VECTOR("TV-DTO-006");
        core::SchemaEnvelope envelope;
        envelope.required_features = {"future.required"};
        expect_error(core::validate_schema_envelope(std::move(envelope)),
                     core::ErrorCategory::compatibility,
                     core::ErrorCode::unsupported_feature);
    }
    {
        A012_VECTOR("TV-DTO-007");
        expect_error(core::parse_event_kind("future_kind"),
                     core::ErrorCategory::compatibility,
                     core::ErrorCode::unknown_enum);
    }
    {
        A012_VECTOR("TV-DTO-008");
        const auto maximum = std::numeric_limits<core::TimeNs>::max();
        core::TimelineSnapshot snapshot;
        snapshot.project_id = core::ProjectId{"project"};
        snapshot.tracks = {{core::TrackId{"track-0"}, 0, {}, {}}};
        snapshot.events = {manual_event("maximum", maximum)};
        const auto result = core::validate_and_normalize_snapshot(std::move(snapshot));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value().events.front().time_ns, maximum);
    }
    {
        A012_VECTOR("TV-DTO-009");
        for (const auto& invalid : {std::string{"has space"},
                                    std::string{"has/slash"},
                                    std::string{"non-ascii-\xC3\xA9"},
                                    std::string(129, 'a')}) {
            expect_error(core::validate_identifier(invalid),
                         core::ErrorCategory::validation,
                         core::ErrorCode::invalid_identifier);
        }
    }
    {
        A012_VECTOR("TV-DTO-010");
        core::ErrorInfo invalid;
        invalid.stage = "decode";
        invalid.message_key = "invalid_dto";
        expect_error(core::validate_error_info(std::move(invalid)),
                     core::ErrorCategory::validation,
                     core::ErrorCode::invalid_dto);
    }
    {
        A012_VECTOR("TV-DTO-011");
        core::ErrorInfo cause;
        cause.category = core::ErrorCategory::validation;
        cause.code = core::ErrorCode::invalid_event;
        cause.stage = "domain";
        cause.diagnostic_id = "diagnostic:1";
        cause.message_key = "invalid_event";
        const auto wrapped = core::wrap_error(cause, "adapter");
        ASSERT_TRUE(wrapped.cause);
        EXPECT_EQ(wrapped.diagnostic_id, cause.diagnostic_id);
        EXPECT_EQ(wrapped.cause->diagnostic_id, cause.diagnostic_id);
        EXPECT_EQ(wrapped.cause->code, core::ErrorCode::invalid_event);
    }
    {
        A012_VECTOR("TV-DTO-012");
        core::ErrorInfo mapped;
        try {
            throw std::runtime_error{"unknown"};
        } catch (...) {
            mapped = core::map_current_exception("boundary");
        }
        EXPECT_EQ(mapped.category, core::ErrorCategory::internal);
        EXPECT_EQ(mapped.code, core::ErrorCode::internal_error);
        EXPECT_FALSE(mapped.diagnostic_id.empty());
    }
}

TEST(CoreConcurrency, SameBaseRevisionHasSingleCommitPoint)
{
    core::Timeline timeline{core::ProjectId{"project"}};
    std::atomic_bool start{false};
    std::array<core::TransactionResult, 2> results;
    auto submit = [&](std::size_t index) {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        results[index] = timeline.submit(request(
            "concurrent-" + std::to_string(index),
            0,
            {core::AddEvent{manual_event("event-" + std::to_string(index),
                                                static_cast<core::TimeNs>(index))}}));
    };
    std::thread first{submit, 0};
    std::thread second{submit, 1};
    start.store(true, std::memory_order_release);
    first.join();
    second.join();

    const auto committed = std::ranges::count_if(results, [](const auto& result) {
        return result.status == core::TransactionStatus::committed;
    });
    const auto stale = std::ranges::count_if(results, [](const auto& result) {
        return result.error && result.error->code == core::ErrorCode::stale_revision;
    });
    EXPECT_EQ(committed, 1);
    EXPECT_EQ(stale, 1);
    EXPECT_EQ(timeline.snapshot()->timeline_revision, 1);
    EXPECT_EQ(timeline.snapshot()->events.size(), 1);
}

TEST(CoreTimelineTransactions, TrackLifecycleAndSnapshotsRemainImmutable)
{
    core::Timeline timeline{core::ProjectId{"project"}};
    const auto initial = timeline.snapshot();
    ASSERT_EQ(timeline.submit(request(
                  "track-add",
                  0,
                  {core::AddTrack{core::Track{core::TrackId{"track-1"},
                                               1,
                                               std::string{"Secondary"},
                                               {}}},
                   core::RenameTrack{core::TrackId{"track-0"},
                                     std::string{"Primary"}}}))
                  .status,
              core::TransactionStatus::committed);
    EXPECT_EQ(initial->timeline_revision, 0);
    EXPECT_EQ(initial->tracks.size(), 1);

    const auto reordered = timeline.submit(request(
        "track-reorder",
        1,
        {core::ReorderTracks{{core::TrackId{"track-1"}, core::TrackId{"track-0"}}}}));
    EXPECT_EQ(reordered.status, core::TransactionStatus::committed);
    ASSERT_EQ(timeline.snapshot()->tracks.size(), 2);
    EXPECT_EQ(timeline.snapshot()->tracks[0].id.value, "track-1");
    EXPECT_EQ(timeline.snapshot()->tracks[1].id.value, "track-0");
    EXPECT_EQ(timeline.snapshot()->tracks[1].label, "Primary");
}

TEST(CoreFusionPolicy, DuplicateSpacingDensityAndReplacementAreDeterministic)
{
    core::Timeline timeline{core::ProjectId{"project"}};
    ASSERT_EQ(add(timeline, analysis_event("old-analysis", 50), "fusion-old").status,
              core::TransactionStatus::committed);
    core::MergeParameters parameters;
    parameters.minimum_spacing_ns = 10;
    parameters.density_window_ns = 100;
    parameters.max_events_per_density_window = 1;
    parameters.seed = 7;
    auto first = candidate("candidate-1", 100, "analysis-a", 900'000, 900'000);
    auto duplicate = first;
    auto close = candidate("candidate-2", 105, "analysis-a", 800'000, 800'000);
    auto dense = candidate("candidate-3", 150, "analysis-a", 700'000, 700'000);
    const auto result = timeline.submit(request(
        "fusion-policy",
        1,
        {merge_operation("analysis-a",
                         {std::move(dense),
                          std::move(duplicate),
                          std::move(close),
                          std::move(first)},
                         parameters)},
        core::TransactionOrigin::analysis));
    EXPECT_EQ(result.status, core::TransactionStatus::committed);
    ASSERT_EQ(timeline.snapshot()->events.size(), 1);
    EXPECT_EQ(timeline.snapshot()->events.front().time_ns, 100);
    std::multiset<core::SuppressionReason> reasons;
    for (const auto& suppressed : result.suppression_report) {
        reasons.insert(suppressed.reason);
    }
    EXPECT_EQ(reasons,
              (std::multiset<core::SuppressionReason>{
                  core::SuppressionReason::duplicate_candidate,
                  core::SuppressionReason::merge_window,
                  core::SuppressionReason::density_limit}));
}
