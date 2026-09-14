#include <space_rhythm/video/analysis.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace space_rhythm::video
{
namespace
{

constexpr double kPpmScale = 1'000'000.0;
constexpr std::uint64_t kEstimatedCandidateBytes = 1'536U;

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U,
};

struct FrameFacts
{
    core::TimeNs time_ns{};
    std::uint64_t decode_ordinal{};
    std::uint64_t format_epoch{};
    double mean_luma{};
    double luma_stddev{};
    bool decode_had_errors{false};
    bool color_assumed{false};
    cv::Mat gray;
    cv::Mat histogram;
};

struct PairFeature
{
    core::TimeNs previous_time_ns{};
    core::TimeNs time_ns{};
    std::uint64_t segment{};
    core::NormPpm histogram_distance_ppm{};
    core::NormPpm structural_change_ppm{};
    std::int32_t signed_luma_change_ppm{};
    core::NormPpm global_motion_ppm{};
    core::NormPpm local_residual_ppm{};
    core::NormPpm motion_value_ppm{};
    core::NormPpm acceleration_ppm{};
    core::NormPpm reversal_ppm{};
    core::NormPpm stop_ppm{};
    core::NormPpm action_value_ppm{};
    double local_dx{};
    double local_dy{};
    double luma_stddev{};
    bool decode_had_errors{false};
    bool color_assumed{false};
    bool flash{false};
    bool shot_boundary{false};
    bool gradual_boundary{false};
};

struct VectorMagnitude
{
    double magnitude{};
    double dx{};
    double dy{};
};

[[nodiscard]] std::uint32_t rotate_right(const std::uint32_t value,
                                         const unsigned int count) noexcept
{
    return std::rotr(value, static_cast<int>(count));
}

[[nodiscard]] std::string sha256_hex(const std::span<const std::byte> input)
{
    std::array<std::uint32_t, 8> hash{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    std::vector<std::byte> padded(input.begin(), input.end());
    padded.push_back(std::byte{0x80});
    while ((padded.size() % 64U) != 56U)
    {
        padded.push_back(std::byte{0});
    }
    const auto bit_count = static_cast<std::uint64_t>(input.size()) * 8U;
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        padded.push_back(static_cast<std::byte>((bit_count >> shift) & 0xffU));
    }

    for (std::size_t offset = 0; offset < padded.size(); offset += 64U)
    {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16U; ++index)
        {
            const auto byte_offset = offset + index * 4U;
            words[index] = (std::to_integer<std::uint32_t>(padded[byte_offset]) << 24U) |
                           (std::to_integer<std::uint32_t>(padded[byte_offset + 1U]) << 16U) |
                           (std::to_integer<std::uint32_t>(padded[byte_offset + 2U]) << 8U) |
                           std::to_integer<std::uint32_t>(padded[byte_offset + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index)
        {
            const auto s0 = rotate_right(words[index - 15U], 7U) ^
                            rotate_right(words[index - 15U], 18U) ^ (words[index - 15U] >> 3U);
            const auto s1 = rotate_right(words[index - 2U], 17U) ^
                            rotate_right(words[index - 2U], 19U) ^ (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
        }

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];
        for (std::size_t index = 0; index < words.size(); ++index)
        {
            const auto sum1 = rotate_right(e, 6U) ^ rotate_right(e, 11U) ^ rotate_right(e, 25U);
            const auto choice = (e & f) ^ ((~e) & g);
            const auto temporary1 = h + sum1 + choice + kSha256RoundConstants[index] + words[index];
            const auto sum0 = rotate_right(a, 2U) ^ rotate_right(a, 13U) ^ rotate_right(a, 22U);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temporary2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }
        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto value : hash)
    {
        output << std::setw(8) << value;
    }
    return output.str();
}

[[nodiscard]] std::string sha256_hex(const std::string_view text)
{
    return sha256_hex(std::as_bytes(std::span{text.data(), text.size()}));
}

[[nodiscard]] bool is_lower_sha256(const std::string_view value) noexcept
{
    return value.size() == 64U &&
           std::ranges::all_of(value,
                               [](const char character)
                               {
                                   return (character >= '0' && character <= '9') ||
                                          (character >= 'a' && character <= 'f');
                               });
}

[[nodiscard]] core::NormPpm ppm(const double normalized) noexcept
{
    if (!std::isfinite(normalized) || normalized <= 0.0)
    {
        return 0U;
    }
    return static_cast<core::NormPpm>(std::llround(std::min(1.0, normalized) * kPpmScale));
}

[[nodiscard]] core::NormPpm motion_ppm(const double pixels,
                                       const AnalysisParameters& parameters) noexcept
{
    const auto full_scale_pixels =
        static_cast<double>(parameters.motion_pixels_full_scale_milli) / 1000.0;
    return ppm(pixels / full_scale_pixels);
}

[[nodiscard]] double median(std::vector<double> values)
{
    if (values.empty())
    {
        return 0.0;
    }
    const auto middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2U);
    std::nth_element(values.begin(), middle, values.end());
    const auto upper = *middle;
    if ((values.size() % 2U) != 0U)
    {
        return upper;
    }
    const auto lower = *std::max_element(values.begin(), middle);
    return (lower + upper) * 0.5;
}

[[nodiscard]] core::ErrorInfo make_error(const core::ErrorCategory category,
                                         const core::ErrorCode code, std::string reason)
{
    core::ErrorInfo error;
    error.category = category;
    error.code = code;
    error.stage = "video.analysis";
    error.diagnostic_id = "video-analysis-" + std::string{core::to_string(code)};
    error.message_key = "video.analysis." + std::string{core::to_string(code)};
    error.context.emplace("reason", std::move(reason));
    return error;
}

[[nodiscard]] bool valid_parameters(const AnalysisParameters& parameters)
{
    return parameters.schema_version == schema_version &&
           parameters.parameter_schema_version == 1U && !parameters.id.empty() &&
           !parameters.version.empty() && parameters.histogram_bins >= 8U &&
           parameters.histogram_bins <= 256U &&
           parameters.hard_cut_histogram_threshold_ppm <= core::norm_ppm_max &&
           parameters.hard_cut_structural_threshold_ppm <= core::norm_ppm_max &&
           parameters.gradual_minimum_change_ppm < parameters.gradual_maximum_change_ppm &&
           parameters.gradual_minimum_frames >= 2U &&
           parameters.flash_luma_threshold_ppm <= core::norm_ppm_max &&
           parameters.flash_recovery_threshold_ppm <= core::norm_ppm_max &&
           parameters.motion_pixels_full_scale_milli > 0U &&
           parameters.motion_peak_threshold_ppm <= core::norm_ppm_max &&
           parameters.action_peak_threshold_ppm <= core::norm_ppm_max &&
           parameters.minimum_peak_prominence_ppm <= core::norm_ppm_max &&
           parameters.stop_history_window_ms > parameters.stop_history_guard_ms &&
           parameters.stop_lookahead_ms > 0U &&
           parameters.stop_minimum_motion_ppm <= core::norm_ppm_max &&
           parameters.stop_maximum_remaining_ratio_ppm <= core::norm_ppm_max &&
           parameters.maximum_sampling_gap_ns > 0 &&
           is_lower_sha256(parameters.parameters_digest_sha256) &&
           parameters.parameters_digest_sha256 == sha256_hex(canonical_parameters_json(parameters));
}

[[nodiscard]] bool valid_request(const AnalysisRequest& request,
                                 const AnalysisParameters& parameters)
{
    const auto valid_range = !request.input_range_ns ||
                             (request.input_range_ns->start_ns >= 0 &&
                              request.input_range_ns->start_ns < request.input_range_ns->end_ns);
    return request.schema_version == schema_version &&
           request.video_analysis_contract_version == contract_version &&
           request.media_contract_version == media::contract_version &&
           request.media_schema_version == media::schema_version &&
           request.algorithm_id == algorithm_id && request.algorithm_version == algorithm_version &&
           request.parameter_set_id == parameters.id &&
           request.parameter_schema_version == parameters.parameter_schema_version &&
           request.parameters_digest_sha256 == parameters.parameters_digest_sha256 &&
           request.deterministic_seed == parameters.deterministic_seed && !request.job_id.empty() &&
           !request.analysis_revision.value.empty() &&
           is_lower_sha256(request.source_fingerprint_sha256) &&
           request.stream_key.source_fingerprint_sha256 == request.source_fingerprint_sha256 &&
           request.stream_key.stream_index >= 0 && valid_range;
}

[[nodiscard]] std::optional<std::string> validate_frame(const media::VideoFrame& frame,
                                                        const AnalysisRequest& request,
                                                        const AnalysisLimits& limits)
{
    if (frame.stream_key != request.stream_key)
    {
        return "stream_key_mismatch";
    }
    if (frame.time_ns < 0)
    {
        return "negative_project_time";
    }
    if (frame.pixel_format != "bgra" || frame.color.pixel_format != "bgra" ||
        frame.color.bit_depth != 8 || frame.color.range != "full" || frame.color.matrix != "rgb")
    {
        return "non_normalized_bgra";
    }
    if (frame.geometry.coded_width <= 0 || frame.geometry.coded_height <= 0 ||
        frame.geometry.crop_top != 0 || frame.geometry.crop_bottom != 0 ||
        frame.geometry.crop_left != 0 || frame.geometry.crop_right != 0 ||
        frame.geometry.display_transform.has_value() || !frame.geometry.sample_aspect_ratio ||
        frame.geometry.sample_aspect_ratio->numerator != 1 ||
        frame.geometry.sample_aspect_ratio->denominator != 1)
    {
        return "non_normalized_geometry";
    }
    if (!frame.lease || frame.lease.byte_size() == 0U ||
        frame.lease.byte_size() > limits.max_frame_bytes || frame.planes.size() != 1U)
    {
        return "invalid_or_oversized_lease";
    }
    const auto& plane = frame.planes.front();
    const auto row_bytes = static_cast<std::uint64_t>(frame.geometry.coded_width) * 4U;
    const auto rows = static_cast<std::uint64_t>(frame.geometry.coded_height);
    if (plane.row_bytes < 0 || static_cast<std::uint64_t>(plane.row_bytes) < row_bytes ||
        plane.rows < rows || plane.offset_bytes > frame.lease.byte_size() ||
        rows >
            std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(plane.row_bytes))
    {
        return "invalid_plane_layout";
    }
    const auto required = rows * static_cast<std::uint64_t>(plane.row_bytes);
    if (required > frame.lease.byte_size() - plane.offset_bytes || plane.valid_bytes < required)
    {
        return "truncated_plane";
    }
    return std::nullopt;
}

[[nodiscard]] FrameFacts frame_facts(const media::VideoFrame& frame,
                                     const AnalysisParameters& parameters)
{
    const auto& plane = frame.planes.front();
    const auto bytes = frame.lease.bytes();
    const auto* start = reinterpret_cast<const unsigned char*>(bytes.data() + plane.offset_bytes);
    const cv::Mat bgra{frame.geometry.coded_height, frame.geometry.coded_width, CV_8UC4,
                       const_cast<unsigned char*>(start),
                       static_cast<std::size_t>(plane.row_bytes)};
    FrameFacts facts;
    facts.time_ns = frame.time_ns;
    facts.decode_ordinal = frame.decode_ordinal;
    facts.format_epoch = frame.lease.format_epoch();
    facts.decode_had_errors = frame.decode_had_errors;
    facts.color_assumed = frame.color.assumed;
    cv::cvtColor(bgra, facts.gray, cv::COLOR_BGRA2GRAY);
    cv::Scalar mean;
    cv::Scalar standard_deviation;
    cv::meanStdDev(facts.gray, mean, standard_deviation);
    facts.mean_luma = mean[0];
    facts.luma_stddev = standard_deviation[0];
    const int channels[] = {0};
    const int histogram_size[] = {static_cast<int>(parameters.histogram_bins)};
    const float range[] = {0.0F, 256.0F};
    const float* ranges[] = {range};
    cv::calcHist(&facts.gray, 1, channels, cv::Mat{}, facts.histogram, 1, histogram_size, ranges,
                 true, false);
    cv::normalize(facts.histogram, facts.histogram, 1.0, 0.0, cv::NORM_L1);
    return facts;
}

[[nodiscard]] PairFeature pair_feature(const FrameFacts& previous, const FrameFacts& current,
                                       const AnalysisParameters& parameters,
                                       const std::uint64_t segment)
{
    PairFeature feature;
    feature.previous_time_ns = previous.time_ns;
    feature.time_ns = current.time_ns;
    feature.segment = segment;
    feature.histogram_distance_ppm =
        ppm(cv::compareHist(previous.histogram, current.histogram, cv::HISTCMP_BHATTACHARYYA));
    cv::Mat difference;
    cv::absdiff(previous.gray, current.gray, difference);
    feature.structural_change_ppm = ppm(cv::mean(difference)[0] / 255.0);
    feature.signed_luma_change_ppm = static_cast<std::int32_t>(std::llround(
        std::clamp((current.mean_luma - previous.mean_luma) / 255.0, -1.0, 1.0) * kPpmScale));
    feature.luma_stddev = std::min(previous.luma_stddev, current.luma_stddev);
    feature.decode_had_errors = previous.decode_had_errors || current.decode_had_errors;
    feature.color_assumed = previous.color_assumed || current.color_assumed;

    cv::Mat flow;
    cv::calcOpticalFlowFarneback(previous.gray, current.gray, flow, 0.5, 3, 15, 3, 5, 1.2,
                                 cv::OPTFLOW_FARNEBACK_GAUSSIAN);
    std::vector<double> dx_values;
    std::vector<double> dy_values;
    const auto sample_rows = static_cast<std::size_t>((flow.rows + 1) / 2);
    const auto sample_columns = static_cast<std::size_t>((flow.cols + 1) / 2);
    dx_values.reserve(sample_rows * sample_columns);
    dy_values.reserve(sample_rows * sample_columns);
    for (int row = 0; row < flow.rows; row += 2)
    {
        for (int column = 0; column < flow.cols; column += 2)
        {
            const auto vector = flow.at<cv::Vec2f>(row, column);
            dx_values.push_back(static_cast<double>(vector[0]));
            dy_values.push_back(static_cast<double>(vector[1]));
        }
    }
    const auto global_dx = median(dx_values);
    const auto global_dy = median(dy_values);
    feature.global_motion_ppm = motion_ppm(std::hypot(global_dx, global_dy), parameters);

    std::vector<VectorMagnitude> residuals;
    residuals.reserve(dx_values.size());
    for (std::size_t index = 0; index < dx_values.size(); ++index)
    {
        const auto dx = dx_values[index] - global_dx;
        const auto dy = dy_values[index] - global_dy;
        residuals.push_back({std::hypot(dx, dy), dx, dy});
    }
    std::ranges::sort(residuals, std::greater{}, &VectorMagnitude::magnitude);
    const auto top_count = std::max<std::size_t>(1U, residuals.size() / 100U);
    double local_magnitude = 0.0;
    for (std::size_t index = 0; index < top_count; ++index)
    {
        local_magnitude += residuals[index].magnitude;
        feature.local_dx += residuals[index].dx;
        feature.local_dy += residuals[index].dy;
    }
    local_magnitude /= static_cast<double>(top_count);
    feature.local_dx /= static_cast<double>(top_count);
    feature.local_dy /= static_cast<double>(top_count);
    feature.local_residual_ppm = motion_ppm(local_magnitude, parameters);
    feature.motion_value_ppm = std::max(feature.global_motion_ppm, feature.local_residual_ppm);
    return feature;
}

[[nodiscard]] std::vector<std::string> quality_reasons(const PairFeature& feature,
                                                       const bool for_action,
                                                       const AnalysisParameters& parameters)
{
    std::set<std::string> reasons;
    if (feature.decode_had_errors)
    {
        reasons.emplace("decode_errors");
    }
    if (feature.color_assumed)
    {
        reasons.emplace("color_metadata_assumed");
    }
    if (feature.luma_stddev * 1000.0 < static_cast<double>(parameters.low_contrast_stddev_milli))
    {
        reasons.emplace("low_contrast");
    }
    if (feature.flash)
    {
        reasons.emplace("flash_ambiguous");
    }
    if (for_action && feature.global_motion_ppm >= parameters.motion_peak_threshold_ppm / 3U &&
        static_cast<std::uint64_t>(feature.local_residual_ppm) * 2U <=
            static_cast<std::uint64_t>(feature.global_motion_ppm) * 3U)
    {
        reasons.emplace("global_motion_dominant");
    }
    return {reasons.begin(), reasons.end()};
}

[[nodiscard]] std::string join_tokens(const std::vector<std::string>& values)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0U)
        {
            output << ',';
        }
        output << values[index];
    }
    return output.str();
}

