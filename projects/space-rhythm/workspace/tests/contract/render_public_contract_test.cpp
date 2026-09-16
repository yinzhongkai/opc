#include <space_rhythm/rendering/render_contract.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace rendering = space_rhythm::rendering;

constexpr std::string_view digest_a{
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
constexpr std::string_view digest_b{
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
constexpr std::string_view digest_c{
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"};

core::TimelineSnapshot make_timeline(core::TimelineRevision revision = 7)
{
    core::TimelineSnapshot timeline;
    timeline.project_id = core::ProjectId{"project-render"};
    timeline.timeline_revision = revision;
    timeline.tracks.push_back(
        core::Track{core::TrackId{"track-0"}, 0, std::string{"Main"}, {}});
    core::EventSource source;
    source.origin = core::EventOrigin::user;
    source.producer_id = "space-rhythm.user";
    source.producer_version = "0.1.0";
    timeline.events.push_back(core::RhythmEvent{core::EventId{"event-1"},
                                                core::TrackId{"track-0"},
                                                10'000'000,
                                                0,
                                                core::EventKind::beat,
                                                std::move(source),
                                                500'000,
                                                std::nullopt,
                                                false,
                                                false,
                                                std::nullopt,
                                                {}});
    return timeline;
}

rendering::FeatureInputRevision make_feature_revision()
{
    return rendering::FeatureInputRevision{core::AnalysisRevision{"analysis-1"},
                                            "0.2.0",
                                            1,
                                            std::string{digest_a},
                                            std::string{digest_b},
                                            std::string{digest_c}};
}

rendering::RenderRecipe make_recipe()
{
    rendering::RenderRecipe recipe;
    recipe.recipe_id = rendering::RenderRecipeId{"recipe-1"};
    recipe.project_id = core::ProjectId{"project-render"};
    recipe.timeline_revision = 7;
    recipe.feature_inputs = {make_feature_revision()};
    recipe.template_parameters.template_id = "template.contract-fixture";
    recipe.template_parameters.template_version = "0.1.0";
    recipe.template_parameters.parameters_digest_sha256 = std::string{digest_b};
    recipe.template_parameters.integer_parameters = {{"gain-ppm", 1'000'000}};
    recipe.output.width_px = 2;
    recipe.output.height_px = 2;
    recipe.output.alpha_mode = rendering::AlphaMode::straight;
    recipe.time_range = core::TimeRange{0, 100'000'000};
    recipe.frame_rate = rendering::FrameRate{30'000, 1'001};
    recipe.deterministic_seed = 42;
    recipe.required_features = {"render.rgba8-srgb-v1", "render.snapshot-v1"};
    return recipe;
}

rendering::RenderSeries make_series()
{
    return rendering::RenderSeries{rendering::RenderSeriesId{"series-1"},
                                   core::AnalysisRevision{"analysis-1"},
                                   "audio.short-time-energy.v1",
                                   "0.2.0",
                                   1,
                                   std::string{digest_a},
                                   std::string{digest_b},
                                   std::string{digest_c},
                                   {{20'000'000, 100'000, std::nullopt},
                                    {60'000'000, 900'000, 400'000}}};
}

std::shared_ptr<const rendering::RenderSnapshot> make_snapshot()
{
    auto result = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{"snapshot-1"},
        make_recipe(),
        make_timeline(),
        {make_series()});
    EXPECT_TRUE(result) << result.error().message_key;
    return result.value();
}

rendering::RenderedFrame make_frame(rendering::FrameIndex index = 0)
{
    rendering::RenderedFrame frame;
    frame.snapshot_id = rendering::RenderSnapshotId{"snapshot-1"};
    frame.timeline_revision = 7;
    frame.device_generation = 1;
    frame.frame_index = index;
    frame.time_ns = index == 0 ? 0 : 33'366'667;
    frame.width_px = 2;
    frame.height_px = 2;
    frame.alpha_mode = rendering::AlphaMode::straight;
    frame.stride_bytes = 8;
    frame.valid_bytes = 16;
    frame.bytes = rendering::FrameLease(std::vector<std::byte>(16, std::byte{0x2a}));
    return frame;
}

TEST(RenderContractVectors, RCTV_VERSION_001_CurrentDescriptorAccepted)
{
    SCOPED_TRACE("RCTV-VERSION-001");
    rendering::ContractDescriptor descriptor;
    descriptor.required_features = {"render.snapshot-v1"};
    descriptor.extensions.emplace(
        "test.extension",
        core::VersionedOpaqueObject{"test.owner", 1, {}, {{"value", "kept"}}});

    auto result = rendering::negotiate_contract(descriptor);

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().extensions.at("test.extension").payload.at("value"),
              "kept");
}

TEST(RenderContractVectors, RCTV_VERSION_002_FutureSchemaRejected)
{
    SCOPED_TRACE("RCTV-VERSION-002");
    rendering::ContractDescriptor descriptor;
    descriptor.schema_version = rendering::schema_version + 1;

    auto result = rendering::negotiate_contract(descriptor);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::ErrorCode::unsupported_schema);
}

TEST(RenderContractVectors, RCTV_VERSION_003_UnknownRequiredFeatureRejected)
{
    SCOPED_TRACE("RCTV-VERSION-003");
    rendering::ContractDescriptor descriptor;
    descriptor.required_features = {"render.future-private-backend"};

    auto result = rendering::negotiate_contract(descriptor);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::ErrorCode::unsupported_feature);
}

