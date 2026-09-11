#include <space_rhythm/rendering/geometry_core.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
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
constexpr core::TimeNs duration_ns = 2'000'000'000;

std::string numbered_id(std::string_view prefix, std::size_t index)
{
    std::ostringstream text;
    text << prefix << '-' << std::setw(6) << std::setfill('0') << index;
    return text.str();
}

core::EventSource user_source()
{
    core::EventSource source;
    source.origin = core::EventOrigin::user;
    source.producer_id = "space-rhythm.user";
    source.producer_version = "0.1.0";
    return source;
}

std::vector<core::RhythmEvent> make_events(std::size_t count)
{
    std::vector<core::RhythmEvent> events;
    events.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto time = static_cast<core::TimeNs>(
            static_cast<std::uint64_t>(index) * duration_ns / std::max<std::size_t>(count, 1));
        events.push_back(core::RhythmEvent{
            core::EventId{numbered_id("event", index)},
            core::TrackId{"track-0"},
            time,
            0,
            core::EventKind::beat,
            user_source(),
            static_cast<core::NormPpm>(100'000 + index % 900'001),
            std::nullopt,
            false,
            false,
            std::nullopt,
            {}});
    }
    return events;
}

std::vector<rendering::RenderSeries> make_series(std::size_t series_count,
                                                 std::size_t samples_per_series)
{
    std::vector<rendering::RenderSeries> series;
    series.reserve(series_count);
    for (std::size_t series_index = 0; series_index < series_count; ++series_index) {
        rendering::RenderSeries item;
        item.id = rendering::RenderSeriesId{numbered_id("series", series_index)};
        item.analysis_revision = core::AnalysisRevision{
            numbered_id("analysis", series_index)};
        item.definition_id = numbered_id("audio.band-energy", series_index);
        item.source_contract_version = "0.2.0";
        item.source_schema_version = 1;
        item.input_fingerprint_sha256 = std::string{digest_a};
        item.parameters_digest_sha256 = std::string{digest_b};
        item.content_digest_sha256 = std::string{digest_c};
        item.samples.reserve(samples_per_series);
        for (std::size_t sample = 0; sample < samples_per_series; ++sample) {
            const auto time = static_cast<core::TimeNs>(
                static_cast<std::uint64_t>(sample) * duration_ns
                / std::max<std::size_t>(samples_per_series, 1));
            const auto value = static_cast<core::NormPpm>(
                (sample * 7'919U + series_index * 65'537U) % 1'000'001U);
            item.samples.push_back(rendering::RenderSeriesSample{
                time,
                value,
                static_cast<core::NormPpm>(1'000'000U - value)});
        }
        series.push_back(std::move(item));
    }
    return series;
}

std::shared_ptr<const rendering::RenderSnapshot> make_snapshot(
    rendering::VisualTemplate visual_template,
    std::vector<core::RhythmEvent> events,
    std::vector<rendering::RenderSeries> series,
    std::uint64_t seed = 42,
    std::string snapshot_id = "snapshot-geometry",
    std::map<std::string, std::int64_t> parameter_overrides = {})
{
    auto parameters = rendering::make_template_parameters(
        visual_template, std::move(parameter_overrides));
    if (!parameters) {
        return {};
    }
    rendering::RenderRecipe recipe;
    recipe.recipe_id = rendering::RenderRecipeId{"recipe-geometry"};
    recipe.project_id = core::ProjectId{"project-render"};
    recipe.timeline_revision = 9;
    recipe.template_parameters = std::move(parameters.value());
    recipe.output.width_px = 640;
    recipe.output.height_px = 360;
    recipe.time_range = core::TimeRange{0, duration_ns};
    recipe.frame_rate = rendering::FrameRate{100, 1};
    recipe.deterministic_seed = seed;
    recipe.required_features = {"render.rgba8-srgb-v1", "render.snapshot-v1"};
    for (const auto& item : series) {
        recipe.feature_inputs.push_back(rendering::FeatureInputRevision{
            item.analysis_revision,
            item.source_contract_version,
            item.source_schema_version,
            item.input_fingerprint_sha256,
            item.parameters_digest_sha256,
            item.content_digest_sha256});
    }

    core::TimelineSnapshot timeline;
    timeline.project_id = recipe.project_id;
    timeline.timeline_revision = recipe.timeline_revision;
    timeline.tracks.push_back(
        core::Track{core::TrackId{"track-0"}, 0, std::string{"Main"}, {}});
    timeline.events = std::move(events);
    auto snapshot = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{std::move(snapshot_id)},
        std::move(recipe),
        std::move(timeline),
        std::move(series));
    return snapshot ? snapshot.value() : nullptr;
}

