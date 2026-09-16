#include <space_rhythm/video/analysis.hpp>

#include <opencv2/core/utility.hpp>

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{

namespace core = space_rhythm::core;
namespace media = space_rhythm::media;
namespace video = space_rhythm::video;
using Clock = std::chrono::steady_clock;

struct Fixture
{
    std::string fingerprint;
    media::StreamKey stream_key;
    std::vector<media::VideoFrame> frames;
};

struct Options
{
    bool smoke = false;
    std::filesystem::path output_path;
    std::string build_preset;
    std::string execution_environment;
    std::size_t warmup_runs = 3U;
    std::size_t measured_runs = 10U;
    std::size_t cancellation_runs = 20U;
};

[[nodiscard]] std::size_t parse_positive_count(const std::string_view value,
                                               const std::string_view option)
{
    std::size_t parsed = 0U;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed == 0U)
    {
        throw std::runtime_error{std::string{option} + " must be a positive integer"};
    }
    return parsed;
}

[[nodiscard]] Options parse_options(const int argc, const char* const argv[])
{
    Options options;
    if (argc == 2 && std::string_view{argv[1]} == "--smoke")
    {
        options.smoke = true;
        options.warmup_runs = 1U;
        options.measured_runs = 1U;
        options.cancellation_runs = 1U;
        return options;
    }

    for (int index = 1; index < argc; index += 2)
    {
        if (index + 1 >= argc)
        {
            throw std::runtime_error{"missing command-line option value"};
        }
        const std::string_view option{argv[index]};
        const std::string_view value{argv[index + 1]};
        if (option == "--output")
        {
            options.output_path = value;
        }
        else if (option == "--build-preset")
        {
            options.build_preset = value;
        }
        else if (option == "--execution-environment")
        {
            options.execution_environment = value;
        }
        else if (option == "--warmup-runs")
        {
            options.warmup_runs = parse_positive_count(value, option);
        }
        else if (option == "--measured-runs")
        {
            options.measured_runs = parse_positive_count(value, option);
        }
        else if (option == "--cancellation-runs")
        {
            options.cancellation_runs = parse_positive_count(value, option);
        }
        else
        {
            throw std::runtime_error{"unknown command-line option: " + std::string{option}};
        }
    }
    if (options.output_path.empty() || options.build_preset.empty() ||
        options.execution_environment.empty())
    {
        throw std::runtime_error{"--output, --build-preset and --execution-environment are required"};
    }
    return options;
}

[[nodiscard]] Fixture decode_fixture()
{
    const auto path =
        std::filesystem::path{SPACE_RHYTHM_GOLDEN_VIDEO_DIR} / "global-pan-ramp-30fps.mkv";
    const auto source = media::MediaSource::open(path);
    if (!source)
    {
        throw std::runtime_error{"cannot open benchmark fixture"};
    }
    const auto selection = source.value()->select(
        {media::SelectionMode::required_default_then_lowest_index, std::nullopt, false},
        {media::SelectionMode::none, std::nullopt, false});
    if (!selection || selection.value().video.selected.size() != 1U)
    {
        throw std::runtime_error{"cannot select benchmark stream"};
    }
    Fixture fixture;
    fixture.fingerprint = source.value()->info().source_fingerprint_sha256;
    fixture.stream_key = selection.value().video.selected.front();
    const auto decoded = source.value()->decode_video(
        selection.value(), fixture.stream_key, {160, 90, true}, media::DecodeLimits{},
        {{},
         [&](media::VideoFrame frame)
         {
             fixture.frames.push_back(std::move(frame));
             return media::PublishResult::accepted;
         }});
    if (!decoded || fixture.frames.empty())
    {
        throw std::runtime_error{"cannot decode benchmark fixture"};
    }
    return fixture;
}

[[nodiscard]] video::AnalysisRequest make_request(const Fixture& fixture,
                                                  const video::AnalysisParameters& parameters)
{
    video::AnalysisRequest request;
    request.job_id = "T-028-performance";
    request.source_fingerprint_sha256 = fixture.fingerprint;
    request.stream_key = fixture.stream_key;
    request.parameters_digest_sha256 = parameters.parameters_digest_sha256;
    request.analysis_revision.value = "video-analysis-performance-revision-1";
    return request;
}

[[nodiscard]] std::uint64_t elapsed_us(const Clock::time_point start, const Clock::time_point end)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
}

