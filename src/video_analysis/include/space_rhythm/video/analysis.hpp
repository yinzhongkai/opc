#pragma once

#include <space_rhythm/core/timeline.hpp>
#include <space_rhythm/media/media.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace space_rhythm::video
{

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::uint32_t motion_curve_schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::string_view algorithm_id{"space-rhythm.video-analysis.classic"};
inline constexpr std::string_view algorithm_version{"1.0.0"};
inline constexpr std::string_view parameter_set_id{"space-rhythm.video-analysis.production"};
inline constexpr std::string_view parameter_set_version{"1.0.0"};

enum class AnalysisStatus
{
    completed,
    low_quality,
    cancelled,
    failed,
};

struct AnalysisRequest
{
    std::uint32_t schema_version{video::schema_version};
    std::string video_analysis_contract_version{contract_version};
    std::string job_id;
    std::string source_fingerprint_sha256;
    media::StreamKey stream_key;
    std::string media_contract_version{media::contract_version};
    std::uint32_t media_schema_version{media::schema_version};
    std::string algorithm_id{video::algorithm_id};
    std::string algorithm_version{video::algorithm_version};
    std::string parameter_set_id{video::parameter_set_id};
    std::uint32_t parameter_schema_version{1};
    std::string parameters_digest_sha256;
    std::uint64_t deterministic_seed{0x5350414345524859ULL};
    core::AnalysisRevision analysis_revision;
    std::optional<core::TimeRange> input_range_ns;
};

struct AnalysisParameters
{
    std::uint32_t schema_version{video::schema_version};
    std::uint32_t parameter_schema_version{1};
    std::string id{parameter_set_id};
    std::string version{parameter_set_version};
    std::uint32_t histogram_bins{32};
    std::uint32_t hard_cut_histogram_threshold_ppm{420'000};
    std::uint32_t hard_cut_structural_threshold_ppm{25'000};
    std::uint32_t gradual_minimum_change_ppm{9'000};
    std::uint32_t gradual_maximum_change_ppm{180'000};
    std::uint32_t gradual_minimum_frames{6};
    std::uint32_t flash_luma_threshold_ppm{500'000};
    std::uint32_t flash_recovery_threshold_ppm{45'000};
    std::uint32_t motion_pixels_full_scale_milli{8'000};
    std::uint32_t motion_peak_threshold_ppm{300'000};
    std::uint32_t action_peak_threshold_ppm{100'000};
    std::uint32_t minimum_peak_prominence_ppm{50'000};
    std::uint32_t minimum_peak_spacing_ms{250};
    std::uint32_t stop_history_window_ms{500};
    std::uint32_t stop_history_guard_ms{100};
    std::uint32_t stop_lookahead_ms{250};
    std::uint32_t stop_minimum_motion_ppm{10'000};
    std::uint32_t stop_maximum_remaining_ratio_ppm{500'000};
    std::uint32_t cut_suppression_window_ms{100};
    std::uint32_t low_contrast_stddev_milli{5'000};
    std::uint32_t compression_noise_floor_ppm{2'000};
    std::uint32_t compression_noise_ceiling_ppm{35'000};
    core::DurationNs maximum_sampling_gap_ns{250'000'000};
    std::uint64_t deterministic_seed{0x5350414345524859ULL};
    std::string parameters_digest_sha256;

    bool operator==(const AnalysisParameters&) const = default;
};

struct AnalysisLimits
{
    std::uint64_t max_input_frames{1'000'000};
    std::uint64_t max_frame_bytes{16ULL << 20U};
    std::uint64_t max_working_bytes{256ULL << 20U};
    std::uint64_t max_candidates{100'000};
};

struct AnalysisDiagnostic
{
    std::string code;
    std::optional<core::TimeRange> range_ns;
    std::vector<std::pair<std::string, std::string>> context;

    bool operator==(const AnalysisDiagnostic&) const = default;
};

struct MotionCurveSample
{
    core::TimeNs previous_time_ns{};
    core::TimeNs time_ns{};
    std::uint64_t segment{};
    core::NormPpm global_motion_ppm{};
    core::NormPpm local_residual_ppm{};
    core::NormPpm motion_value_ppm{};
    core::NormPpm action_value_ppm{};

    bool operator==(const MotionCurveSample&) const = default;
};

struct AnalysisResult
{
    std::uint32_t schema_version{video::schema_version};
    std::string video_analysis_contract_version{contract_version};
    std::string core_contract_version{core::contract_version};
    std::uint32_t core_schema_version{core::schema_version};
    std::string media_contract_version{media::contract_version};
    std::uint32_t media_schema_version{media::schema_version};
    AnalysisStatus status{AnalysisStatus::failed};
    std::string job_id;
    std::string source_fingerprint_sha256;
    media::StreamKey stream_key;
    std::uint64_t deterministic_seed{};
    core::AnalysisRevision analysis_revision;
    std::optional<core::TimeRange> input_range_ns;
    std::string algorithm_id{video::algorithm_id};
    std::string algorithm_version{video::algorithm_version};
    std::string parameter_set_id{video::parameter_set_id};
    std::uint32_t parameter_schema_version{1};
    std::string opencv_version;
    std::string parameters_digest_sha256;
    std::uint32_t motion_curve_schema_version{video::motion_curve_schema_version};
    std::uint64_t analyzed_frame_count{};
    std::uint64_t segment_count{};
    std::uint64_t estimated_peak_working_bytes{};
    std::vector<MotionCurveSample> motion_curve_samples;
    std::vector<core::AnalysisCandidate> candidates;
    std::vector<AnalysisDiagnostic> diagnostics;
    std::optional<core::ErrorInfo> error;
};

[[nodiscard]] AnalysisParameters production_parameters();
[[nodiscard]] std::string canonical_parameters_json(const AnalysisParameters& parameters);
[[nodiscard]] std::string_view opencv_runtime_version() noexcept;

class Analyzer final
{
  public:
    [[nodiscard]] AnalysisResult
    analyze(std::span<const media::VideoFrame> frames, const AnalysisRequest& request,
            const AnalysisParameters& parameters, const AnalysisLimits& limits = {},
            const core::CancellationToken* cancellation = nullptr) const;
};

} // namespace space_rhythm::video