TEST(RenderContractVectors, RCTV_VERSION_004_DifferentContractVersionRejected)
{
    SCOPED_TRACE("RCTV-VERSION-004");
    rendering::ContractDescriptor descriptor;
    descriptor.render_contract_version = "1.0.0";

    auto result = rendering::negotiate_contract(descriptor);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::ErrorCode::unsupported_schema);
}

TEST(RenderContractVectors, RCTV_RECIPE_001_CompleteRecipeAccepted)
{
    SCOPED_TRACE("RCTV-RECIPE-001");
    auto result = rendering::validate_render_recipe(make_recipe());

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().timeline_revision, 7U);
    EXPECT_EQ(result.value().deterministic_seed, 42U);
    EXPECT_EQ(result.value().feature_inputs.front().content_digest_sha256, digest_c);
}

TEST(RenderContractVectors, RCTV_SNAPSHOT_001_DeepCopyIsImmutable)
{
    SCOPED_TRACE("RCTV-SNAPSHOT-001");
    auto recipe = make_recipe();
    auto timeline = make_timeline();
    auto series = std::vector<rendering::RenderSeries>{make_series()};

    auto result = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{"snapshot-1"}, recipe, timeline, series);
    ASSERT_TRUE(result);
    static_assert(std::is_same_v<decltype(result.value()->recipe()),
                                 const rendering::RenderRecipe&>);

    recipe.deterministic_seed = 99;
    timeline.events.front().time_ns = 90'000'000;
    series.front().samples.front().primary_ppm = 1;

    EXPECT_EQ(result.value()->recipe().deterministic_seed, 42U);
    EXPECT_EQ(result.value()->timeline().events.front().time_ns, 10'000'000);
    EXPECT_EQ(result.value()->series().front().samples.front().primary_ppm, 100'000U);
}

TEST(RenderContractVectors, RCTV_SNAPSHOT_002_StaleTimelineRevisionRejected)
{
    SCOPED_TRACE("RCTV-SNAPSHOT-002");
    auto result = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{"snapshot-1"},
        make_recipe(),
        make_timeline(8),
        {make_series()});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::ErrorCode::stale_revision);
}

TEST(RenderContractVectors, RCTV_SNAPSHOT_003_StaleFeatureDigestRejected)
{
    SCOPED_TRACE("RCTV-SNAPSHOT-003");
    auto series = make_series();
    series.content_digest_sha256 = std::string{digest_a};

    auto result = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{"snapshot-1"},
        make_recipe(),
        make_timeline(),
        {series});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, core::ErrorCode::stale_revision);
}

TEST(RenderContractVectors, RCTV_TIME_001_RationalFrameTimeUsesNearestEven)
{
    SCOPED_TRACE("RCTV-TIME-001");
    auto recipe = make_recipe();

    auto first = rendering::frame_time_ns(recipe, 0);
    auto second = rendering::frame_time_ns(recipe, 1);
    auto outside = rendering::frame_time_ns(recipe, 3);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value(), 0);
    EXPECT_EQ(second.value(), 33'366'667);
    EXPECT_FALSE(outside);
}

