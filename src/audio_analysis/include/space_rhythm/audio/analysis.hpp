#pragma once

#include <space_rhythm/core/timeline.hpp>
#include <space_rhythm/media/media.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::audio {

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::string_view producer_version{"0.1.0"};
inline constexpr std::string_view algorithm_id{"space-rhythm.audio-analysis.classic"};
inline constexpr std::string_view algorithm_version{"1.0.0"};
inline constexpr std::string_view parameter_set_id{"space-rhythm.audio-analysis.production"};
inline constexpr std::string_view parameter_set_version{"1.0.0"};
inline constexpr std::string_view fft_backend_id{"kissfft-float"};
inline constexpr std::string_view fft_backend_version{
    "131.2.0-vcpkg.port+cmake.131.1.0"};
inline constexpr std::string_view fft_backend_feature{"float"};

struct RationalFrames {
    std::int64_t numerator{};
    std::int64_t denominator{1};

    bool operator==(const RationalFrames&) const = default;
};

struct ResampleTrace {
    bool performed{false};
    std::uint32_t input_sample_rate{};
    std::uint32_t output_sample_rate{};
    std::string implementation_id;
    std::string implementation_version;
    std::string parameters_digest_sha256;
    RationalFrames delay_before_input_frames;
    std::string delay_unit{"input_frames"};
    bool delay_accounted_in_first_sample_index{true};
    bool emitted_from_drain{false};

    [[nodiscard]] static ResampleTrace identity(std::uint32_t sample_rate);
    bool operator==(const ResampleTrace&) const = default;
};

struct PcmAdapterContext {
    core::TimeNs segment_origin_time_ns{};
    std::int64_t segment_origin_sample_index{};
    std::optional<ResampleTrace> resample_trace;
};

struct DspPcmBuffer {
    std::uint32_t schema_version{audio::schema_version};
    std::string dsp_contract_version{contract_version};
    std::string media_contract_version{media::contract_version};
    std::string input_fingerprint_sha256;
    media::StreamKey stream_key;
    std::string segment_id;
    std::uint64_t format_epoch{};
    core::TimeNs segment_origin_time_ns{};
    std::int64_t segment_origin_sample_index{};
    std::int64_t first_sample_index{};
    std::uint64_t valid_frame_count{};
    std::uint32_t sample_rate{};
    std::string sample_format{"f32_le"};
    bool interleaved{true};
    std::vector<std::string> channel_order;
    std::uint32_t frame_stride_bytes{};
    std::uint64_t offset_bytes{};
    std::uint64_t valid_bytes{};
    ResampleTrace resample_trace;
    media::BufferLease lease;

    [[nodiscard]] std::uint32_t channel_count() const noexcept;
    [[nodiscard]] core::Result<float> sample(std::uint64_t frame,
                                             std::uint32_t channel) const;
};

class PcmNarrowAdapter final {
public:
    [[nodiscard]] core::Result<DspPcmBuffer> adapt(
        const media::PcmBuffer& source,
        const PcmAdapterContext& context);
    void reset() noexcept;

private:
    struct PreviousBuffer {
        media::StreamKey stream_key;
        std::string segment_id;
        std::uint64_t format_epoch{};
        core::TimeNs segment_origin_time_ns{};
        std::int64_t segment_origin_sample_index{};
        std::int64_t next_sample_index{};
        std::uint32_t sample_rate{};
        std::vector<std::string> channel_order;
        ResampleTrace resample_trace;
    };
    std::optional<PreviousBuffer> previous_;
};

struct BandDefinition {
    std::string id;
    std::uint64_t low_millihz_inclusive{};
    std::uint64_t high_millihz_exclusive{};

    bool operator==(const BandDefinition&) const = default;
};

