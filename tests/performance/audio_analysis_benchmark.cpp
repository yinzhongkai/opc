#include <space_rhythm/audio/analysis.hpp>

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using space_rhythm::audio::AnalysisResult;
using space_rhythm::audio::AnalysisStatus;
using space_rhythm::audio::DspPcmBuffer;
using space_rhythm::audio::PcmNarrowAdapter;
using space_rhythm::media::BufferLease;
using space_rhythm::media::PcmBuffer;
using space_rhythm::media::PlaneView;
using space_rhythm::media::StreamKey;

constexpr std::uint32_t kSampleRate = 48'000;
constexpr std::uint64_t kFixtureFrames = 96'000;
constexpr std::string_view kFixtureHash{
    "c44e3d6c621a7bd8c3d481e941688cb4a31a036a6c915b376594a616b23dcf0a"};

[[nodiscard]] std::vector<std::byte> read_bytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error{"cannot open benchmark fixture: " + path.string()};
    }
    const auto size = input.tellg();
    if (size < 0) {
        throw std::runtime_error{"cannot size benchmark fixture"};
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        throw std::runtime_error{"cannot read benchmark fixture"};
    }
    return bytes;
}

[[nodiscard]] DspPcmBuffer adapt(std::vector<std::byte> bytes,
                                 const std::uint64_t frames,
                                 const std::string_view hash,
                                 std::string segment_id)
{
    PcmBuffer pcm;
    pcm.stream_key = StreamKey{std::string{hash}, 0};
    pcm.time_ns = 0;
    const auto duration = space_rhythm::core::scale_ticks(
        static_cast<std::int64_t>(frames),
        {1, kSampleRate},
        space_rhythm::core::RoundingMode::nearest_ties_to_even);
    if (!duration) {
        throw std::runtime_error{"benchmark duration overflow"};
    }
    pcm.duration_ns = duration.value();
    pcm.segment_id = std::move(segment_id);
    pcm.segment_origin_time_ns = 0;
    pcm.segment_origin_sample_index = 0;
    pcm.first_sample_index = 0;
    pcm.sample_count = frames;
    pcm.sample_rate = kSampleRate;
    pcm.sample_format = "flt";
    pcm.channel_layout = "mono";
    pcm.channel_order = {"FC"};
    pcm.planar = false;
    pcm.resample_trace.input_sample_rate = kSampleRate;
    pcm.resample_trace.output_sample_rate = kSampleRate;
    pcm.resample_trace.implementation_id = "identity";
    pcm.resample_trace.implementation_version = "1";
    pcm.resample_trace.parameters_digest_sha256 =
        "08954dce7647560265a6c959e8eb2b90d8101319cf9c17b0624a56ccea094584";
    pcm.planes.push_back(PlaneView{0,
                                   sizeof(float),
                                   static_cast<std::uint32_t>(frames),
                                   frames * sizeof(float)});
    pcm.lease = BufferLease::from_bytes(std::move(bytes), 1, "T-031-benchmark");
    PcmNarrowAdapter adapter;
    const auto result = adapter.adapt(pcm);
    if (!result) {
        throw std::runtime_error{"benchmark PCM adapter rejected input"};
    }
    return result.value();
}

[[nodiscard]] std::uint64_t elapsed_us(const Clock::time_point start,
                                       const Clock::time_point end)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
}

[[nodiscard]] std::uint64_t percentile(std::vector<std::uint64_t> values,
                                       const std::size_t numerator,
                                       const std::size_t denominator)
{
    std::ranges::sort(values);
    const auto rank = (values.size() * numerator + denominator - 1U) / denominator;
    return values[std::max<std::size_t>(1U, rank) - 1U];
}

[[nodiscard]] std::uint64_t peak_working_set_bytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)) == 0) {
        throw std::runtime_error{"GetProcessMemoryInfo failed"};
    }
    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
}

void write_array(std::ostream& output, const std::vector<std::uint64_t>& values)
{
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << values[index];
    }
    output << ']';
}

} // namespace