TEST(RenderContractVectors, RCTV_COORD_001_BoundariesAndMidpointAreExact)
{
    SCOPED_TRACE("RCTV-COORD-001");
    const rendering::CoordinateTransform transform{
        rendering::RenderSnapshotId{"snapshot-1"},
        7,
        core::TimeRange{0, 100'000'000},
        rendering::ItemRectSp{10'240, 20'480, 102'400, 40'960}};

    auto left = rendering::time_to_item_x_sp(transform, 0);
    auto middle = rendering::time_to_item_x_sp(transform, 50'000'000);
    auto right = rendering::time_to_item_x_sp(transform, 100'000'000);
    auto reverse = rendering::item_x_sp_to_time(transform, 61'440);

    ASSERT_TRUE(left);
    ASSERT_TRUE(middle);
    ASSERT_TRUE(right);
    ASSERT_TRUE(reverse);
    EXPECT_EQ(left.value(), 10'240);
    EXPECT_EQ(middle.value(), 61'440);
    EXPECT_EQ(right.value(), 112'640);
    EXPECT_EQ(reverse.value(), 50'000'000);
    EXPECT_FALSE(rendering::item_x_sp_to_time(transform, 112'641));
}

TEST(RenderContractVectors, RCTV_HIT_001_StaleRequestsFailAndResultsSortStably)
{
    SCOPED_TRACE("RCTV-HIT-001");
    const auto snapshot = make_snapshot();
    const rendering::CoordinateTransform transform{snapshot->id(),
                                                   7,
                                                   core::TimeRange{0, 100'000'000},
                                                   rendering::ItemRectSp{0, 0, 102'400, 51'200}};
    rendering::HitTestRequest request;
    request.snapshot_id = snapshot->id();
    request.timeline_revision = 7;
    request.item_x_sp = 20'000;
    request.item_y_sp = 10'000;
    request.radius_sp = 2'048;
    EXPECT_TRUE(rendering::validate_hit_test_request(request, *snapshot, transform));
    const auto valid_request = request;
    request.timeline_revision = 6;
    auto stale = rendering::validate_hit_test_request(request, *snapshot, transform);
    ASSERT_FALSE(stale);
    EXPECT_EQ(stale.error().code, core::ErrorCode::stale_revision);

    rendering::HitTestResult result;
    result.snapshot_id = snapshot->id();
    result.timeline_revision = 7;
    result.candidates = {
        {rendering::HitTargetKind::series_sample,
         rendering::RenderTargetId{"series-hit"},
         20'000'000,
         4,
         1},
        {rendering::HitTargetKind::event,
         rendering::RenderTargetId{"event-hit"},
         10'000'000,
         9,
         2},
        {rendering::HitTargetKind::event,
         rendering::RenderTargetId{"event-near"},
         11'000'000,
         1,
         2},
    };
    auto sorted = rendering::validate_and_sort_hit_test_result(
        result, valid_request, *snapshot);
    ASSERT_TRUE(sorted);
    EXPECT_EQ(sorted.value().candidates[0].target_id.value, "event-near");
    EXPECT_EQ(sorted.value().candidates[1].target_id.value, "event-hit");
    EXPECT_EQ(sorted.value().candidates[2].target_id.value, "series-hit");
}

TEST(RenderContractVectors, RCTV_LIFECYCLE_001_InvalidationCreatesNewDeviceGeneration)
{
    SCOPED_TRACE("RCTV-LIFECYCLE-001");
    rendering::SceneGraphLifecycle lifecycle;
    EXPECT_FALSE(lifecycle.apply(rendering::SceneGraphEvent::initialize_scene_graph));
    ASSERT_TRUE(lifecycle.apply(rendering::SceneGraphEvent::attach_window));
    auto first = lifecycle.apply(rendering::SceneGraphEvent::initialize_scene_graph);
    ASSERT_TRUE(first);
    EXPECT_EQ(first.value(),
              (rendering::SceneGraphEpoch{rendering::SceneGraphState::ready, 1}));
    auto invalidated = lifecycle.apply(rendering::SceneGraphEvent::invalidate_scene_graph);
    ASSERT_TRUE(invalidated);
    EXPECT_EQ(invalidated.value().state, rendering::SceneGraphState::invalidated);
    auto restored = lifecycle.apply(rendering::SceneGraphEvent::initialize_scene_graph);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored.value(),
              (rendering::SceneGraphEpoch{rendering::SceneGraphState::ready, 2}));
}

TEST(RenderContractVectors, RCTV_LIFECYCLE_002_ExplicitCleanupPrecedesReinitialize)
{
    SCOPED_TRACE("RCTV-LIFECYCLE-002");
    rendering::SceneGraphLifecycle lifecycle;
    ASSERT_TRUE(lifecycle.apply(rendering::SceneGraphEvent::attach_window));
    ASSERT_TRUE(lifecycle.apply(rendering::SceneGraphEvent::initialize_scene_graph));
    ASSERT_TRUE(lifecycle.apply(rendering::SceneGraphEvent::request_cleanup));
    auto cleaned = lifecycle.apply(rendering::SceneGraphEvent::complete_cleanup);
    ASSERT_TRUE(cleaned);
    EXPECT_EQ(cleaned.value().state,
              rendering::SceneGraphState::awaiting_initialization);
}

TEST(RenderContractVectors, RCTV_FRAME_001_StrideTimestampAndOwnershipValidated)
{
    SCOPED_TRACE("RCTV-FRAME-001");
    const auto snapshot = make_snapshot();
    auto valid = rendering::validate_rendered_frame(make_frame(1), *snapshot);
    ASSERT_TRUE(valid);
    EXPECT_EQ(valid.value().stride_bytes, 8U);
    EXPECT_EQ(valid.value().valid_bytes, 16U);
    EXPECT_EQ(valid.value().bytes.use_count(), 1U);

    auto bad_stride = make_frame();
    bad_stride.stride_bytes = 7;
    EXPECT_FALSE(rendering::validate_rendered_frame(std::move(bad_stride), *snapshot));

    auto bad_time = make_frame(1);
    bad_time.time_ns = 33'366'666;
    auto mismatch = rendering::validate_rendered_frame(std::move(bad_time), *snapshot);
    ASSERT_FALSE(mismatch);
    EXPECT_EQ(mismatch.error().code, core::ErrorCode::timestamp_mismatch);
}

TEST(RenderContractVectors, RCTV_QUEUE_001_BackpressureCountsUntilLeaseRelease)
{
    SCOPED_TRACE("RCTV-QUEUE-001");
    rendering::BoundedFrameQueue queue{1, 16};
    EXPECT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);
    EXPECT_EQ(queue.outstanding_frames(), 1U);

    auto taken = queue.try_take();
    ASSERT_EQ(taken.status, rendering::FrameTakeStatus::frame);
    EXPECT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::would_block);
    taken.frame.reset();
    EXPECT_EQ(queue.outstanding_frames(), 0U);
    EXPECT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);
}

TEST(RenderContractVectors, RCTV_QUEUE_002_CancelPreservesHeldBytes)
{
    SCOPED_TRACE("RCTV-QUEUE-002");
    rendering::BoundedFrameQueue queue{2, 32};
    ASSERT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);
    auto taken = queue.try_take();
    ASSERT_TRUE(taken.frame);
    const auto* held_data = taken.frame->bytes.data();
    ASSERT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);

    queue.cancel();

    EXPECT_EQ(queue.state(), rendering::FrameQueueState::cancelled);
    EXPECT_EQ(queue.try_take().status, rendering::FrameTakeStatus::cancelled);
    EXPECT_NE(held_data, nullptr);
    EXPECT_EQ(taken.frame->bytes.data()[0], std::byte{0x2a});
    EXPECT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::cancelled);
}