rendering::GeometryBuildRequest make_request(
    std::shared_ptr<const rendering::RenderSnapshot> snapshot,
    std::uint32_t width = 320,
    std::uint32_t height = 180,
    std::uint64_t generation = 1,
    core::TimeRange viewport = core::TimeRange{0, duration_ns})
{
    rendering::GeometryBuildRequest request;
    request.snapshot = std::move(snapshot);
    request.expected_snapshot_id = request.snapshot->id();
    request.expected_timeline_revision = request.snapshot->recipe().timeline_revision;
    request.viewport_time_range = viewport;
    request.target_width_px = width;
    request.target_height_px = height;
    request.frame_index = 50;
    request.device_generation = generation;
    return request;
}

std::size_t vertex_count(const rendering::GeometryFrame& frame,
                         rendering::GeometryBatchKind kind)
{
    std::size_t count = 0;
    for (const auto& batch : frame.batches) {
        if (batch.kind == kind) {
            count += batch.vertices.size();
        }
    }
    return count;
}

std::vector<rendering::GeometryVertex> vertices(
    const rendering::GeometryFrame& frame,
    rendering::GeometryBatchKind kind)
{
    std::vector<rendering::GeometryVertex> result;
    for (const auto& batch : frame.batches) {
        if (batch.kind == kind) {
            result.insert(result.end(), batch.vertices.begin(), batch.vertices.end());
        }
    }
    return result;
}

TEST(RenderGeometry, RGV_TEMPLATE_001_DefaultsRangesVersionsAndDigestAreStable)
{
    for (const auto kind : {rendering::VisualTemplate::waveform_oscilloscope,
                            rendering::VisualTemplate::spectrum_geometry,
                            rendering::VisualTemplate::rhythm_line_pulse}) {
        auto first = rendering::make_template_parameters(kind);
        auto second = rendering::make_template_parameters(kind);
        ASSERT_TRUE(first);
        ASSERT_TRUE(second);
        EXPECT_EQ(first.value(), second.value());
        EXPECT_EQ(first.value().template_version, "1.0.0");
        EXPECT_EQ(first.value().parameters_digest_sha256.size(), 64U);
        EXPECT_TRUE(rendering::validate_template_parameters(first.value()));
        if (kind == rendering::VisualTemplate::waveform_oscilloscope) {
            EXPECT_EQ(first.value().parameters_digest_sha256,
                      "a3855a14e6a80ba5d66b75a5063995c3a49497ba2f0a7ddec78d08a63fb6bf9a");
        }

        auto tampered = first.value();
        tampered.integer_parameters.begin()->second += 1;
        EXPECT_FALSE(rendering::validate_template_parameters(tampered));

        auto old_version = first.value();
        old_version.template_version = "0.9.0";
        old_version.parameters_digest_sha256 =
            rendering::template_parameters_sha256(old_version);
        EXPECT_FALSE(rendering::validate_template_parameters(old_version));
    }
    EXPECT_FALSE(rendering::make_template_parameters(
        rendering::VisualTemplate::rhythm_line_pulse,
        {{"particles-per-event", 33}}));
    EXPECT_FALSE(rendering::make_template_parameters(
        rendering::VisualTemplate::waveform_oscilloscope,
        {{"unknown-parameter", 1}}));
}