struct AnalysisParameters {
    std::uint32_t schema_version{audio::schema_version};
    std::uint32_t parameter_schema_version{1};
    std::string id{parameter_set_id};
    std::string version{parameter_set_version};
    std::uint32_t sample_rate{};
    std::uint32_t frame_length_frames{1024};
    std::uint32_t hop_length_frames{256};
    std::uint32_t fft_size{1024};
    std::vector<BandDefinition> bands;
    std::uint32_t onset_amplitude_threshold_ppm{200'000};
    std::uint32_t onset_minimum_spacing_frames{64};
    std::uint32_t signal_floor_ppm{100};
    std::uint32_t period_tolerance_ppm{20'000};
    std::uint32_t minimum_rhythmic_onsets{3};
    std::uint32_t minimum_tempo_millibpm{60'000};
    std::uint32_t maximum_tempo_millibpm{300'000};
    std::uint64_t deterministic_seed{0x5350414345524859ULL};
    std::string window_coefficients_digest_sha256;
    std::string parameters_digest_sha256;

    bool operator==(const AnalysisParameters&) const = default;
};

[[nodiscard]] AnalysisParameters production_parameters(std::uint32_t sample_rate);
[[nodiscard]] std::string canonical_parameters_json(const AnalysisParameters& parameters);

struct FeatureMeasurement {
    std::string definition_id;
    std::int64_t mantissa{};
    std::int32_t decimal_scale{-12};
    std::string unit;
    std::optional<core::NormPpm> normalized_ppm;
    bool valid{true};
    std::string invalid_reason;

    bool operator==(const FeatureMeasurement&) const = default;
};

struct BandEnergy {
    std::uint64_t low_millihz_inclusive{};
    std::uint64_t high_millihz_exclusive{};
    FeatureMeasurement measurement;

    bool operator==(const BandEnergy&) const = default;
};

struct ProducerSource {
    std::string producer_id{"space-rhythm.audio-dsp"};
    std::string producer_version{audio::producer_version};
    std::string algorithm_id{audio::algorithm_id};
    std::string algorithm_version{audio::algorithm_version};
    std::string fft_backend_id{audio::fft_backend_id};
    std::string fft_backend_version{audio::fft_backend_version};
    std::string input_fingerprint_sha256;
    std::string parameter_set_id{audio::parameter_set_id};
    std::string parameter_set_version{audio::parameter_set_version};
    std::string parameters_digest_sha256;
    std::uint64_t deterministic_seed{};

    bool operator==(const ProducerSource&) const = default;
};

struct AudioFeatureFrame {
    std::uint32_t schema_version{audio::schema_version};
    std::uint32_t feature_schema_version{1};
    std::string id;
    core::AnalysisRevision analysis_revision;
    std::string input_fingerprint_sha256;
    std::string segment_id;
    std::int64_t window_start_sample_index{};
    std::uint32_t window_frame_count{};
    std::uint32_t valid_input_frame_count{};
    std::uint32_t padding_before_frames{};
    std::uint32_t padding_after_frames{};
    std::int64_t anchor_sample_numerator{};
    std::uint32_t anchor_sample_denominator{2};
    core::TimeNs anchor_time_ns{};
    core::TimeNs coverage_start_time_ns{};
    core::TimeNs coverage_end_time_ns{};
    std::string channel_aggregation{"mean_energy"};
    FeatureMeasurement short_time_energy;
    std::vector<BandEnergy> band_energies;
    FeatureMeasurement spectral_change;
    ProducerSource source;

    bool operator==(const AudioFeatureFrame&) const = default;
};

enum class CandidateKind { onset, beat };

struct AnalysisCandidate {
    std::uint32_t schema_version{audio::schema_version};
    std::uint32_t candidate_schema_version{1};
    core::CandidateId id;
    core::AnalysisRevision analysis_revision;
    CandidateKind kind{CandidateKind::onset};
    std::string segment_id;
    std::int64_t sample_index{};
    core::TimeNs time_ns{};
    core::TimeNs duration_ns{};
    core::NormPpm strength_ppm{};
    core::NormPpm confidence_ppm{};
    std::vector<std::string> supporting_feature_frame_ids;
    std::optional<std::uint32_t> tempo_millibpm;
    std::optional<RationalFrames> beat_period;
    ProducerSource source;
    std::vector<std::string> diagnostics;

    bool operator==(const AnalysisCandidate&) const = default;
};

enum class AnalysisStatus { success, low_confidence, no_signal, failed, cancelled };

struct AnalysisLimits {
    std::uint64_t max_input_frames{10'000'000};
    std::uint64_t max_feature_frames{100'000};
    std::uint64_t max_working_bytes{512ULL << 20U};
};

struct AnalysisResult {
    AnalysisStatus status{AnalysisStatus::failed};
    std::optional<core::NormPpm> overall_confidence_ppm;
    std::vector<std::string> reason_codes;
    std::vector<AudioFeatureFrame> feature_frames;
    std::vector<AnalysisCandidate> candidates;
    std::optional<core::ErrorInfo> error;
    std::uint64_t estimated_peak_working_bytes{};
};

class Analyzer final {
public:
    [[nodiscard]] AnalysisResult analyze(
        std::span<const DspPcmBuffer> buffers,
        const AnalysisParameters& parameters,
        const AnalysisLimits& limits = {},
        const core::CancellationToken* cancellation = nullptr) const;
};

[[nodiscard]] std::string_view to_string(AnalysisStatus status) noexcept;
[[nodiscard]] std::string_view to_string(CandidateKind kind) noexcept;

} // namespace space_rhythm::audio
