#include <space_rhythm/video/analysis.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{

namespace core = space_rhythm::core;
namespace media = space_rhythm::media;
namespace video = space_rhythm::video;

struct Truth
{
    core::EventKind kind;
    core::TimeNs time_ns;
    core::DurationNs duration_ns;
    core::DurationNs before_ns;
    core::DurationNs after_ns;
};

struct FixtureSpec
{
    std::string_view id;
    std::string_view file;
    std::vector<Truth> truth;
};

struct Statistics
{
    std::uint64_t true_positive{};
    std::uint64_t false_positive{};
    std::uint64_t false_negative{};
    std::vector<std::uint64_t> absolute_time_errors_ns;
};

struct FixtureMeasurement
{
    std::string id;
    std::string fingerprint;
    video::AnalysisStatus status{video::AnalysisStatus::failed};
    std::size_t motion_curve_sample_count{};
    std::vector<core::AnalysisCandidate> candidates;
    std::vector<video::AnalysisDiagnostic> diagnostics;
    std::map<core::EventKind, Statistics> statistics;
};

[[nodiscard]] std::vector<FixtureSpec> fixture_specs()
{
    using enum core::EventKind;
    return {
        {"VV-HARD-CUT-001",
         "hard-cut-30fps.mkv",
         {{shot, 1'000'000'000, 0, 33'333'334, 33'333'334}}},
        {"VV-DISSOLVE-001", "dissolve-30fps.mkv", {{shot, 1'000'000'000, 1'000'000'000, 0, 0}}},
        {"VV-FLASH-001", "single-frame-flash-30fps.mkv", {}},
        {"VV-GLOBAL-PAN-001",
         "global-pan-ramp-30fps.mkv",
         {{motion_peak, 500'000'000, 0, 66'666'667, 66'666'667}}},
        {"VV-LOCAL-IMPACT-001",
         "local-impact-30fps.mkv",
         {{motion_peak, 1'500'000'000, 0, 50'000'000, 50'000'000},
          {action_peak, 1'500'000'000, 0, 50'000'000, 50'000'000}}},
        {"VV-STATIC-001", "static-grid-30fps.mkv", {}},
        {"VV-SLOW-MOTION-001",
         "slow-motion-stop-60fps.mkv",
         {{motion_peak, 2'500'000'000, 0, 100'000'000, 100'000'000},
          {action_peak, 2'500'000'000, 0, 100'000'000, 100'000'000}}},
        {"VV-VFR-REVERSAL-001",
         "vfr-reversal.mkv",
         {{motion_peak, 240'000'000, 0, 60'000'000, 60'000'000},
          {action_peak, 240'000'000, 0, 60'000'000, 60'000'000}}},
        {"VV-COMPRESSION-NOISE-001", "compression-noise-static-30fps.mkv", {}},
        {"VV-FAST-CUT-ACTION-001",
         "fast-cut-action-30fps.mkv",
         {{shot, 500'000'000, 0, 33'333'334, 33'333'334},
          {shot, 1'000'000'000, 0, 33'333'334, 33'333'334},
          {motion_peak, 1'250'000'000, 0, 50'000'000, 50'000'000},
          {action_peak, 1'250'000'000, 0, 50'000'000, 50'000'000},
          {shot, 1'500'000'000, 0, 33'333'334, 33'333'334}}},
    };
}

[[nodiscard]] std::string_view status_name(const video::AnalysisStatus status)
{
    switch (status)
    {
    case video::AnalysisStatus::completed:
        return "completed";
    case video::AnalysisStatus::low_quality:
        return "low_quality";
    case video::AnalysisStatus::cancelled:
        return "cancelled";
    case video::AnalysisStatus::failed:
        return "failed";
    }
    return "failed";
}

[[nodiscard]] FixtureMeasurement measure_fixture(const FixtureSpec& spec,
                                                 const video::AnalysisParameters& parameters)
{
    const auto path = std::filesystem::path{SPACE_RHYTHM_GOLDEN_VIDEO_DIR} / spec.file;
    const auto source = media::MediaSource::open(path);
    if (!source)
    {
        throw std::runtime_error{"cannot open golden fixture"};
    }
    const auto selection = source.value()->select(
        {media::SelectionMode::required_default_then_lowest_index, std::nullopt, false},
        {media::SelectionMode::none, std::nullopt, false});
    if (!selection || selection.value().video.selected.size() != 1U)
    {
        throw std::runtime_error{"cannot select golden fixture video"};
    }
    std::vector<media::VideoFrame> frames;
    const auto stream_key = selection.value().video.selected.front();
    const auto decoded = source.value()->decode_video(selection.value(), stream_key,
                                                      {160, 90, true}, media::DecodeLimits{},
                                                      {{},
                                                       [&](media::VideoFrame frame)
                                                       {
                                                           frames.push_back(std::move(frame));
                                                           return media::PublishResult::accepted;
                                                       }});
    if (!decoded || !decoded.value().end_of_stream)
    {
        throw std::runtime_error{"cannot decode golden fixture"};
    }
    video::AnalysisRequest request;
    request.job_id = "T-028-effect-measurement";
    request.source_fingerprint_sha256 = source.value()->info().source_fingerprint_sha256;
    request.stream_key = stream_key;
    request.parameters_digest_sha256 = parameters.parameters_digest_sha256;
    request.analysis_revision.value = "video-analysis-effect-revision-1";
    const auto result = video::Analyzer{}.analyze(frames, request, parameters);
    if (result.status == video::AnalysisStatus::failed ||
        result.status == video::AnalysisStatus::cancelled)
    {
        throw std::runtime_error{"golden analysis did not complete"};
    }

    FixtureMeasurement measurement;
    measurement.id = spec.id;
    measurement.fingerprint = source.value()->info().source_fingerprint_sha256;
    measurement.status = result.status;
    measurement.motion_curve_sample_count = result.motion_curve_samples.size();
    measurement.candidates = result.candidates;
    measurement.diagnostics = result.diagnostics;
    for (const auto kind :
         {core::EventKind::shot, core::EventKind::motion_peak, core::EventKind::action_peak})
    {
        std::vector<bool> matched_truth(spec.truth.size(), false);
        for (const auto& candidate : result.candidates)
        {
            if (candidate.kind != kind)
            {
                continue;
            }
            std::optional<std::size_t> best;
            auto best_error = std::numeric_limits<std::uint64_t>::max();
            for (std::size_t index = 0; index < spec.truth.size(); ++index)
            {
                const auto& truth = spec.truth[index];
                if (matched_truth[index] || truth.kind != kind ||
                    candidate.time_ns < truth.time_ns - truth.before_ns ||
                    candidate.time_ns > truth.time_ns + truth.after_ns)
                {
                    continue;
                }
                const auto error = static_cast<std::uint64_t>(
                    candidate.time_ns >= truth.time_ns ? candidate.time_ns - truth.time_ns
                                                       : truth.time_ns - candidate.time_ns);
                if (error < best_error)
                {
                    best = index;
                    best_error = error;
                }
            }
            auto& statistics = measurement.statistics[kind];
            if (best)
            {
                matched_truth[*best] = true;
                ++statistics.true_positive;
                statistics.absolute_time_errors_ns.push_back(best_error);
            }
            else
            {
                ++statistics.false_positive;
            }
        }
        auto& statistics = measurement.statistics[kind];
        for (std::size_t index = 0; index < spec.truth.size(); ++index)
        {
            if (spec.truth[index].kind == kind && !matched_truth[index])
            {
                ++statistics.false_negative;
            }
        }
    }
    return measurement;
}

void write_ratio_ppm(std::ostream& output, const std::uint64_t numerator,
                     const std::uint64_t denominator)
{
    if (denominator == 0U)
    {
        output << "\"unavailable(reason=undefined_denominator)\"";
        return;
    }
    output << numerator * 1'000'000ULL / denominator;
}

[[nodiscard]] std::uint64_t percentile(std::vector<std::uint64_t> values,
                                       const std::size_t numerator, const std::size_t denominator)
{
    if (values.empty())
    {
        return 0U;
    }
    std::ranges::sort(values);
    const auto rank = (values.size() * numerator + denominator - 1U) / denominator;
    return values[std::max<std::size_t>(1U, rank) - 1U];
}

} // namespace

int main(const int argc, const char* const argv[])
{
    try
    {
        if (argc != 7 || std::string_view{argv[1]} != "--output" ||
            std::string_view{argv[3]} != "--build-preset" ||
            std::string_view{argv[5]} != "--execution-environment")
        {
            std::cerr << "usage: space_rhythm_video_analysis_golden_measurement "
                         "--output <json> --build-preset <name> "
                         "--execution-environment <name>\n";
            return 2;
        }
        const auto parameters = video::production_parameters();
        std::vector<FixtureMeasurement> measurements;
        for (const auto& fixture : fixture_specs())
        {
            measurements.push_back(measure_fixture(fixture, parameters));
        }
        std::map<core::EventKind, Statistics> aggregate;
        for (const auto& fixture : measurements)
        {
            for (const auto& [kind, statistics] : fixture.statistics)
            {
                auto& total = aggregate[kind];
                total.true_positive += statistics.true_positive;
                total.false_positive += statistics.false_positive;
                total.false_negative += statistics.false_negative;
                total.absolute_time_errors_ns.insert(total.absolute_time_errors_ns.end(),
                                                     statistics.absolute_time_errors_ns.begin(),
                                                     statistics.absolute_time_errors_ns.end());
            }
        }
        const std::filesystem::path output_path{argv[2]};
        std::filesystem::create_directories(output_path.parent_path());
        std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error{"cannot create effect measurement output"};
        }
        output << "{\n"
               << "  \"schemaVersion\": 1,\n"
               << "  \"taskId\": \"T-028\",\n"
               << "  \"status\": \"measured\",\n"
               << "  \"effectThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"naturalnessThresholdEvaluation\": \"not-evaluated\",\n"
               << "  \"buildPreset\": \"" << argv[4] << "\",\n"
               << "  \"executionEnvironment\": \"" << argv[6] << "\",\n"
               << "  \"algorithmId\": \"" << video::algorithm_id << "\",\n"
               << "  \"algorithmVersion\": \"" << video::algorithm_version << "\",\n"
               << "  \"opencvVersion\": \"" << video::opencv_runtime_version() << "\",\n"
               << "  \"motionCurveSchemaVersion\": " << video::motion_curve_schema_version << ",\n"
               << "  \"parametersDigestSha256\": \"" << parameters.parameters_digest_sha256
               << "\",\n"
               << "  \"fixtures\": [\n";
        for (std::size_t fixture_index = 0; fixture_index < measurements.size(); ++fixture_index)
        {
            const auto& fixture = measurements[fixture_index];
            output << "    {\"id\":\"" << fixture.id << "\",\"sourceFingerprintSha256\":\""
                   << fixture.fingerprint << "\",\"analysisStatus\":\""
                   << status_name(fixture.status)
                   << "\",\"motionCurveSampleCount\":" << fixture.motion_curve_sample_count
                   << ",\"diagnostics\":[";
            for (std::size_t index = 0; index < fixture.diagnostics.size(); ++index)
            {
                if (index != 0U)
                {
                    output << ',';
                }
                output << '\"' << fixture.diagnostics[index].code << '\"';
            }
            output << "],\"candidates\":[";
            for (std::size_t index = 0; index < fixture.candidates.size(); ++index)
            {
                if (index != 0U)
                {
                    output << ',';
                }
                const auto& candidate = fixture.candidates[index];
                output << "{\"id\":\"" << candidate.id.value << "\",\"kind\":\""
                       << core::to_string(candidate.kind) << "\",\"timeNs\":" << candidate.time_ns
                       << ",\"durationNs\":" << candidate.duration_ns
                       << ",\"strengthPpm\":" << candidate.strength_ppm
                       << ",\"confidencePpm\":" << candidate.confidence_ppm
                       << ",\"qualityReasonTokens\":\""
                       << candidate.payload.payload.at("qualityReasonTokens") << "\",\"payload\":{";
                std::size_t payload_index = 0U;
                for (const auto& [key, value] : candidate.payload.payload)
                {
                    if (payload_index++ != 0U)
                    {
                        output << ',';
                    }
                    output << '\"' << key << "\":\"" << value << '\"';
                }
                output << "}}";
            }
            output << "]}" << (fixture_index + 1U == measurements.size() ? "\n" : ",\n");
        }
        output << "  ],\n  \"aggregateByKind\": {\n";
        const std::array kinds{core::EventKind::shot, core::EventKind::motion_peak,
                               core::EventKind::action_peak};
        for (std::size_t index = 0; index < kinds.size(); ++index)
        {
            const auto kind = kinds[index];
            const auto& value = aggregate[kind];
            const auto precision_denominator = value.true_positive + value.false_positive;
            const auto recall_denominator = value.true_positive + value.false_negative;
            const auto f1_denominator =
                2U * value.true_positive + value.false_positive + value.false_negative;
            output << "    \"" << core::to_string(kind) << "\": {\"tp\":" << value.true_positive
                   << ",\"fp\":" << value.false_positive << ",\"fn\":" << value.false_negative
                   << ",\"precisionPpm\":";
            write_ratio_ppm(output, value.true_positive, precision_denominator);
            output << ",\"recallPpm\":";
            write_ratio_ppm(output, value.true_positive, recall_denominator);
            output << ",\"f1Ppm\":";
            write_ratio_ppm(output, 2U * value.true_positive, f1_denominator);
            if (value.absolute_time_errors_ns.empty())
            {
                output << ",\"absoluteTimeErrorNsMedian\":"
                          "\"unavailable(reason=no_matches)\","
                          "\"absoluteTimeErrorNsP95\":"
                          "\"unavailable(reason=no_matches)\"";
            }
            else
            {
                output << ",\"absoluteTimeErrorNsMedian\":"
                       << percentile(value.absolute_time_errors_ns, 50U, 100U)
                       << ",\"absoluteTimeErrorNsP95\":"
                       << percentile(value.absolute_time_errors_ns, 95U, 100U);
            }
            output << "}" << (index + 1U == kinds.size() ? "\n" : ",\n");
        }
        output << "  }\n}\n";
        std::cout << "VIDEO_ANALYSIS_EFFECT_MEASUREMENT=PASS output=" << output_path.string()
                  << '\n';
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "VIDEO_ANALYSIS_EFFECT_MEASUREMENT=FAIL reason=" << exception.what() << '\n';
        return 1;
    }
}