int main(const int argc, const char* const argv[])
{
    try {
        if (argc != 3 || std::string_view{argv[1]} != "--output") {
            std::cerr << "usage: space_rhythm_audio_analysis_benchmark --output <json>\n";
            return 2;
        }
        const auto fixture_path = std::filesystem::path{SPACE_RHYTHM_GOLDEN_AUDIO_DIR}
            / "fixed-beat-120bpm-48000-mono.f32le";
        const auto fixture = adapt(read_bytes(fixture_path),
                                   kFixtureFrames,
                                   kFixtureHash,
                                   "AV-FIXED-BEAT-120-001-segment-0");
        const auto parameters = space_rhythm::audio::production_parameters(kSampleRate);
        const auto parameters_44100 = space_rhythm::audio::production_parameters(44'100);
        space_rhythm::audio::Analyzer analyzer;
        for (std::size_t run = 0; run < 5U; ++run) {
            const auto result = analyzer.analyze(std::span<const DspPcmBuffer>{&fixture, 1},
                                                 parameters);
            if (result.status != AnalysisStatus::success) {
                throw std::runtime_error{"warmup analysis failed"};
            }
        }

        const auto peak_before = peak_working_set_bytes();
        std::vector<std::uint64_t> durations_us;
        std::vector<std::uint64_t> throughput_frames_per_second;
        AnalysisResult last_result;
        for (std::size_t run = 0; run < 30U; ++run) {
            const auto start = Clock::now();
            last_result = analyzer.analyze(std::span<const DspPcmBuffer>{&fixture, 1}, parameters);
            const auto duration = std::max<std::uint64_t>(1U, elapsed_us(start, Clock::now()));
            if (last_result.status != AnalysisStatus::success) {
                throw std::runtime_error{"measured analysis failed"};
            }
            durations_us.push_back(duration);
            throughput_frames_per_second.push_back(kFixtureFrames * 1'000'000ULL / duration);
        }
        const auto peak_after = peak_working_set_bytes();

        constexpr std::uint64_t cancel_frames = 480'000;
        std::vector<std::byte> cancel_bytes(cancel_frames * sizeof(float));
        const auto cancel_fixture = adapt(
            std::move(cancel_bytes),
            cancel_frames,
            "124617c1f65e92d3bc895fbd869e4bb16a30754b198f59e6e973949b9aaa1b01",
            "T-031-cancel-segment");
        std::vector<std::uint64_t> cancellation_latency_us;
        for (std::size_t run = 0; run < 30U; ++run) {
            space_rhythm::core::CancellationToken cancellation;
            std::atomic_bool started{false};
            AnalysisResult cancelled_result;
            std::thread worker([&]() {
                started.store(true, std::memory_order_release);
                cancelled_result = analyzer.analyze(
                    std::span<const DspPcmBuffer>{&cancel_fixture, 1},
                    parameters,
                    {},
                    &cancellation);
            });
            while (!started.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            const auto request_time = Clock::now();
            cancellation.cancel();
            worker.join();
            cancellation_latency_us.push_back(elapsed_us(request_time, Clock::now()));
            if (cancelled_result.status != AnalysisStatus::cancelled) {
                throw std::runtime_error{"cancellation was not observed"};
            }
        }

        const std::filesystem::path output_path{argv[2]};
        std::filesystem::create_directories(output_path.parent_path());
        std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error{"cannot create benchmark output"};
        }
        output << "{\n"
               << "  \"schemaVersion\": 1,\n"
               << "  \"taskId\": \"T-031\",\n"
               << "  \"status\": \"measured\",\n"
               << "  \"effectThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"performanceThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"algorithmId\": \"" << space_rhythm::audio::algorithm_id << "\",\n"
               << "  \"algorithmVersion\": \"" << space_rhythm::audio::algorithm_version << "\",\n"
               << "  \"backendId\": \"" << space_rhythm::audio::fft_backend_id << "\",\n"
               << "  \"backendVersion\": \"" << space_rhythm::audio::fft_backend_version << "\",\n"
               << "  \"parameterSetId\": \"" << parameters.id << "\",\n"
               << "  \"parameterSetVersion\": \"" << parameters.version << "\",\n"
               << "  \"parameterDigestsBySampleRate\": {\"44100\": \""
               << parameters_44100.parameters_digest_sha256 << "\", \"48000\": \""
               << parameters.parameters_digest_sha256 << "\"},\n"
               << "  \"windowCoefficientsDigestSha256\": \""
               << parameters.window_coefficients_digest_sha256 << "\",\n"
               << "  \"sampleRate\": " << kSampleRate << ",\n"
               << "  \"inputFramesPerRun\": " << kFixtureFrames << ",\n"
               << "  \"warmupRuns\": 5,\n"
               << "  \"measuredRuns\": 30,\n"
               << "  \"durationUsSamples\": ";
        write_array(output, durations_us);
        output << ",\n  \"durationUsMedian\": " << percentile(durations_us, 50U, 100U)
               << ",\n  \"durationUsP95\": " << percentile(durations_us, 95U, 100U)
               << ",\n  \"throughputFramesPerSecondSamples\": ";
        write_array(output, throughput_frames_per_second);
        output << ",\n  \"throughputFramesPerSecondMedian\": "
               << percentile(throughput_frames_per_second, 50U, 100U)
               << ",\n  \"processPeakWorkingSetBytesBefore\": " << peak_before
               << ",\n  \"processPeakWorkingSetBytesAfter\": " << peak_after
               << ",\n  \"analyzerEstimatedPeakWorkingBytes\": "
               << last_result.estimated_peak_working_bytes
               << ",\n  \"cancellationRuns\": 30,\n"
               << "  \"cancellationLatencyUsSamples\": ";
        write_array(output, cancellation_latency_us);
        output << ",\n  \"cancellationLatencyUsMedian\": "
               << percentile(cancellation_latency_us, 50U, 100U)
               << ",\n  \"cancellationLatencyUsP95\": "
               << percentile(cancellation_latency_us, 95U, 100U) << "\n}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
