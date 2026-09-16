#include <space_rhythm/rendering/offscreen_renderer.hpp>

#include <QGuiApplication>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace rendering = space_rhythm::rendering;

constexpr core::TimeNs duration_ns = 2'000'000'000;
constexpr std::string_view digest_a{
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
constexpr std::string_view digest_b{
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"};
constexpr std::string_view digest_c{
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"};

std::string numbered_id(std::string_view prefix, std::size_t index)
{
    std::ostringstream text;
    text << prefix << '-' << std::setw(6) << std::setfill('0') << index;
    return text.str();
}

core::EventSource source()
{
    return core::EventSource{core::EventOrigin::user,
                             "space-rhythm.test",
                             "0.1.0",
                             std::nullopt,
                             std::nullopt,
                             std::nullopt,
                             {},
                             {}};
}

std::vector<core::RhythmEvent> events(std::size_t count)
{
    std::vector<core::RhythmEvent> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(core::RhythmEvent{
            core::EventId{numbered_id("event", index)},
            core::TrackId{"track-0"},
            static_cast<core::TimeNs>(index * static_cast<std::uint64_t>(duration_ns)
                                      / std::max<std::size_t>(count, 1)),
            0,
            core::EventKind::beat,
            source(),
            static_cast<core::NormPpm>(100'000 + index % 900'001),
            std::nullopt,
            false,
            false,
            std::nullopt,
            {}});
    }
    return result;
}

std::vector<rendering::RenderSeries> series(std::size_t count,
                                             std::size_t samples)
{
    std::vector<rendering::RenderSeries> result;
    for (std::size_t item_index = 0; item_index < count; ++item_index) {
        rendering::RenderSeries item;
        item.id = rendering::RenderSeriesId{numbered_id("series", item_index)};
        item.analysis_revision = core::AnalysisRevision{
            numbered_id("analysis", item_index)};
        item.definition_id = "audio.band-energy";
        item.source_contract_version = "0.2.0";
        item.source_schema_version = 1;
        item.input_fingerprint_sha256 = std::string{digest_a};
        item.parameters_digest_sha256 = std::string{digest_b};
        item.content_digest_sha256 = std::string{digest_c};
        for (std::size_t sample = 0; sample < samples; ++sample) {
            const auto value = static_cast<core::NormPpm>(
                (sample * 7'919U + item_index * 65'537U) % 1'000'001U);
            item.samples.push_back(rendering::RenderSeriesSample{
                static_cast<core::TimeNs>(
                    sample * static_cast<std::uint64_t>(duration_ns)
                    / std::max<std::size_t>(samples, 1)),
                value,
                static_cast<core::NormPpm>(1'000'000U - value)});
        }
        result.push_back(std::move(item));
    }
    return result;
}

std::shared_ptr<const rendering::RenderSnapshot> snapshot(
    rendering::VisualTemplate visual_template,
    std::uint32_t width,
    std::uint32_t height,
    std::size_t event_count = 8,
    std::size_t series_count = 8,
    std::size_t sample_count = 128,
    std::map<std::string, std::int64_t> overrides = {})
{
    auto parameters = rendering::make_template_parameters(
        visual_template, std::move(overrides));
    if (!parameters) {
        return {};
    }
    auto input_series = series(series_count, sample_count);
    rendering::RenderRecipe recipe;
    recipe.recipe_id = rendering::RenderRecipeId{
        "recipe-offscreen-" + std::to_string(width) + 'x' + std::to_string(height)};
    recipe.project_id = core::ProjectId{"project-render"};
    recipe.timeline_revision = 35;
    recipe.template_parameters = std::move(parameters.value());
    recipe.output.width_px = width;
    recipe.output.height_px = height;
    recipe.time_range = {0, duration_ns};
    recipe.frame_rate = {100, 1};
    recipe.deterministic_seed = 0x350'340'330ULL;
    recipe.required_features = {"render.rgba8-srgb-v1", "render.snapshot-v1"};
    for (const auto& item : input_series) {
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
    timeline.events = events(event_count);
    auto made = rendering::make_render_snapshot(
        rendering::RenderSnapshotId{
            "snapshot-offscreen-" + std::to_string(width) + 'x'
            + std::to_string(height) + '-'
            + std::to_string(static_cast<int>(visual_template))},
        std::move(recipe),
        std::move(timeline),
        std::move(input_series));
    return made ? made.value() : nullptr;
}

rendering::OffscreenSessionConfig config(
    std::shared_ptr<const rendering::RenderSnapshot> input,
    rendering::BackendPreference preference =
        rendering::BackendPreference::software_only,
    std::size_t maximum_frames = 2)
{
    rendering::OffscreenSessionConfig result;
    result.snapshot = std::move(input);
    result.expected_snapshot_id = result.snapshot->id();
    result.expected_timeline_revision = result.snapshot->recipe().timeline_revision;
    result.viewport_time_range = result.snapshot->recipe().time_range;
    result.backend_preference = preference;
    result.max_outstanding_frames = maximum_frames;
    return result;
}

void print_measurements(std::string_view case_id,
                        const rendering::OffscreenRenderResult& result)
{
    const auto& metrics = result.measurements;
    std::cout << "[measured/not-evaluated] case=" << case_id
              << " total_frame_ns=" << metrics.total_frame_time_ns.value
              << " process_cpu_ns=" << metrics.process_cpu_time_ns.value
              << " geometry_ns=" << metrics.geometry_build_time_ns.value
              << " render_readback_ns="
              << metrics.polish_sync_render_readback_time_ns.value
              << " uploaded_vertices=" << metrics.uploaded_vertices.value
              << " uploaded_bytes=" << metrics.uploaded_bytes.value
              << " working_set_bytes=" << metrics.process_working_set_bytes.value
              << " gpu_frame_time=unavailable"
              << " gpu_memory_usage=unavailable\n";
}

TEST(OffscreenContract, CapabilitySelectionCoversDefaultGpuAndSoftwareFallback)
{
    auto report = rendering::probe_graphics_capabilities();
    ASSERT_EQ(report.capabilities.size(), 2U);
    auto actual = rendering::select_graphics_backend(
        rendering::BackendPreference::default_gpu_with_software_fallback, report);
    ASSERT_TRUE(actual);

    for (auto& capability : report.capabilities) {
        if (capability.backend == rendering::GraphicsBackend::default_gpu_d3d11) {
            capability.availability = rendering::CapabilityAvailability::unavailable;
            capability.detail = "forced test vector";
        }
    }
    auto fallback = rendering::select_graphics_backend(
        rendering::BackendPreference::default_gpu_with_software_fallback, report);
    ASSERT_TRUE(fallback);
    EXPECT_EQ(fallback.value().backend, rendering::GraphicsBackend::qt_software);
    EXPECT_TRUE(fallback.value().used_fallback);
    EXPECT_FALSE(rendering::select_graphics_backend(
        rendering::BackendPreference::default_gpu_only, report));
}

TEST(OffscreenSoftware, FrameContractMetricsAndLeaseBackpressure)
{
    const auto input = snapshot(rendering::VisualTemplate::waveform_oscilloscope,
                                160,
                                90);
    ASSERT_NE(input, nullptr);
    auto session_result = rendering::OffscreenRenderSession::create(
        config(input, rendering::BackendPreference::software_only, 1));
    ASSERT_TRUE(session_result) << session_result.error().diagnostic_id;
    auto session = std::move(session_result.value());

    auto first = session->render_frame(50);
    ASSERT_TRUE(first) << first.error().diagnostic_id;
    EXPECT_EQ(first.value().publish_status,
              rendering::FramePublishStatus::accepted);
    EXPECT_EQ(first.value().measurements.evaluation,
              rendering::MeasurementEvaluation::measured_not_evaluated);
    EXPECT_EQ(first.value().measurements.uploaded_vertices.availability,
              rendering::MetricAvailability::measured);
    EXPECT_EQ(first.value().measurements.gpu_frame_time_ns.availability,
              rendering::MetricAvailability::unavailable);
    EXPECT_EQ(first.value().measurements.gpu_memory_usage_bytes.availability,
              rendering::MetricAvailability::unavailable);
    print_measurements("software-waveform-160x90", first.value());

    auto taken = session->try_take();
    ASSERT_EQ(taken.status, rendering::FrameTakeStatus::frame);
    ASSERT_TRUE(taken.frame);
    EXPECT_EQ(taken.frame->width_px, 160U);
    EXPECT_EQ(taken.frame->height_px, 90U);
    EXPECT_EQ(taken.frame->stride_bytes, 640U);
    EXPECT_EQ(taken.frame->row_order, rendering::RowOrder::top_down);
    EXPECT_EQ(taken.frame->pixel_format, rendering::PixelFormat::rgba8_unorm);
    EXPECT_EQ(taken.frame->alpha_mode, rendering::AlphaMode::straight);

    auto second = session->render_frame(51);
    ASSERT_TRUE(second);
    EXPECT_EQ(second.value().publish_status,
              rendering::FramePublishStatus::would_block);
}

TEST(OffscreenSoftware, CancellationAndOldGenerationTerminationRebuild)
{
    const auto input = snapshot(rendering::VisualTemplate::rhythm_line_pulse,
                                128,
                                72);
    ASSERT_NE(input, nullptr);
    auto cancelled_session_result = rendering::OffscreenRenderSession::create(
        config(input));
    ASSERT_TRUE(cancelled_session_result);
    auto cancelled_session = std::move(cancelled_session_result.value());
    core::CancellationToken token;
    token.cancel();
    auto cancelled_frame = cancelled_session->render_frame(50, &token);
    ASSERT_FALSE(cancelled_frame);
    EXPECT_EQ(cancelled_frame.error().code, core::ErrorCode::cancelled);
    EXPECT_EQ(cancelled_session->queue_state(),
              rendering::FrameQueueState::cancelled);

    auto old_session_result = rendering::OffscreenRenderSession::create(config(input));
    ASSERT_TRUE(old_session_result)
        << old_session_result.error().stage << ' '
        << (old_session_result.error().context.contains("detail")
                ? old_session_result.error().context.at("detail")
                : std::string{});
    auto old_session = std::move(old_session_result.value());
    const auto old_generation = old_session->device_generation();
    old_session->notify_device_lost("T035-DEVICE-LOSS", "simulated removal");
    EXPECT_EQ(old_session->queue_state(), rendering::FrameQueueState::failed);
    ASSERT_TRUE(old_session->last_diagnostic());
    EXPECT_EQ(old_session->last_diagnostic()->device_generation, old_generation);
    EXPECT_EQ(old_session->last_diagnostic()->failure_code,
              rendering::FrameFailureCode::device_lost);
    auto failed_take = old_session->try_take();
    ASSERT_EQ(failed_take.status, rendering::FrameTakeStatus::failed);
    ASSERT_TRUE(failed_take.failure);
    EXPECT_EQ(failed_take.failure->device_generation, old_generation);
    EXPECT_FALSE(old_session->render_frame(50));

    auto rebuilt_result = old_session->rebuild();
    ASSERT_TRUE(rebuilt_result) << rebuilt_result.error().diagnostic_id;
    auto rebuilt = std::move(rebuilt_result.value());
    EXPECT_GT(rebuilt->device_generation(), old_generation);
    auto recovered_frame = rebuilt->render_frame(50);
    ASSERT_TRUE(recovered_frame) << recovered_frame.error().diagnostic_id;
    EXPECT_EQ(recovered_frame.value().publish_status,
              rendering::FramePublishStatus::accepted);
}

TEST(OffscreenSoftware, EmptyDenseAndMultipleResolutionInputs)
{
    for (const auto [width, height] : {
             std::pair{64U, 64U},
             std::pair{320U, 180U},
             std::pair{640U, 360U}}) {
        const auto empty = snapshot(rendering::VisualTemplate::waveform_oscilloscope,
                                    width,
                                    height,
                                    0,
                                    0,
                                    0);
        ASSERT_NE(empty, nullptr);
        auto session_result = rendering::OffscreenRenderSession::create(config(empty));
        ASSERT_TRUE(session_result);
        auto session = std::move(session_result.value());
        auto rendered = session->render_frame(0);
        ASSERT_TRUE(rendered);
        print_measurements("software-empty-" + std::to_string(width) + 'x'
                               + std::to_string(height),
                           rendered.value());
        EXPECT_EQ(rendered.value().geometry_stats.input_event_count, 0U);
        EXPECT_EQ(rendered.value().geometry_stats.input_sample_count, 0U);
        auto taken = session->try_take();
        ASSERT_EQ(taken.status, rendering::FrameTakeStatus::frame);
        EXPECT_EQ(taken.frame->valid_bytes,
                  static_cast<std::uint64_t>(width) * height * 4U);
    }

    const auto dense = snapshot(rendering::VisualTemplate::waveform_oscilloscope,
                                128,
                                96,
                                50'000,
                                1,
                                100'000,
                                {{"max-total-vertices", 60'000},
                                 {"max-vertices-per-batch", 6'000}});
    ASSERT_NE(dense, nullptr);
    auto dense_session_result = rendering::OffscreenRenderSession::create(config(dense));
    ASSERT_TRUE(dense_session_result);
    auto dense_session = std::move(dense_session_result.value());
    auto dense_frame = dense_session->render_frame(50);
    ASSERT_TRUE(dense_frame);
    print_measurements("software-dense-128x96", dense_frame.value());
    EXPECT_EQ(dense_frame.value().geometry_stats.input_event_count, 50'000U);
    EXPECT_EQ(dense_frame.value().geometry_stats.input_sample_count, 100'000U);
    EXPECT_GT(dense_frame.value().geometry_stats.lod_level, 1U);
}

TEST(OffscreenSoftware, ThreeTemplatesMeasureOnscreenOffscreenPixelDifference)
{
    for (const auto kind : {rendering::VisualTemplate::waveform_oscilloscope,
                            rendering::VisualTemplate::spectrum_geometry,
                            rendering::VisualTemplate::rhythm_line_pulse}) {
        const auto input = snapshot(kind, 192, 108, 12, 16, 256);
        ASSERT_NE(input, nullptr);
        auto session_result = rendering::OffscreenRenderSession::create(config(input));
        ASSERT_TRUE(session_result);
        auto session = std::move(session_result.value());
        auto rendered = session->render_frame(50);
        ASSERT_TRUE(rendered);
        print_measurements(
            input->recipe().template_parameters.template_id,
            rendered.value());
        auto candidate = session->try_take();
        ASSERT_EQ(candidate.status, rendering::FrameTakeStatus::frame);
        ASSERT_TRUE(candidate.frame);
        auto reference = rendering::render_onscreen_reference(
            input,
            input->recipe().time_range,
            50,
            session->device_generation(),
            session->backend());
        ASSERT_TRUE(reference)
            << reference.error().diagnostic_id << ' ' << reference.error().stage
            << ' '
            << (reference.error().context.contains("detail")
                    ? reference.error().context.at("detail")
                    : std::string{});
        EXPECT_EQ(reference.value().snapshot_id, candidate.frame->snapshot_id);
        EXPECT_EQ(reference.value().time_ns, candidate.frame->time_ns);
        auto difference = rendering::measure_pixel_difference(
            reference.value(), *candidate.frame);
        ASSERT_TRUE(difference);
        EXPECT_EQ(difference.value().evaluation,
                  rendering::MeasurementEvaluation::measured_not_evaluated);
        EXPECT_EQ(difference.value().compared_pixel_count, 192U * 108U);
        std::cout << "[measured/not-evaluated] template="
                  << input->recipe().template_parameters.template_id
                  << " reference_sha256=" << difference.value().reference_sha256
                  << " offscreen_sha256=" << difference.value().candidate_sha256
                  << " max_difference="
                  << static_cast<unsigned int>(
                         difference.value().maximum_channel_difference)
                  << " different_pixels="
                  << difference.value().different_pixel_count << '\n';
    }
}

TEST(OffscreenContract, OldSnapshotAndPixelLayoutMismatchAreRejected)
{
    const auto input = snapshot(rendering::VisualTemplate::waveform_oscilloscope,
                                64,
                                64);
    ASSERT_NE(input, nullptr);
    auto stale = config(input);
    stale.expected_snapshot_id = rendering::RenderSnapshotId{"newer-snapshot"};
    EXPECT_FALSE(rendering::OffscreenRenderSession::create(std::move(stale)));

    rendering::RenderedFrame first;
    first.width_px = first.height_px = 1;
    first.stride_bytes = first.valid_bytes = 4;
    first.bytes = rendering::FrameLease{std::vector<std::byte>(4)};
    auto second = first;
    second.width_px = 2;
    EXPECT_FALSE(rendering::measure_pixel_difference(first, second));
}

TEST(OffscreenDefaultGpu, PublicD3D11RenderControlPath)
{
    const auto capability = rendering::probe_graphics_capabilities();
    const auto selected = rendering::select_graphics_backend(
        rendering::BackendPreference::default_gpu_only, capability);
    if (!selected) {
        GTEST_SKIP() << "Default D3D11 GPU unavailable: "
                     << selected.error().diagnostic_id;
    }
    for (const auto& item : capability.capabilities) {
        if (item.backend == rendering::GraphicsBackend::default_gpu_d3d11) {
            std::cout << "[capability] api=" << item.graphics_api
                      << " adapter=" << item.adapter_name
                      << " vendor_id=" << item.vendor_id
                      << " device_id=" << item.device_id
                      << " dedicated_capacity_bytes="
                      << item.dedicated_video_memory_capacity_bytes.value_or(0)
                      << " gpu_memory_usage=unavailable\n";
        }
    }
    for (const auto kind : {rendering::VisualTemplate::waveform_oscilloscope,
                            rendering::VisualTemplate::spectrum_geometry,
                            rendering::VisualTemplate::rhythm_line_pulse}) {
        const auto input = snapshot(kind, 192, 108, 12, 16, 256);
        ASSERT_NE(input, nullptr);
        auto reference = rendering::render_onscreen_reference(
            input,
            input->recipe().time_range,
            50,
            1U + static_cast<std::uint64_t>(kind),
            rendering::GraphicsBackend::default_gpu_d3d11);
        ASSERT_TRUE(reference)
            << reference.error().diagnostic_id << ' ' << reference.error().stage
            << ' '
            << (reference.error().context.contains("detail")
                    ? reference.error().context.at("detail")
                    : std::string{});
        auto session_result = rendering::OffscreenRenderSession::create(config(
            input, rendering::BackendPreference::default_gpu_only));
        ASSERT_TRUE(session_result)
            << session_result.error().diagnostic_id << ' '
            << session_result.error().stage << ' '
            << (session_result.error().context.contains("detail")
                    ? session_result.error().context.at("detail")
                    : std::string{});
        auto session = std::move(session_result.value());
        EXPECT_EQ(session->backend(),
                  rendering::GraphicsBackend::default_gpu_d3d11);
        auto rendered = session->render_frame(50);
        ASSERT_TRUE(rendered) << rendered.error().diagnostic_id;
        print_measurements(
            "default-gpu-"
                + input->recipe().template_parameters.template_id,
            rendered.value());
        EXPECT_EQ(rendered.value().publish_status,
                  rendering::FramePublishStatus::accepted);
        auto taken = session->try_take();
        ASSERT_EQ(taken.status, rendering::FrameTakeStatus::frame);
        ASSERT_TRUE(taken.frame);
        EXPECT_EQ(taken.frame->valid_bytes, 192U * 108U * 4U);

        auto difference = rendering::measure_pixel_difference(
            reference.value(), *taken.frame);
        ASSERT_TRUE(difference);
        std::cout << "[measured/not-evaluated] backend=default-gpu template="
                  << input->recipe().template_parameters.template_id
                  << " reference_sha256=" << difference.value().reference_sha256
                  << " offscreen_sha256=" << difference.value().candidate_sha256
                  << " max_difference="
                  << static_cast<unsigned int>(
                         difference.value().maximum_channel_difference)
                  << " different_pixels="
                  << difference.value().different_pixel_count << '\n';
    }
}

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
