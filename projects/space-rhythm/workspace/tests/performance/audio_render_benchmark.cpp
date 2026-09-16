#include <space_rhythm/audio/render.hpp>

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace render = space_rhythm::audio::render;
using Clock = std::chrono::steady_clock;

std::vector<std::byte> read_bytes(const std::filesystem::path& path)
{
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw std::runtime_error{"fixture open failed"};
    }
    const std::vector<char> characters{std::istreambuf_iterator<char>{input},
                                       std::istreambuf_iterator<char>{}};
    std::vector<std::byte> result(characters.size());
    std::transform(characters.begin(), characters.end(), result.begin(), [](const char value) {
        return static_cast<std::byte>(static_cast<unsigned char>(value));
    });
    return result;
}

std::vector<render::Timbre> load_timbres()
{
    std::vector<render::Timbre> result;
    for (const auto& registration : render::registered_test_timbres()) {
        const auto bytes = read_bytes(std::filesystem::path{SPACE_RHYTHM_GOLDEN_AUDIO_DIR}
                                      / registration.output_file);
        auto loaded = render::load_registered_test_timbre(registration.fixture_id, bytes);
        if (!loaded) {
            throw std::runtime_error{"fixture validation failed"};
        }
        result.push_back(std::move(loaded.value()));
    }
    return result;
}

core::RhythmEvent make_event(const std::string& id,
                             const core::TimeNs time_ns,
                             const core::EventKind kind)
{
    core::RhythmEvent event;
    event.id.value = id;
    event.track_id.value = "benchmark-track";
    event.time_ns = time_ns;
    event.kind = kind;
    event.strength_ppm = core::norm_ppm_max;
    return event;
}

render::RenderParameters parameters()
{
    render::RenderParameters value;
    value.mapping = {
        {core::EventKind::onset, "AT-CLICK-001", 1'000'000, -250'000},
        {core::EventKind::beat, "AT-LOW-PULSE-001", 900'000, 0},
        {core::EventKind::manual, "AT-NOISE-HIT-001", 500'000, 250'000},
    };
    return value;
}

std::uint64_t peak_working_set_bytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)) == FALSE) {
        return 0;
    }
    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
}

double milliseconds(const Clock::duration duration)
{
    return std::chrono::duration<double, std::milli>{duration}.count();
}

} // namespace

int main(int argc, char** argv)
try {
    bool smoke = false;
    std::optional<std::filesystem::path> output_path;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--smoke") {
            smoke = true;
        } else if (argument == "--output" && index + 1 < argc) {
            output_path = std::filesystem::path{argv[++index]};
        } else {
            throw std::runtime_error{"unknown or incomplete argument"};
        }
    }
    const std::uint64_t seconds = smoke ? 2U : 120U;
    const auto frame_count = seconds * render::render_sample_rate;
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;

    render::RenderRequest request;
    request.frame_count = frame_count;
    request.parameters = parameters();
    for (std::uint64_t frame = 0, index = 0; frame < frame_count;
         frame += render::render_sample_rate / 4U, ++index) {
        const auto time_ns = static_cast<core::TimeNs>(frame)
            * 1'000'000'000LL / render::render_sample_rate;
        const auto kind = index % 4U == 0U ? core::EventKind::beat
            : (index % 4U == 2U ? core::EventKind::manual : core::EventKind::onset);
        request.events.push_back(make_event("benchmark-event-" + std::to_string(index),
                                            time_ns, kind));
    }

    const auto prepare_start = Clock::now();
    const auto prepared = mixer.prepare(std::move(request), timbres);
    const auto prepare_end = Clock::now();
    if (!prepared) {
        throw std::runtime_error{"benchmark prepare failed"};
    }
    const auto render_start = Clock::now();
    const auto pcm = mixer.render(prepared.value());
    const auto render_end = Clock::now();
    if (!pcm) {
        throw std::runtime_error{"benchmark render failed"};
    }
    const auto render_ms = milliseconds(render_end - render_start);
    const auto throughput_frames_per_second = static_cast<double>(frame_count)
        / (render_ms / 1'000.0);

    render::RenderRequest cancellation_request;
    cancellation_request.frame_count = 4'800;
    cancellation_request.parameters = parameters();
    cancellation_request.parameters.mapping = {
        {core::EventKind::beat, "AT-LOW-PULSE-001", 1'000'000, 0},
    };
    const std::uint64_t cancellation_events = smoke ? 2'000U : 50'000U;
    cancellation_request.events.reserve(static_cast<std::size_t>(cancellation_events));
    for (std::uint64_t index = 0; index < cancellation_events; ++index) {
        cancellation_request.events.push_back(
            make_event("cancel-event-" + std::to_string(index), 0, core::EventKind::beat));
    }
    auto cancellation_mix = mixer.prepare(std::move(cancellation_request), timbres);
    if (!cancellation_mix) {
        throw std::runtime_error{"cancellation benchmark prepare failed"};
    }
    core::CancellationToken cancellation;
    std::atomic_bool render_entered{false};
    bool cancelled_result = false;
    std::thread worker{[&] {
        render_entered.store(true, std::memory_order_release);
        const auto result = mixer.render(cancellation_mix.value(), &cancellation);
        cancelled_result = !result && result.error().code == core::ErrorCode::cancelled;
    }};
    while (!render_entered.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    const auto cancel_start = Clock::now();
    cancellation.cancel();
    worker.join();
    const auto cancel_end = Clock::now();
    if (!cancelled_result) {
        throw std::runtime_error{"cancellation was not observed"};
    }

    const auto pcm_bytes = render::pcm_f32le_bytes(pcm.value());
    std::ostringstream report;
    report << std::fixed << std::setprecision(3)
           << "{\n"
              << "  \"schemaVersion\": 1,\n"
              << "  \"mode\": \"" << (smoke ? "smoke" : "measured") << "\",\n"
              << "  \"thresholdStatus\": \"not-evaluated\",\n"
              << "  \"algorithmVersion\": \"" << render::algorithm_version << "\",\n"
              << "  \"backendVersion\": \"" << render::backend_version << "\",\n"
              << "  \"frames\": " << frame_count << ",\n"
              << "  \"events\": " << prepared.value().mapped_event_count() << ",\n"
              << "  \"prepareMs\": " << milliseconds(prepare_end - prepare_start) << ",\n"
              << "  \"renderMs\": " << render_ms << ",\n"
              << "  \"throughputFramesPerSecond\": " << throughput_frames_per_second << ",\n"
              << "  \"realtimeFactor\": "
              << throughput_frames_per_second / render::render_sample_rate << ",\n"
              << "  \"peakWorkingSetBytes\": " << peak_working_set_bytes() << ",\n"
              << "  \"cancelEvents\": " << cancellation_events << ",\n"
              << "  \"cancelLatencyMs\": " << milliseconds(cancel_end - cancel_start) << ",\n"
              << "  \"pcmSha256\": \"" << render::sha256_hex(pcm_bytes) << "\"\n"
           << "}\n";
    std::cout << report.str();
    if (output_path) {
        std::ofstream output{*output_path, std::ios::binary | std::ios::trunc};
        if (!output) {
            throw std::runtime_error{"benchmark output open failed"};
        }
        output << report.str();
        if (!output) {
            throw std::runtime_error{"benchmark output write failed"};
        }
    }
    return 0;
} catch (const std::exception& exception) {
    std::cerr << "audio render benchmark failed: " << exception.what() << '\n';
    return 1;
}