[[nodiscard]] std::string
candidate_canonical(const AnalysisRequest& request, const core::EventKind kind,
                    const core::TimeNs time_ns, const core::DurationNs duration_ns,
                    const core::TimeNs window_start_ns, const core::TimeNs window_end_ns,
                    const std::uint64_t segment)
{
    std::ostringstream canonical;
    canonical << "v1|contract=" << contract_version << "|schema=" << schema_version
              << "|input=" << request.source_fingerprint_sha256
              << "|stream=" << request.stream_key.stream_index << "|segment=" << segment
              << "|timeNs=" << time_ns << "|durationNs=" << duration_ns
              << "|kind=" << core::to_string(kind) << "|algorithm=" << request.algorithm_id
              << "|version=" << request.algorithm_version
              << "|parameters=" << request.parameters_digest_sha256
              << "|seed=" << request.deterministic_seed << "|window=" << window_start_ns << ':'
              << window_end_ns;
    return canonical.str();
}

[[nodiscard]] core::AnalysisCandidate
make_candidate(const AnalysisRequest& request, const core::EventKind kind,
               const core::TimeNs time_ns, const core::DurationNs duration_ns,
               const core::TimeNs window_start_ns, const core::TimeNs window_end_ns,
               const std::uint64_t segment, const core::NormPpm strength_ppm,
               const core::NormPpm confidence_ppm, const std::vector<std::string>& reasons,
               std::map<std::string, std::string> payload)
{
    core::AnalysisCandidate candidate;
    const auto kind_name = std::string{core::to_string(kind)};
    candidate.id.value = "va:" + kind_name + ':' +
                         sha256_hex(candidate_canonical(request, kind, time_ns, duration_ns,
                                                        window_start_ns, window_end_ns, segment));
    candidate.time_ns = time_ns;
    candidate.duration_ns = duration_ns;
    candidate.kind = kind;
    candidate.source.origin = core::EventOrigin::analysis;
    candidate.source.producer_id = "space-rhythm.video-analysis";
    candidate.source.producer_version = request.algorithm_version;
    candidate.source.input_fingerprint = request.source_fingerprint_sha256;
    candidate.source.parameters_digest = request.parameters_digest_sha256;
    candidate.source.analysis_revision = request.analysis_revision;
    candidate.source.candidate_ids.push_back(candidate.id);
    candidate.strength_ppm = strength_ppm;
    candidate.confidence_ppm = confidence_ppm;
    candidate.payload.owner = "space-rhythm.video-analysis";
    candidate.payload.schema_version = 1U;
    payload.emplace("evidenceWindowStartNs", std::to_string(window_start_ns));
    payload.emplace("evidenceWindowEndNs", std::to_string(window_end_ns));
    payload.emplace("detectorId", "space-rhythm.video-analysis.classic");
    payload.emplace("detectorVersion", request.algorithm_version);
    payload.emplace("normalizationScope", "detector-fixed-v1");
    payload.emplace("qualityReasonTokens", join_tokens(reasons));
    payload.emplace("segment", std::to_string(segment));
    candidate.payload.payload = std::move(payload);
    return candidate;
}