TEST(RenderGeometry, RGV_EMPTY_001_EmptySnapshotProducesNoGeometry)
{
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope, {}, {});
    ASSERT_NE(snapshot, nullptr);

    auto frame = rendering::build_geometry_frame(make_request(snapshot));

    ASSERT_TRUE(frame);
    EXPECT_TRUE(frame.value().batches.empty());
    EXPECT_EQ(frame.value().stats.input_event_count, 0U);
    EXPECT_EQ(frame.value().stats.input_sample_count, 0U);
}

TEST(RenderGeometry, RGV_CLIP_001_ViewportUsesHalfOpenBoundaries)
{
    auto events = make_events(4);
    auto series = make_series(1, 4);
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope,
        std::move(events),
        std::move(series));
    ASSERT_NE(snapshot, nullptr);
    const core::TimeRange viewport{500'000'000, 1'500'000'000};

    auto frame = rendering::build_geometry_frame(
        make_request(snapshot, 100, 100, 1, viewport));

    ASSERT_TRUE(frame);
    EXPECT_EQ(frame.value().stats.visible_event_count, 2U);
    EXPECT_EQ(frame.value().stats.visible_sample_count, 2U);
    for (const auto& batch : frame.value().batches) {
        for (const auto& point : batch.vertices) {
            EXPECT_GE(point.x, 0.0F);
            EXPECT_LE(point.x, 100.0F);
            EXPECT_GE(point.y, 0.0F);
            EXPECT_LE(point.y, 100.0F);
        }
    }
}

