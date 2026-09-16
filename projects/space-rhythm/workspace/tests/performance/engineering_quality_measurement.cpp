#include <space_rhythm/media/playback_export.hpp>
#include <space_rhythm/system/runtime.hpp>

#include <windows.h>
#include <psapi.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace playback = space_rhythm::media::playback_export;
namespace rendering = space_rhythm::rendering;
namespace system = space_rhythm::system;
using Clock = std::chrono::steady_clock;

[[nodiscard]] std::uint64_t elapsed_us(const Clock::time_point start,
                                       const Clock::time_point finish)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count());
}

[[nodiscard]] std::uint64_t peak_working_set_bytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)) == FALSE) {
        throw std::runtime_error{"GetProcessMemoryInfo failed"};
    }
    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
}

[[nodiscard]] system::ProjectDocument project(core::TimelineRevision revision)
{
    system::ProjectDocument document;
    document.app_version = "0.1.0";
    document.timeline.project_id = core::ProjectId{"t022-recovery-measurement"};
    document.timeline.timeline_revision = revision;
    document.timeline.tracks.push_back(
        core::Track{core::TrackId{"track-0"}, 0, std::string{"Main"}, {}});
    return document;
}

void write_corrupt(const std::filesystem::path& path)
{
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    output << "{corrupt";
    if (!output) {
        throw std::runtime_error{"cannot write corrupt recovery fixture"};
    }
}

} // namespace