[[nodiscard]] core::NormPpm confidence(const core::NormPpm strength,
                                       const std::size_t penalty_count) noexcept
{
    std::uint64_t value = 650'000U + static_cast<std::uint64_t>(strength) / 3U;
    value = std::min<std::uint64_t>(980'000U, value);
    const auto penalty = static_cast<std::uint64_t>(penalty_count) * 140'000U;
    return static_cast<core::NormPpm>(value > penalty ? value - penalty : 0U);
}

[[nodiscard]] int kind_rank(const core::EventKind kind) noexcept
{
    switch (kind)
    {
    case core::EventKind::shot:
        return 0;
    case core::EventKind::motion_peak:
        return 1;
    case core::EventKind::action_peak:
        return 2;
    default:
        return 3;
    }
}

template <typename Value>
[[nodiscard]] std::vector<std::size_t>
peak_regions(const std::vector<PairFeature>& features, const core::NormPpm threshold,
             const core::NormPpm minimum_prominence, const core::DurationNs minimum_spacing_ns,
             Value value)
{
    std::vector<std::size_t> peaks;
    for (std::size_t index = 0U; index < features.size(); ++index)
    {
        if (features[index].shot_boundary || features[index].flash ||
            value(features[index]) < threshold)
        {
            continue;
        }
        const auto segment = features[index].segment;
        const auto current = value(features[index]);
        const auto left = index == 0U || features[index - 1U].segment != segment
                              ? 0U
                              : value(features[index - 1U]);
        const auto right = index + 1U >= features.size() || features[index + 1U].segment != segment
                               ? 0U
                               : value(features[index + 1U]);
        if (current < left || current < right || (current == left && current == right))
        {
            continue;
        }

        auto window_begin = index;
        while (window_begin > 0U && features[window_begin - 1U].segment == segment &&
               features[index].time_ns - features[window_begin - 1U].time_ns <= minimum_spacing_ns)
        {
            --window_begin;
        }
        auto window_end = index;
        while (window_end + 1U < features.size() && features[window_end + 1U].segment == segment &&
               features[window_end + 1U].time_ns - features[index].time_ns <= minimum_spacing_ns)
        {
            ++window_end;
        }
        auto left_minimum = current;
        for (auto sample = window_begin; sample < index; ++sample)
        {
            if (!features[sample].shot_boundary && !features[sample].flash)
            {
                left_minimum = std::min(left_minimum, value(features[sample]));
            }
        }
        auto right_minimum = current;
        for (auto sample = index + 1U; sample <= window_end; ++sample)
        {
            if (!features[sample].shot_boundary && !features[sample].flash)
            {
                right_minimum = std::min(right_minimum, value(features[sample]));
            }
        }
        const auto baseline = std::max(left_minimum, right_minimum);
        if (current - baseline < minimum_prominence)
        {
            continue;
        }
        if (peaks.empty() ||
            features[index].time_ns - features[peaks.back()].time_ns >= minimum_spacing_ns)
        {
            peaks.push_back(index);
        }
        else if (current > value(features[peaks.back()]))
        {
            peaks.back() = index;
        }
    }
    return peaks;
}

[[nodiscard]] bool near_boundary(const PairFeature& feature,
                                 const std::vector<core::TimeRange>& boundaries,
                                 const core::DurationNs padding)
{
    return std::ranges::any_of(
        boundaries,
        [&](const core::TimeRange& range)
        {
            const auto start = std::max<core::TimeNs>(0, range.start_ns - padding);
            const auto end = range.end_ns > std::numeric_limits<core::TimeNs>::max() - padding
                                 ? std::numeric_limits<core::TimeNs>::max()
                                 : range.end_ns + padding;
            return feature.time_ns >= start && feature.time_ns <= end;
        });
}

} // namespace