[[nodiscard]] std::uint64_t percentile(std::vector<std::uint64_t> values,
                                       const std::size_t numerator, const std::size_t denominator)
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
                             sizeof(counters)) == 0)
    {
        throw std::runtime_error{"GetProcessMemoryInfo failed"};
    }
    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
}

void write_array(std::ostream& output, const std::vector<std::uint64_t>& values)
{
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0U)
        {
            output << ',';
        }
        output << values[index];
    }
    output << ']';
}

} // namespace

int main(const int argc, const char* const argv[])
{
    try
    {
        const auto options = parse_options(argc, argv);
        constexpr int opencv_thread_limit = 8;
        cv::setNumThreads(opencv_thread_limit);
        const auto fixture = decode_fixture();
        const auto parameters = video::production_parameters();
        const auto request = make_request(fixture, parameters);
        const auto ffmpeg = media::query_ffmpeg_build_info();
        if (!ffmpeg)
        {
            throw std::runtime_error{"cannot query FFmpeg build"};
        }
        video::Analyzer analyzer;
        const auto warmup_runs = options.warmup_runs;
        const auto measured_runs = options.measured_runs;
        const auto cancellation_runs = options.cancellation_runs;
        for (std::size_t run = 0; run < warmup_runs; ++run)
        {
            const auto result = analyzer.analyze(fixture.frames, request, parameters);
            if (result.status != video::AnalysisStatus::completed)
            {
                throw std::runtime_error{"warmup analysis failed"};
            }
        }

        const auto peak_before = peak_working_set_bytes();
        std::vector<std::uint64_t> durations_us;
        std::vector<std::uint64_t> analyzed_frames_per_second;
        std::vector<std::uint64_t> realtime_factor_ppm;
        video::AnalysisResult last_result;
        const auto media_duration_ns = static_cast<std::uint64_t>(fixture.frames.back().time_ns -
                                                                  fixture.frames.front().time_ns);
        for (std::size_t run = 0; run < measured_runs; ++run)
        {
            const auto start = Clock::now();
            last_result = analyzer.analyze(fixture.frames, request, parameters);
            const auto duration = std::max<std::uint64_t>(1U, elapsed_us(start, Clock::now()));
            if (last_result.status != video::AnalysisStatus::completed)
            {
                throw std::runtime_error{"measured analysis failed"};
            }
            durations_us.push_back(duration);
            analyzed_frames_per_second.push_back(static_cast<std::uint64_t>(fixture.frames.size()) *
                                                 1'000'000ULL / duration);
            realtime_factor_ppm.push_back(media_duration_ns * 1'000ULL / duration);
        }
        const auto peak_after = peak_working_set_bytes();

        std::vector<media::VideoFrame> cancellation_input;
        cancellation_input.reserve(3'000U);
        for (std::size_t index = 0; index < 3'000U; ++index)
        {
            auto frame = fixture.frames[index % fixture.frames.size()];
            frame.time_ns = static_cast<core::TimeNs>(index) * 1'000'000;
            frame.decode_ordinal = index;
            cancellation_input.push_back(std::move(frame));
        }
        std::vector<std::uint64_t> cancellation_latency_us;
        for (std::size_t run = 0; run < cancellation_runs; ++run)
        {
            core::CancellationToken cancellation;
            std::atomic_bool started{false};
            video::AnalysisResult result;
            std::thread worker(
                [&]()
                {
                    started.store(true, std::memory_order_release);
                    result = analyzer.analyze(cancellation_input, request, parameters, {},
                                              &cancellation);
                });
            while (!started.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            const auto cancellation_requested = Clock::now();
            cancellation.cancel();
            worker.join();
            cancellation_latency_us.push_back(elapsed_us(cancellation_requested, Clock::now()));
            if (result.status != video::AnalysisStatus::cancelled)
            {
                throw std::runtime_error{"cancellation was not observed"};
            }
        }

        if (options.smoke)
        {
            std::cout << "VIDEO_ANALYSIS_PERFORMANCE_SMOKE=PASS\n";
            return 0;
        }
        SYSTEM_INFO system_info{};
        GetNativeSystemInfo(&system_info);
        MEMORYSTATUSEX memory_status{};
        memory_status.dwLength = static_cast<DWORD>(sizeof(memory_status));
        if (GlobalMemoryStatusEx(&memory_status) == 0)
        {
            throw std::runtime_error{"GlobalMemoryStatusEx failed"};
        }
        std::array<char, 1'024> processor{};
        const auto processor_length = GetEnvironmentVariableA(
            "PROCESSOR_IDENTIFIER", processor.data(), static_cast<DWORD>(processor.size()));
        const auto& output_path = options.output_path;
        std::filesystem::create_directories(output_path.parent_path());
        std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error{"cannot create benchmark output"};
        }
        output << "{\n"
               << "  \"schemaVersion\": 1,\n"
               << "  \"taskId\": \"T-028\",\n"
               << "  \"status\": \"measured\",\n"
               << "  \"effectThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"performanceThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"buildPreset\": \"" << options.build_preset << "\",\n"
               << "  \"executionEnvironment\": \"" << options.execution_environment
               << "\",\n"
               << "  \"operatingSystem\": \"Windows\",\n"
               << "  \"processorIdentifier\": \""
               << (processor_length == 0U || processor_length >= processor.size()
                       ? "unavailable(reason=environment-not-set)"
                       : processor.data())
               << "\",\n"
               << "  \"logicalProcessorCount\": " << system_info.dwNumberOfProcessors << ",\n"
               << "  \"systemPhysicalMemoryBytes\": " << memory_status.ullTotalPhys << ",\n"
               << "  \"gpuTiming\": \"unavailable(reason=cpu-only-classic-path)\",\n"
               << "  \"gpuMemory\": \"unavailable(reason=cpu-only-classic-path)\",\n"
               << "  \"fixtureId\": \"VV-GLOBAL-PAN-001\",\n"
               << "  \"algorithmId\": \"" << video::algorithm_id << "\",\n"
               << "  \"algorithmVersion\": \"" << video::algorithm_version << "\",\n"
               << "  \"opencvVersion\": \"" << video::opencv_runtime_version() << "\",\n"
               << "  \"opencvThreadLimit\": " << opencv_thread_limit << ",\n"
               << "  \"opencvReportedThreads\": " << cv::getNumThreads() << ",\n"
               << "  \"ffmpegDecoderThreads\": 2,\n"
               << "  \"ffmpegVersion\": \"" << ffmpeg.value().ffmpeg_version << "\",\n"
               << "  \"parameterSetId\": \"" << parameters.id << "\",\n"
               << "  \"parameterSetVersion\": \"" << parameters.version << "\",\n"
               << "  \"parametersDigestSha256\": \"" << parameters.parameters_digest_sha256
               << "\",\n"
               << "  \"inputFrameCount\": " << fixture.frames.size() << ",\n"
               << "  \"inputPixelCountPerRun\": " << fixture.frames.size() * 160U * 90U << ",\n"
               << "  \"inputDurationNs\": " << media_duration_ns << ",\n"
               << "  \"warmupRuns\": " << warmup_runs << ",\n"
               << "  \"measuredRuns\": " << measured_runs << ",\n"
               << "  \"durationUsSamples\": ";
        write_array(output, durations_us);
        output << ",\n  \"durationUsMedian\": " << percentile(durations_us, 50U, 100U)
               << ",\n  \"durationUsP95\": " << percentile(durations_us, 95U, 100U)
               << ",\n  \"analyzedFramesPerSecondSamples\": ";
        write_array(output, analyzed_frames_per_second);
        output << ",\n  \"analyzedFramesPerSecondMedian\": "
               << percentile(analyzed_frames_per_second, 50U, 100U)
               << ",\n  \"realtimeFactorPpmSamples\": ";
        write_array(output, realtime_factor_ppm);
        output << ",\n  \"realtimeFactorPpmMedian\": " << percentile(realtime_factor_ppm, 50U, 100U)
               << ",\n  \"processPeakWorkingSetBytesBefore\": " << peak_before
               << ",\n  \"processPeakWorkingSetBytesAfter\": " << peak_after
               << ",\n  \"analyzerEstimatedPeakWorkingBytes\": "
               << last_result.estimated_peak_working_bytes
               << ",\n  \"cancellationRuns\": " << cancellation_runs
               << ",\n  \"cancellationLatencyUsSamples\": ";
        write_array(output, cancellation_latency_us);
        output << ",\n  \"cancellationLatencyUsMedian\": "
               << percentile(cancellation_latency_us, 50U, 100U)
               << ",\n  \"cancellationLatencyUsP95\": "
               << percentile(cancellation_latency_us, 95U, 100U) << "\n}\n";
        std::cout << "VIDEO_ANALYSIS_MEASUREMENT=PASS output=" << output_path.string() << '\n';
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "VIDEO_ANALYSIS_MEASUREMENT=FAIL reason=" << exception.what() << '\n';
        return 1;
    }
}