int main(const int argc, const char* const argv[])
try {
    if (argc != 3 || std::string_view{argv[1]} != "--output") {
        std::cerr << "usage: space_rhythm_engineering_quality_measurement --output <json>\n";
        return 2;
    }
    const std::filesystem::path output_path{argv[2]};
    const auto work_root = output_path.parent_path() / "engineering-quality-work";
    std::error_code ignored;
    std::filesystem::remove_all(work_root, ignored);
    std::filesystem::create_directories(work_root);
    std::cerr << "T022_MEASUREMENT_STAGE=sync\n";

    const auto monotonic_start = playback::PreviewSynchronizer::MonotonicTimePoint{};
    playback::PreviewConfiguration configuration;
    configuration.timeline_range = {0, 500'000'000};
    configuration.audio_sample_rate = 48'000;
    configuration.initial_audio_played_frames = 1'000;
    configuration.drift_warning_threshold_ns = 20'000'000;
    configuration.video_frames = {
        {0, 0}, {1, 40'000'000}, {2, 100'000'000}, {3, 180'000'000}, {4, 260'000'000}};
    auto synchronizer_result = playback::PreviewSynchronizer::create(
        configuration, monotonic_start);
    if (!synchronizer_result) {
        throw std::runtime_error{"preview synchronizer creation failed"};
    }
    auto synchronizer = std::move(synchronizer_result.value());
    const auto seek_start = Clock::now();
    auto seek = synchronizer->seek(200'000'000, monotonic_start, 0);
    const auto seek_latency_us = elapsed_us(seek_start, Clock::now());
    if (!seek || seek.value().playhead_time_ns != 200'000'000
        || !seek.value().video_frame || seek.value().video_frame->time_ns != 180'000'000) {
        throw std::runtime_error{"public seek contract did not select the expected VFR frame"};
    }
    if (!synchronizer->set_audio_played_frames(2'400)) {
        throw std::runtime_error{"audio sample clock update failed"};
    }
    auto drift = synchronizer->tick(monotonic_start);
    if (!drift || drift.value().playhead_time_ns != 250'000'000
        || drift.value().video_lateness_ns != 70'000'000) {
        throw std::runtime_error{"public A/V drift contract produced unexpected values"};
    }

    const std::vector<core::TimeNs> preview_events{
        100'000'000, 205'000'000, 300'000'000};
    const std::vector<core::TimeNs> export_events{
        100'000'000, 200'000'000, 333'000'000};
    auto alignment = playback::compare_event_positions(
        preview_events, export_events, rendering::FrameRate{30, 1});
    if (!alignment || !alignment.value().within_one_output_frame) {
        throw std::runtime_error{"public preview/export alignment contract failed"};
    }

    std::cerr << "T022_MEASUREMENT_STAGE=recovery-prepare\n";
    const auto primary = work_root / "project.srp.json";
    const system::ProjectStore store;
    if (!store.save(primary, project(1)) || !store.save_autosave(primary, project(2))
        || !store.mark_session_clean(primary, false)) {
        throw std::runtime_error{"cannot prepare autosave recovery measurement"};
    }
    std::cerr << "T022_MEASUREMENT_STAGE=recovery-autosave\n";
    const auto autosave_start = Clock::now();
    auto autosave_recovery = store.recover(primary);
    const auto autosave_recovery_us = elapsed_us(autosave_start, Clock::now());
    if (!autosave_recovery
        || autosave_recovery.value().source != system::RecoverySource::autosave
        || autosave_recovery.value().project.document.timeline.timeline_revision != 2U) {
        throw std::runtime_error{"autosave recovery contract failed"};
    }

    std::cerr << "T022_MEASUREMENT_STAGE=recovery-fallback\n";
    write_corrupt(system::ProjectStore::autosave_path(primary));
    const auto fallback_start = Clock::now();
    auto primary_fallback = store.recover(primary);
    const auto primary_fallback_us = elapsed_us(fallback_start, Clock::now());
    if (!primary_fallback
        || primary_fallback.value().source != system::RecoverySource::primary
        || primary_fallback.value().project.document.timeline.timeline_revision != 1U
        || !primary_fallback.value().ignored_recovery_error) {
        throw std::runtime_error{"corrupt autosave fallback contract failed"};
    }

    std::cerr << "T022_MEASUREMENT_STAGE=write-output\n";
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        throw std::runtime_error{"cannot create engineering measurement output"};
    }
    output << "{\n"
           << "  \"schemaVersion\": 1,\n"
           << "  \"taskId\": \"T-022\",\n"
           << "  \"status\": \"measured\",\n"
           << "  \"thresholdEvaluation\": \"not-evaluated\",\n"
           << "  \"productEffectEvaluation\": "
              "\"not-evaluated(deferred-to-personal-use-feedback)\",\n"
           << "  \"naturalnessEvaluation\": "
              "\"not-evaluated(deferred-to-personal-use-feedback)\",\n"
           << "  \"realVfrEvaluation\": "
              "\"not-evaluated(deferred-to-personal-use-feedback)\",\n"
           << "  \"formalProductPerformanceEvaluation\": "
              "\"not-evaluated(deferred-to-personal-use-feedback)\",\n"
           << "  \"seek\": {\"requestedTimeNs\": 200000000, "
              "\"selectedFrameTimeNs\": " << seek.value().video_frame->time_ns
           << ", \"selectionErrorNs\": "
           << 200'000'000 - seek.value().video_frame->time_ns
           << ", \"callLatencyUs\": " << seek_latency_us << "},\n"
           << "  \"avDrift\": {\"audioPlayheadTimeNs\": "
           << drift.value().playhead_time_ns << ", \"selectedVideoTimeNs\": "
           << seek.value().video_frame->time_ns << ", \"videoLatenessNs\": "
           << drift.value().video_lateness_ns << "},\n"
           << "  \"previewExportAlignment\": {\"oneOutputFrameNs\": "
           << alignment.value().one_output_frame_ns << ", \"maximumErrorNs\": "
           << alignment.value().maximum_error_ns
           << ", \"withinOneOutputFrame\": true},\n"
           << "  \"recovery\": {\"autosaveSource\": \"autosave\", "
              "\"autosaveRevision\": 2, \"autosaveRecoveryUs\": "
           << autosave_recovery_us << ", \"corruptAutosaveFallbackSource\": "
              "\"primary\", \"primaryRevision\": 1, \"primaryFallbackUs\": "
           << primary_fallback_us << ", \"corruptAutosaveDiagnosticPresent\": true},\n"
           << "  \"peakWorkingSetBytes\": " << peak_working_set_bytes() << "\n"
           << "}\n";
    if (!output) {
        throw std::runtime_error{"cannot write engineering measurement output"};
    }
    std::cout << "T022_ENGINEERING_MEASUREMENT=PASS output=" << output_path.string()
              << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << "T022_ENGINEERING_MEASUREMENT=FAIL reason=" << error.what() << '\n';
    return 1;
}