TEST(RenderGeometry, RGV_DENSE_001_FiftyThousandEventsCollapseToPixelLod)
{
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope,
        make_events(50'000),
        {},
        42,
        "snapshot-dense-events",
        {{"max-total-vertices", 6'000}, {"max-vertices-per-batch", 600}});
    ASSERT_NE(snapshot, nullptr);

    auto frame = rendering::build_geometry_frame(make_request(snapshot, 64, 64));

    ASSERT_TRUE(frame);
    EXPECT_EQ(frame.value().stats.input_event_count, 50'000U);
    EXPECT_EQ(frame.value().stats.visible_event_count, 50'000U);
    EXPECT_GT(frame.value().stats.lod_level, 1U);
    EXPECT_LE(vertex_count(frame.value(), rendering::GeometryBatchKind::event_timeline),
              64U * 6U);
    for (const auto& batch : frame.value().batches) {
        EXPECT_LE(batch.vertices.size(), 600U);
    }
}

TEST(RenderGeometry, RGV_DENSE_002_HundredThousandSamplesUseMinMaxLod)
{
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope,
        {},
        make_series(1, 100'000),
        42,
        "snapshot-dense-wave");
    ASSERT_NE(snapshot, nullptr);

    auto frame = rendering::build_geometry_frame(make_request(snapshot, 128, 96));

    ASSERT_TRUE(frame);
    EXPECT_EQ(frame.value().stats.input_sample_count, 100'000U);
    EXPECT_EQ(frame.value().stats.visible_sample_count, 100'000U);
    EXPECT_GT(frame.value().stats.lod_level, 1U);
    EXPECT_LE(vertex_count(frame.value(), rendering::GeometryBatchKind::waveform),
              128U * 6U);
}

TEST(RenderGeometry, RGV_TEMPLATES_001_AllThreeTemplatesProduceBatchedTriangles)
{
    const auto events = make_events(8);
    const auto series = make_series(16, 80);
    for (const auto [kind, expected_batch] : {
             std::pair{rendering::VisualTemplate::waveform_oscilloscope,
                       rendering::GeometryBatchKind::waveform},
             std::pair{rendering::VisualTemplate::spectrum_geometry,
                       rendering::GeometryBatchKind::spectrum},
             std::pair{rendering::VisualTemplate::rhythm_line_pulse,
                       rendering::GeometryBatchKind::rhythm_line_pulse}}) {
        const auto snapshot = make_snapshot(kind, events, series);
        ASSERT_NE(snapshot, nullptr);
        auto frame = rendering::build_geometry_frame(make_request(snapshot));
        ASSERT_TRUE(frame);
        EXPECT_GT(vertex_count(frame.value(), expected_batch), 0U);
        for (const auto& batch : frame.value().batches) {
            EXPECT_EQ(batch.vertices.size() % 6U, 0U);
        }
    }
}

TEST(RenderGeometry, RGV_SEED_001_SameRequestIsSharedAndDeterministic)
{
    auto events = make_events(8);
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::rhythm_line_pulse, events, {});
    ASSERT_NE(snapshot, nullptr);
    const auto request = make_request(snapshot);

    auto onscreen_input = rendering::build_geometry_frame(request);
    auto offscreen_input = rendering::build_geometry_frame(request);

    ASSERT_TRUE(onscreen_input);
    ASSERT_TRUE(offscreen_input);
    EXPECT_EQ(onscreen_input.value(), offscreen_input.value());

    const auto other_seed_snapshot = make_snapshot(
        rendering::VisualTemplate::rhythm_line_pulse,
        std::move(events),
        {},
        43,
        "snapshot-other-seed");
    ASSERT_NE(other_seed_snapshot, nullptr);
    auto other_seed = rendering::build_geometry_frame(
        make_request(other_seed_snapshot));
    ASSERT_TRUE(other_seed);
    EXPECT_NE(vertices(onscreen_input.value(),
                       rendering::GeometryBatchKind::rhythm_line_pulse),
              vertices(other_seed.value(),
                       rendering::GeometryBatchKind::rhythm_line_pulse));
}

TEST(RenderGeometry, RGV_STALE_001_OldSnapshotIsRejected)
{
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope, {}, {});
    ASSERT_NE(snapshot, nullptr);
    auto request = make_request(snapshot);
    request.expected_snapshot_id = rendering::RenderSnapshotId{"snapshot-newer"};

    auto frame = rendering::build_geometry_frame(request);

    ASSERT_FALSE(frame);
    EXPECT_EQ(frame.error().code, core::ErrorCode::stale_revision);
}

TEST(RenderGeometry, RGV_GENERATION_001_UpdatePlanReusesAndRebuildsByGeneration)
{
    const auto snapshot = make_snapshot(
        rendering::VisualTemplate::waveform_oscilloscope,
        make_events(8),
        make_series(1, 256));
    ASSERT_NE(snapshot, nullptr);
    auto generation_one = rendering::build_geometry_frame(make_request(snapshot, 128, 96, 1));
    ASSERT_TRUE(generation_one);

    auto first = rendering::plan_geometry_batch_update(
        std::nullopt, generation_one.value());
    ASSERT_TRUE(first);
    EXPECT_FALSE(first.value().rebuild_for_device_generation);
    EXPECT_EQ(first.value().create_batch_count,
              generation_one.value().batches.size());
    EXPECT_EQ(first.value().reuse_batch_count, 0U);

    auto reuse = rendering::plan_geometry_batch_update(
        first.value().next_state, generation_one.value());
    ASSERT_TRUE(reuse);
    EXPECT_FALSE(reuse.value().rebuild_for_device_generation);
    EXPECT_EQ(reuse.value().reuse_batch_count,
              generation_one.value().batches.size());
    EXPECT_EQ(reuse.value().create_batch_count, 0U);

    auto generation_two_request = make_request(snapshot, 128, 96, 2);
    auto generation_two = rendering::build_geometry_frame(generation_two_request);
    ASSERT_TRUE(generation_two);
    auto rebuild = rendering::plan_geometry_batch_update(
        reuse.value().next_state, generation_two.value());
    ASSERT_TRUE(rebuild);
    EXPECT_TRUE(rebuild.value().rebuild_for_device_generation);
    EXPECT_EQ(rebuild.value().remove_batch_count,
              generation_one.value().batches.size());
    EXPECT_EQ(rebuild.value().create_batch_count,
              generation_two.value().batches.size());
}

} // namespace