AnalysisParameters production_parameters()
{
    AnalysisParameters parameters;
    parameters.parameters_digest_sha256 = sha256_hex(canonical_parameters_json(parameters));
    return parameters;
}

std::string canonical_parameters_json(const AnalysisParameters& parameters)
{
    std::ostringstream json;
    json << "{\"actionPeakThresholdPpm\":" << parameters.action_peak_threshold_ppm
         << ",\"algorithmId\":\"" << algorithm_id << "\",\"algorithmVersion\":\""
         << algorithm_version << "\",\"deterministicSeed\":" << parameters.deterministic_seed
         << ",\"flashLumaThresholdPpm\":" << parameters.flash_luma_threshold_ppm
         << ",\"flashRecoveryThresholdPpm\":" << parameters.flash_recovery_threshold_ppm
         << ",\"gradualMaximumChangePpm\":" << parameters.gradual_maximum_change_ppm
         << ",\"gradualMinimumChangePpm\":" << parameters.gradual_minimum_change_ppm
         << ",\"gradualMinimumFrames\":" << parameters.gradual_minimum_frames
         << ",\"hardCutHistogramThresholdPpm\":" << parameters.hard_cut_histogram_threshold_ppm
         << ",\"hardCutStructuralThresholdPpm\":" << parameters.hard_cut_structural_threshold_ppm
         << ",\"histogramBins\":" << parameters.histogram_bins
         << ",\"lowContrastStddevMilli\":" << parameters.low_contrast_stddev_milli
         << ",\"maxSamplingGapNs\":" << parameters.maximum_sampling_gap_ns
         << ",\"minimumPeakProminencePpm\":" << parameters.minimum_peak_prominence_ppm
         << ",\"minimumPeakSpacingNs\":"
         << static_cast<std::uint64_t>(parameters.minimum_peak_spacing_ms) * 1'000'000U
         << ",\"motionFullScaleMillipixels\":" << parameters.motion_pixels_full_scale_milli
         << ",\"motionPeakThresholdPpm\":" << parameters.motion_peak_threshold_ppm
         << ",\"opencvFlow\":{\"flags\":256,\"iterations\":3,\"levels\":3,"
            "\"polyN\":5,\"polySigmaMilli\":1200,\"pyramidScalePpm\":500000,"
            "\"windowSize\":15}"
         << ",\"parameterSchemaVersion\":" << parameters.parameter_schema_version
         << ",\"parameterSetId\":\"" << parameters.id << "\",\"parameterSetVersion\":\""
         << parameters.version << "\",\"shotSuppressionPaddingNs\":"
         << static_cast<std::uint64_t>(parameters.cut_suppression_window_ms) * 1'000'000U
         << ",\"stopHistoryGuardNs\":"
         << static_cast<std::uint64_t>(parameters.stop_history_guard_ms) * 1'000'000U
         << ",\"stopHistoryWindowNs\":"
         << static_cast<std::uint64_t>(parameters.stop_history_window_ms) * 1'000'000U
         << ",\"stopLookaheadNs\":"
         << static_cast<std::uint64_t>(parameters.stop_lookahead_ms) * 1'000'000U
         << ",\"stopMaximumRemainingRatioPpm\":" << parameters.stop_maximum_remaining_ratio_ppm
         << ",\"stopMinimumMotionPpm\":" << parameters.stop_minimum_motion_ppm
         << ",\"syntheticCompressionNoiseMaximumPpm\":" << parameters.compression_noise_ceiling_ppm
         << ",\"syntheticCompressionNoiseMinimumPpm\":" << parameters.compression_noise_floor_ppm
         << '}';
    return json.str();
}