TEST(RenderContractVectors, RCTV_QUEUE_003_DeviceLossFailsEpochAndPreservesHeldLease)
{
    SCOPED_TRACE("RCTV-QUEUE-003");
    rendering::BoundedFrameQueue queue{2, 32};
    ASSERT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);
    auto taken = queue.try_take();
    ASSERT_TRUE(taken.frame);
    ASSERT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);

    queue.fail(rendering::FrameFailure{rendering::FrameFailureCode::device_lost,
                                       1,
                                       "device-loss-1"});

    auto failed = queue.try_take();
    ASSERT_EQ(failed.status, rendering::FrameTakeStatus::failed);
    ASSERT_TRUE(failed.failure);
    EXPECT_EQ(failed.failure->code, rendering::FrameFailureCode::device_lost);
    EXPECT_EQ(failed.failure->device_generation, 1U);
    EXPECT_EQ(taken.frame->bytes.data()[0], std::byte{0x2a});
    EXPECT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::failed);
}

TEST(RenderContractVectors, RCTV_QUEUE_004_DrainEndsAfterLastLeaseRelease)
{
    SCOPED_TRACE("RCTV-QUEUE-004");
    rendering::BoundedFrameQueue queue{1, 16};
    ASSERT_EQ(queue.publish(make_frame()), rendering::FramePublishStatus::accepted);
    auto taken = queue.try_take();
    ASSERT_TRUE(taken.frame);
    queue.drain();
    EXPECT_EQ(queue.state(), rendering::FrameQueueState::draining);
    EXPECT_EQ(queue.try_take().status, rendering::FrameTakeStatus::empty);
    taken.frame.reset();
    EXPECT_EQ(queue.state(), rendering::FrameQueueState::ended);
    EXPECT_EQ(queue.try_take().status, rendering::FrameTakeStatus::ended);
}

} // namespace