std::string_view opencv_runtime_version() noexcept
{
    static const std::string runtime_version{cv::getVersionString()};
    return runtime_version;
}

AnalysisResult Analyzer::analyze(const std::span<const media::VideoFrame> frames,
                                 const AnalysisRequest& request,
                                 const AnalysisParameters& parameters, const AnalysisLimits& limits,
                                 const core::CancellationToken* cancellation) const
{
    AnalysisResult result;
    result.job_id = request.job_id;
    result.source_fingerprint_sha256 = request.source_fingerprint_sha256;
    result.stream_key = request.stream_key;
    result.deterministic_seed = request.deterministic_seed;
    result.analysis_revision = request.analysis_revision;
    result.input_range_ns = request.input_range_ns;
    result.algorithm_id = request.algorithm_id;
    result.algorithm_version = request.algorithm_version;
    result.parameter_set_id = request.parameter_set_id;
    result.parameter_schema_version = request.parameter_schema_version;
    result.parameters_digest_sha256 = request.parameters_digest_sha256;
    result.opencv_version = std::string{opencv_runtime_version()};
    const auto fail = [&](const core::ErrorCategory category, const core::ErrorCode code,
                          const std::string& reason)
    {
        result.status = AnalysisStatus::failed;
        result.error = make_error(category, code, reason);
        result.motion_curve_samples.clear();
        result.candidates.clear();
        return result;
    };
    const auto cancel = [&]()
    {
        result.status = AnalysisStatus::cancelled;
        result.error =
            make_error(core::ErrorCategory::cancelled, core::ErrorCode::cancelled, "cancelled");
        result.motion_curve_samples.clear();
        result.candidates.clear();
        return result;
    };

    if (!valid_parameters(parameters))
    {
        return fail(core::ErrorCategory::validation, core::ErrorCode::invalid_analysis_parameters,
                    "invalid_or_unhashed_parameters");
    }
    if (!valid_request(request, parameters))
    {
        return fail(core::ErrorCategory::validation, core::ErrorCode::invalid_dto,
                    "request_contract_mismatch");
    }
    if (limits.max_frame_bytes == 0U || limits.max_input_frames == 0U ||
        limits.max_working_bytes == 0U || limits.max_candidates == 0U ||
        frames.size() > limits.max_input_frames)
    {
        return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                    "configured_analysis_limit_exceeded");
    }
    const auto feature_count = frames.empty() ? 0U : frames.size() - 1U;
    constexpr auto temporal_value_bytes = sizeof(std::uint64_t);
    if (feature_count > std::numeric_limits<std::uint64_t>::max() / sizeof(PairFeature) ||
        feature_count > std::numeric_limits<std::uint64_t>::max() / sizeof(MotionCurveSample) ||
        feature_count == std::numeric_limits<std::size_t>::max() ||
        feature_count + 1U > std::numeric_limits<std::uint64_t>::max() / temporal_value_bytes ||
        static_cast<std::uint64_t>(feature_count) * sizeof(PairFeature) >
            limits.max_working_bytes ||
        static_cast<std::uint64_t>(feature_count + 1U) * temporal_value_bytes >
            limits.max_working_bytes -
                static_cast<std::uint64_t>(feature_count) * sizeof(PairFeature) ||
        static_cast<std::uint64_t>(feature_count) * sizeof(MotionCurveSample) >
            limits.max_working_bytes -
                static_cast<std::uint64_t>(feature_count) * sizeof(PairFeature) -
                static_cast<std::uint64_t>(feature_count + 1U) * temporal_value_bytes)
    {
        return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                    "feature_storage_exceeds_limit");
    }
    if (cancellation != nullptr && cancellation->is_cancelled())
    {
        return cancel();
    }

    std::vector<PairFeature> features;
    features.reserve(feature_count);
    std::optional<FrameFacts> previous;
    std::uint64_t segment = 0U;
    std::uint64_t frame_bytes_high_water = 0U;
    std::optional<core::NormPpm> previous_local;
    double previous_local_dx = 0.0;
    double previous_local_dy = 0.0;

    for (const auto& frame : frames)
    {
        if (request.input_range_ns && !request.input_range_ns->contains(frame.time_ns))
        {
            continue;
        }
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        if (const auto reason = validate_frame(frame, request, limits))
        {
            return fail(core::ErrorCategory::validation, core::ErrorCode::invalid_feature_frame,
                        *reason);
        }
        auto current = frame_facts(frame, parameters);
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        ++result.analyzed_frame_count;
        frame_bytes_high_water = std::max(
            frame_bytes_high_water,
            static_cast<std::uint64_t>(current.gray.total() * current.gray.elemSize()) * 24U);
        const auto feature_bytes =
            static_cast<std::uint64_t>(features.capacity()) * sizeof(PairFeature);
        if (frame_bytes_high_water > limits.max_working_bytes ||
            feature_bytes > limits.max_working_bytes - frame_bytes_high_water)
        {
            return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                        "estimated_feature_storage_exceeds_limit");
        }

        if (!previous)
        {
            previous = std::move(current);
            result.segment_count = 1U;
            continue;
        }
        bool reset = false;
        if (current.time_ns < previous->time_ns)
        {
            result.diagnostics.push_back({"timestamp_discontinuity",
                                          core::TimeRange{current.time_ns, previous->time_ns + 1},
                                          {{"previousTimeNs", std::to_string(previous->time_ns)},
                                           {"timeNs", std::to_string(current.time_ns)}}});
            reset = true;
        }
        else if (current.time_ns - previous->time_ns > parameters.maximum_sampling_gap_ns)
        {
            result.diagnostics.push_back(
                {"sampling_gap",
                 core::TimeRange{previous->time_ns, current.time_ns},
                 {{"gapNs", std::to_string(current.time_ns - previous->time_ns)}}});
            reset = true;
        }
        else if (current.format_epoch != previous->format_epoch ||
                 current.gray.size() != previous->gray.size())
        {
            result.diagnostics.push_back(
                {"format_epoch_boundary",
                 core::TimeRange{previous->time_ns, current.time_ns + 1},
                 {{"previousEpoch", std::to_string(previous->format_epoch)},
                  {"epoch", std::to_string(current.format_epoch)}}});
            reset = true;
        }
        if (reset)
        {
            ++segment;
            ++result.segment_count;
            previous_local.reset();
            previous_local_dx = 0.0;
            previous_local_dy = 0.0;
            previous = std::move(current);
            continue;
        }

        auto feature = pair_feature(*previous, current, parameters, segment);
        if (previous_local)
        {
            const auto difference = feature.local_residual_ppm > *previous_local
                                        ? feature.local_residual_ppm - *previous_local
                                        : *previous_local - feature.local_residual_ppm;
            feature.acceleration_ppm = std::min<core::NormPpm>(
                core::norm_ppm_max,
                static_cast<core::NormPpm>(static_cast<std::uint64_t>(difference) * 2U));
            const auto previous_magnitude = std::hypot(previous_local_dx, previous_local_dy);
            const auto current_magnitude = std::hypot(feature.local_dx, feature.local_dy);
            if (previous_magnitude > 0.05 && current_magnitude > 0.05)
            {
                const auto cosine =
                    (previous_local_dx * feature.local_dx + previous_local_dy * feature.local_dy) /
                    (previous_magnitude * current_magnitude);
                if (cosine < -0.25)
                {
                    feature.reversal_ppm = ppm(
                        (-cosine) * std::min(previous_magnitude, current_magnitude) /
                        (static_cast<double>(parameters.motion_pixels_full_scale_milli) / 1000.0));
                }
            }
            feature.action_value_ppm = std::max(feature.acceleration_ppm, feature.reversal_ppm);
            if (feature.reversal_ppm > 0U)
            {
                feature.motion_value_ppm = std::max<core::NormPpm>(
                    feature.motion_value_ppm,
                    static_cast<core::NormPpm>(std::min<std::uint64_t>(
                        core::norm_ppm_max,
                        static_cast<std::uint64_t>(feature.reversal_ppm) * 2U)));
            }
        }
        previous_local = feature.local_residual_ppm;
        previous_local_dx = feature.local_dx;
        previous_local_dy = feature.local_dy;
        features.push_back(feature);
        previous = std::move(current);
    }

    if (result.analyzed_frame_count < 2U)
    {
        result.diagnostics.push_back({"insufficient_frames", std::nullopt, {}});
        result.status = AnalysisStatus::low_quality;
        result.estimated_peak_working_bytes = frame_bytes_high_water;
        return result;
    }

    std::vector<std::uint64_t> local_motion_prefix(features.size() + 1U, 0U);
    for (std::size_t index = 0; index < features.size(); ++index)
    {
        local_motion_prefix[index + 1U] =
            local_motion_prefix[index] + features[index].local_residual_ppm;
    }
    const auto history_window_ns = static_cast<core::DurationNs>(
        static_cast<std::uint64_t>(parameters.stop_history_window_ms) * 1'000'000U);
    const auto history_guard_ns = static_cast<core::DurationNs>(
        static_cast<std::uint64_t>(parameters.stop_history_guard_ms) * 1'000'000U);
    const auto lookahead_ns = static_cast<core::DurationNs>(
        static_cast<std::uint64_t>(parameters.stop_lookahead_ms) * 1'000'000U);
    std::size_t segment_begin = 0U;
    while (segment_begin < features.size())
    {
        auto segment_end = segment_begin + 1U;
        while (segment_end < features.size() &&
               features[segment_end].segment == features[segment_begin].segment)
        {
            ++segment_end;
        }
        const auto first_time = features[segment_begin].time_ns;
        const auto last_time = features[segment_end - 1U].time_ns;
        const auto lower_time = [&](const core::TimeNs time_ns)
        {
            return static_cast<std::size_t>(
                std::lower_bound(features.begin() + static_cast<std::ptrdiff_t>(segment_begin),
                                 features.begin() + static_cast<std::ptrdiff_t>(segment_end),
                                 time_ns, [](const PairFeature& feature, const core::TimeNs value)
                                 { return feature.time_ns < value; }) -
                features.begin());
        };
        for (auto index = segment_begin; index < segment_end; ++index)
        {
            if (cancellation != nullptr && cancellation->is_cancelled())
            {
                return cancel();
            }
            const auto time_ns = features[index].time_ns;
            if (time_ns - first_time < history_window_ns || last_time - time_ns < lookahead_ns)
            {
                continue;
            }
            const auto history_begin = lower_time(time_ns - history_window_ns);
            const auto history_end = lower_time(time_ns - history_guard_ns);
            const auto future_begin = lower_time(time_ns);
            const auto future_end = lower_time(time_ns + lookahead_ns);
            if (history_begin == history_end || future_begin == future_end)
            {
                continue;
            }
            const auto history_average =
                (local_motion_prefix[history_end] - local_motion_prefix[history_begin]) /
                static_cast<std::uint64_t>(history_end - history_begin);
            const auto future_average =
                (local_motion_prefix[future_end] - local_motion_prefix[future_begin]) /
                static_cast<std::uint64_t>(future_end - future_begin);
            if (history_average < parameters.stop_minimum_motion_ppm ||
                future_average > parameters.stop_minimum_motion_ppm ||
                future_average * core::norm_ppm_max >
                    history_average * parameters.stop_maximum_remaining_ratio_ppm)
            {
                continue;
            }
            auto& feature = features[index];
            feature.stop_ppm = static_cast<core::NormPpm>((history_average - future_average) *
                                                          core::norm_ppm_max / history_average);
            feature.action_value_ppm = std::max(feature.action_value_ppm, feature.stop_ppm);
            feature.motion_value_ppm = std::max(feature.motion_value_ppm, feature.stop_ppm);
        }
        segment_begin = segment_end;
    }

    result.motion_curve_samples.reserve(features.size());
    for (const auto& feature : features)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        result.motion_curve_samples.push_back(
            {feature.previous_time_ns, feature.time_ns, feature.segment, feature.global_motion_ppm,
             feature.local_residual_ppm, feature.motion_value_ppm, feature.action_value_ppm});
    }

    const auto feature_storage_bytes =
        static_cast<std::uint64_t>(features.capacity()) * sizeof(PairFeature);
    const auto temporal_storage_bytes =
        static_cast<std::uint64_t>(local_motion_prefix.capacity()) * temporal_value_bytes;
    const auto curve_storage_bytes = static_cast<std::uint64_t>(
        result.motion_curve_samples.capacity() * sizeof(MotionCurveSample));
    const auto append_candidate = [&](core::AnalysisCandidate candidate)
    {
        if (result.candidates.size() >= limits.max_candidates ||
            result.candidates.size() >=
                std::numeric_limits<std::uint64_t>::max() / kEstimatedCandidateBytes)
        {
            return false;
        }
        const auto next_candidate_bytes =
            static_cast<std::uint64_t>(result.candidates.size() + 1U) * kEstimatedCandidateBytes;
        if (frame_bytes_high_water > limits.max_working_bytes ||
            feature_storage_bytes > limits.max_working_bytes - frame_bytes_high_water ||
            temporal_storage_bytes >
                limits.max_working_bytes - frame_bytes_high_water - feature_storage_bytes ||
            curve_storage_bytes > limits.max_working_bytes - frame_bytes_high_water -
                                      feature_storage_bytes - temporal_storage_bytes ||
            next_candidate_bytes > limits.max_working_bytes - frame_bytes_high_water -
                                       feature_storage_bytes - temporal_storage_bytes -
                                       curve_storage_bytes)
        {
            return false;
        }
        result.candidates.push_back(std::move(candidate));
        return true;
    };

    std::vector<core::TimeRange> boundary_ranges;
    for (std::size_t index = 0; index < features.size(); ++index)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        auto& feature = features[index];
        if (index + 1U < features.size() && features[index + 1U].segment == feature.segment)
        {
            const auto current_delta = static_cast<std::int64_t>(feature.signed_luma_change_ppm);
            const auto next_delta =
                static_cast<std::int64_t>(features[index + 1U].signed_luma_change_ppm);
            const auto recovery_error = std::llabs(current_delta + next_delta);
            const auto detected_flash =
                std::llabs(current_delta) >= parameters.flash_luma_threshold_ppm &&
                std::llabs(next_delta) >= parameters.flash_luma_threshold_ppm &&
                ((current_delta < 0) != (next_delta < 0)) &&
                recovery_error <= parameters.flash_recovery_threshold_ppm;
            feature.flash = feature.flash || detected_flash;
            features[index + 1U].flash = features[index + 1U].flash || detected_flash;
            if (detected_flash && index + 2U < features.size() &&
                features[index + 2U].segment == feature.segment)
            {
                features[index + 2U].flash = true;
            }
            if (detected_flash)
            {
                result.diagnostics.push_back(
                    {"flash_ambiguous",
                     core::TimeRange{feature.previous_time_ns, features[index + 1U].time_ns + 1},
                     {}});
            }
        }
        if (!feature.flash &&
            feature.histogram_distance_ppm >= parameters.hard_cut_histogram_threshold_ppm &&
            feature.structural_change_ppm >= parameters.hard_cut_structural_threshold_ppm)
        {
            feature.shot_boundary = true;
            boundary_ranges.push_back({feature.time_ns, feature.time_ns + 1});
            const auto strength =
                std::max(feature.histogram_distance_ppm, feature.structural_change_ppm);
            const auto reasons = quality_reasons(feature, false, parameters);
            if (!append_candidate(make_candidate(
                    request, core::EventKind::shot, feature.time_ns, 0, feature.previous_time_ns,
                    feature.time_ns, feature.segment, strength,
                    confidence(strength, reasons.size()), reasons,
                    {{"boundaryKind", "hard_cut"},
                     {"flashLikelihoodPpm", std::to_string(feature.flash ? 1'000'000U : 0U)},
                     {"histogramDistancePpm", std::to_string(feature.histogram_distance_ppm)},
                     {"structuralChangePpm", std::to_string(feature.structural_change_ppm)},
                     {"transitionEndNs", std::to_string(feature.time_ns)},
                     {"transitionStartNs", std::to_string(feature.time_ns)}})))
            {
                return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                            "candidate_storage_exceeds_limit");
            }
        }
    }

    std::size_t gradual_index = 0U;
    while (gradual_index < features.size())
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        const auto& feature = features[gradual_index];
        const auto sign = feature.signed_luma_change_ppm < 0 ? -1 : 1;
        const auto eligible =
            feature.structural_change_ppm >= parameters.gradual_minimum_change_ppm &&
            feature.structural_change_ppm <= parameters.gradual_maximum_change_ppm &&
            std::llabs(static_cast<std::int64_t>(feature.signed_luma_change_ppm)) >=
                parameters.gradual_minimum_change_ppm / 3U &&
            feature.motion_value_ppm < parameters.motion_peak_threshold_ppm;
        if (!eligible)
        {
            ++gradual_index;
            continue;
        }
        const auto begin = gradual_index;
        const auto current_segment = feature.segment;
        while (gradual_index + 1U < features.size())
        {
            const auto& next = features[gradual_index + 1U];
            const auto next_sign = next.signed_luma_change_ppm < 0 ? -1 : 1;
            if (next.segment != current_segment || next_sign != sign ||
                next.structural_change_ppm < parameters.gradual_minimum_change_ppm ||
                next.structural_change_ppm > parameters.gradual_maximum_change_ppm ||
                std::llabs(static_cast<std::int64_t>(next.signed_luma_change_ppm)) <
                    parameters.gradual_minimum_change_ppm / 3U ||
                next.motion_value_ppm >= parameters.motion_peak_threshold_ppm)
            {
                break;
            }
            ++gradual_index;
        }
        const auto count = gradual_index - begin + 1U;
        if (count >= parameters.gradual_minimum_frames)
        {
            const auto start = features[begin].time_ns;
            const auto end = features[gradual_index].time_ns;
            const auto duration = end - start;
            core::NormPpm strength = 0U;
            for (std::size_t index = begin; index <= gradual_index; ++index)
            {
                features[index].shot_boundary = true;
                features[index].gradual_boundary = true;
                strength = std::max(strength, features[index].structural_change_ppm);
            }
            boundary_ranges.push_back({start, end + 1});
            const auto reasons = quality_reasons(features[begin], false, parameters);
            if (!append_candidate(make_candidate(request, core::EventKind::shot, start, duration,
                                                 features[begin].previous_time_ns, end,
                                                 current_segment, strength,
                                                 confidence(strength, reasons.size()), reasons,
                                                 {{"boundaryKind", "gradual_transition"},
                                                  {"flashLikelihoodPpm", "0"},
                                                  {"histogramDistancePpm", "0"},
                                                  {"structuralChangePpm", std::to_string(strength)},
                                                  {"transitionEndNs", std::to_string(end)},
                                                  {"transitionStartNs", std::to_string(start)}})))
            {
                return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                            "candidate_storage_exceeds_limit");
            }
        }
        ++gradual_index;
    }

    const auto padding = static_cast<core::DurationNs>(
        static_cast<std::uint64_t>(parameters.cut_suppression_window_ms) * 1'000'000U);
    for (auto& feature : features)
    {
        if (near_boundary(feature, boundary_ranges, padding))
        {
            feature.shot_boundary = true;
        }
    }
    const auto spacing = static_cast<core::DurationNs>(
        static_cast<std::uint64_t>(parameters.minimum_peak_spacing_ms) * 1'000'000U);
    const auto motion_peaks = peak_regions(
        features, parameters.motion_peak_threshold_ppm, parameters.minimum_peak_prominence_ppm,
        spacing, [](const PairFeature& value) { return value.motion_value_ppm; });
    for (const auto index : motion_peaks)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        const auto& feature = features[index];
        const auto reasons = quality_reasons(feature, false, parameters);
        const auto global = static_cast<std::uint64_t>(feature.global_motion_ppm);
        const auto local = static_cast<std::uint64_t>(feature.local_residual_ppm);
        const auto motion_class =
            global * 2U > local * 3U ? "global" : (local * 2U > global * 3U ? "local" : "mixed");
        if (!append_candidate(make_candidate(
                request, core::EventKind::motion_peak, feature.time_ns, 0, feature.previous_time_ns,
                feature.time_ns, feature.segment, feature.motion_value_ppm,
                confidence(feature.motion_value_ppm, reasons.size()), reasons,
                {{"curveSchemaVersion", "1"},
                 {"curveValuePpm", std::to_string(feature.motion_value_ppm)},
                 {"globalMotionPpm", std::to_string(feature.global_motion_ppm)},
                 {"localResidualPpm", std::to_string(feature.local_residual_ppm)},
                 {"motionClass", motion_class},
                 {"prominencePpm", std::to_string(feature.motion_value_ppm)}})))
        {
            return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                        "candidate_storage_exceeds_limit");
        }
    }

    const auto action_peaks = peak_regions(
        features, parameters.action_peak_threshold_ppm, parameters.minimum_peak_prominence_ppm,
        spacing,
        [&](const PairFeature& value)
        {
            return value.global_motion_ppm >= parameters.motion_peak_threshold_ppm / 3U &&
                           static_cast<std::uint64_t>(value.local_residual_ppm) * 2U <=
                               static_cast<std::uint64_t>(value.global_motion_ppm) * 3U
                       ? 0U
                       : value.action_value_ppm;
        });
    for (const auto index : action_peaks)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            return cancel();
        }
        const auto& feature = features[index];
        auto reasons = quality_reasons(feature, true, parameters);
        const auto action_class =
            feature.stop_ppm >= feature.reversal_ppm &&
                    feature.stop_ppm >= feature.acceleration_ppm && feature.stop_ppm > 0U
                ? "stop"
            : feature.reversal_ppm >= feature.acceleration_ppm && feature.reversal_ppm > 0U
                ? "reversal"
                : "impact";
        if (!append_candidate(make_candidate(
                request, core::EventKind::action_peak, feature.time_ns, 0, feature.previous_time_ns,
                feature.time_ns, feature.segment, feature.action_value_ppm,
                confidence(feature.action_value_ppm, reasons.size()), reasons,
                {{"accelerationPpm", std::to_string(feature.acceleration_ppm)},
                 {"actionClass", action_class},
                 {"globalSuppressionPpm", std::to_string(feature.global_motion_ppm)},
                 {"localResidualPpm", std::to_string(feature.local_residual_ppm)},
                 {"reversalPpm", std::to_string(feature.reversal_ppm)},
                 {"stopPpm", std::to_string(feature.stop_ppm)},
                 {"stillnessAfterPpm",
                  std::to_string(index + 1U < features.size()
                                     ? core::norm_ppm_max - features[index + 1U].local_residual_ppm
                                     : 0U)}})))
        {
            return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                        "candidate_storage_exceeds_limit");
        }
    }

    const auto candidate_bytes =
        static_cast<std::uint64_t>(result.candidates.size()) * kEstimatedCandidateBytes;
    result.estimated_peak_working_bytes = frame_bytes_high_water + feature_storage_bytes +
                                          temporal_storage_bytes + curve_storage_bytes +
                                          candidate_bytes;
    if (result.candidates.size() > limits.max_candidates ||
        result.estimated_peak_working_bytes > limits.max_working_bytes)
    {
        return fail(core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                    "candidate_or_working_memory_limit_exceeded");
    }

    core::NormPpm maximum_motion = 0U;
    std::uint64_t structural_sum = 0U;
    for (const auto& feature : features)
    {
        maximum_motion = std::max(maximum_motion, feature.motion_value_ppm);
        structural_sum += feature.structural_change_ppm;
    }
    const auto structural_average =
        features.empty() ? 0U : static_cast<core::NormPpm>(structural_sum / features.size());
    if (maximum_motion < parameters.motion_peak_threshold_ppm / 4U)
    {
        result.diagnostics.push_back({"near_static", std::nullopt, {}});
    }
    if (boundary_ranges.empty() && structural_average >= parameters.compression_noise_floor_ppm &&
        structural_average <= parameters.compression_noise_ceiling_ppm &&
        maximum_motion < parameters.motion_peak_threshold_ppm / 2U)
    {
        result.diagnostics.push_back(
            {"compression_noise",
             std::nullopt,
             {{"averageStructuralChangePpm", std::to_string(structural_average)}}});
    }
    if (std::ranges::any_of(features, &PairFeature::decode_had_errors))
    {
        result.diagnostics.push_back({"decode_errors", std::nullopt, {}});
    }
    if (std::ranges::any_of(features, &PairFeature::color_assumed))
    {
        result.diagnostics.push_back({"color_metadata_assumed", std::nullopt, {}});
    }

    std::ranges::sort(result.candidates,
                      [](const auto& left, const auto& right)
                      {
                          return std::tuple{left.time_ns, kind_rank(left.kind), left.id.value} <
                                 std::tuple{right.time_ns, kind_rank(right.kind), right.id.value};
                      });
    const auto duplicate =
        std::adjacent_find(result.candidates.begin(), result.candidates.end(),
                           [](const auto& left, const auto& right) { return left.id == right.id; });
    if (duplicate != result.candidates.end())
    {
        return fail(core::ErrorCategory::internal, core::ErrorCode::invariant_violation,
                    "candidate_id_collision");
    }

    const auto low_quality =
        std::ranges::any_of(result.diagnostics,
                            [](const auto& diagnostic)
                            {
                                return diagnostic.code == "decode_errors" ||
                                       diagnostic.code == "timestamp_discontinuity" ||
                                       diagnostic.code == "sampling_gap" ||
                                       diagnostic.code == "format_epoch_boundary" ||
                                       diagnostic.code == "insufficient_frames" ||
                                       diagnostic.code == "compression_noise";
                            });
    result.status = low_quality ? AnalysisStatus::low_quality : AnalysisStatus::completed;
    return result;
}

} // namespace space_rhythm::video
