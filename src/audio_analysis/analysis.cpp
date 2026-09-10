#include <space_rhythm/audio/analysis.hpp>

#include <kiss_fft.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <sstream>
#include <tuple>
#include <utility>

namespace space_rhythm::audio {
namespace {

constexpr std::string_view kIdentityDigestPrefix{"identity-v1|sampleRate="};
constexpr double kMeasurementScale = 1'000'000'000'000.0;
constexpr double kPpmScale = 1'000'000.0;
constexpr double kPi = 3.141592653589793238462643383279502884;

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
    0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
    0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
    0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
    0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
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
    while ((padded.size() % 64U) != 56U) {
        padded.push_back(std::byte{0});
    }
    const auto bit_count = static_cast<std::uint64_t>(input.size()) * 8U;
    for (int shift = 56; shift >= 0; shift -= 8) {
        padded.push_back(static_cast<std::byte>((bit_count >> shift) & 0xffU));
    }

    for (std::size_t offset = 0; offset < padded.size(); offset += 64U) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16U; ++index) {
            const auto byte_offset = offset + index * 4U;
            words[index] = (std::to_integer<std::uint32_t>(padded[byte_offset]) << 24U)
                | (std::to_integer<std::uint32_t>(padded[byte_offset + 1U]) << 16U)
                | (std::to_integer<std::uint32_t>(padded[byte_offset + 2U]) << 8U)
                | std::to_integer<std::uint32_t>(padded[byte_offset + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            const auto s0 = rotate_right(words[index - 15U], 7U)
                ^ rotate_right(words[index - 15U], 18U) ^ (words[index - 15U] >> 3U);
            const auto s1 = rotate_right(words[index - 2U], 17U)
                ^ rotate_right(words[index - 2U], 19U) ^ (words[index - 2U] >> 10U);
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
        for (std::size_t index = 0; index < words.size(); ++index) {
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
    for (const auto value : hash) {
        output << std::setw(8) << value;
    }
    return output.str();
}

[[nodiscard]] std::string sha256_hex(const std::string_view text)
{
    return sha256_hex(std::as_bytes(std::span{text.data(), text.size()}));
}

[[nodiscard]] bool is_lower_sha256(const std::string_view text) noexcept
{
    return text.size() == 64U
        && std::ranges::all_of(text, [](const char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f');
           });
}

[[nodiscard]] core::ErrorInfo make_error(const core::ErrorCategory category,
                                         const core::ErrorCode code,
                                         std::string stage,
                                         std::string reason)
{
    core::ErrorInfo error;
    error.category = category;
    error.code = code;
    error.stage = std::move(stage);
    error.diagnostic_id = "audio-analysis-" + std::string{core::to_string(code)};
    error.message_key = "audio.analysis." + std::string{core::to_string(code)};
    error.context.emplace("reason", std::move(reason));
    return error;
}

[[nodiscard]] core::Result<core::TimeNs> sample_time_ns(
    const std::int64_t sample_index,
    const std::int64_t origin_sample_index,
    const core::TimeNs origin_time_ns,
    const std::uint32_t sample_rate,
    const core::RoundingMode rounding)
{
    const auto delta = core::checked_subtract(sample_index, origin_sample_index);
    if (!delta) {
        return core::Result<core::TimeNs>::failure(delta.error());
    }
    const auto offset = core::scale_ticks(delta.value(), {1, sample_rate}, rounding);
    if (!offset) {
        return core::Result<core::TimeNs>::failure(offset.error());
    }
    return core::checked_add(origin_time_ns, offset.value());
}

[[nodiscard]] bool checked_multiply(const std::uint64_t left,
                                    const std::uint64_t right,
                                    std::uint64_t& result) noexcept
{
    if (left != 0U && right > std::numeric_limits<std::uint64_t>::max() / left) {
        return false;
    }
    result = left * right;
    return true;
}

[[nodiscard]] core::NormPpm to_ppm(const double value) noexcept
{
    if (!(value > 0.0)) {
        return 0U;
    }
    if (value >= 1.0) {
        return core::norm_ppm_max;
    }
    return static_cast<core::NormPpm>(std::llround(value * kPpmScale));
}

[[nodiscard]] FeatureMeasurement measurement(std::string definition,
                                             const double value,
                                             std::string unit,
                                             const bool normalized)
{
    FeatureMeasurement result;
    result.definition_id = std::move(definition);
    result.mantissa = static_cast<std::int64_t>(std::llround(value * kMeasurementScale));
    result.unit = std::move(unit);
    if (normalized) {
        result.normalized_ppm = to_ppm(value);
    }
    return result;
}

[[nodiscard]] FeatureMeasurement invalid_measurement(std::string definition,
                                                     std::string unit,
                                                     std::string reason)
{
    FeatureMeasurement result;
    result.definition_id = std::move(definition);
    result.unit = std::move(unit);
    result.valid = false;
    result.invalid_reason = std::move(reason);
    return result;
}

[[nodiscard]] std::vector<double> hann_window(const std::uint32_t size)
{
    std::vector<double> result(size);
    for (std::uint32_t index = 0; index < size; ++index) {
        result[index] = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(index)
                                             / static_cast<double>(size));
    }
    return result;
}

[[nodiscard]] std::string window_digest(const std::vector<double>& window)
{
    std::vector<std::byte> bytes(window.size() * sizeof(double));
    std::memcpy(bytes.data(), window.data(), bytes.size());
    return sha256_hex(std::span<const std::byte>{bytes});
}

[[nodiscard]] bool is_power_of_two(const std::uint32_t value) noexcept
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

[[nodiscard]] core::Result<std::uint64_t> total_frames(
    const std::span<const DspPcmBuffer> buffers)
{
    std::uint64_t total = 0U;
    for (const auto& buffer : buffers) {
        if (buffer.valid_frame_count > std::numeric_limits<std::uint64_t>::max() - total) {
            return core::Result<std::uint64_t>::failure(make_error(
                core::ErrorCategory::validation,
                core::ErrorCode::invalid_pcm_buffer,
                "audio.analysis.input",
                "input_frame_count_overflow"));
        }
        total += buffer.valid_frame_count;
    }
    return core::Result<std::uint64_t>::success(total);
}

[[nodiscard]] core::Result<std::uint64_t> feature_count(
    const std::span<const DspPcmBuffer> buffers,
    const std::uint32_t hop)
{
    std::uint64_t count = 0U;
    for (std::size_t index = 0; index < buffers.size();) {
        const auto segment_id = buffers[index].segment_id;
        std::uint64_t frames = 0U;
        while (index < buffers.size() && buffers[index].segment_id == segment_id) {
            if (buffers[index].valid_frame_count
                > std::numeric_limits<std::uint64_t>::max() - frames) {
                return core::Result<std::uint64_t>::failure(make_error(
                    core::ErrorCategory::validation,
                    core::ErrorCode::invalid_pcm_buffer,
                    "audio.analysis.input",
                    "segment_frame_count_overflow"));
            }
            frames += buffers[index].valid_frame_count;
            ++index;
        }
        if (frames != 0U) {
            const auto segment_count = 1U + (frames - 1U) / hop;
            if (segment_count > std::numeric_limits<std::uint64_t>::max() - count) {
                return core::Result<std::uint64_t>::failure(make_error(
                    core::ErrorCategory::validation,
                    core::ErrorCode::invalid_pcm_buffer,
                    "audio.analysis.input",
                    "feature_frame_count_overflow"));
            }
            count += segment_count;
        }
    }
    return core::Result<std::uint64_t>::success(count);
}

[[nodiscard]] std::optional<core::ErrorInfo> validate_parameters(
    const AnalysisParameters& parameters)
{
    if (parameters.schema_version != schema_version || parameters.parameter_schema_version != 1U) {
        return make_error(core::ErrorCategory::compatibility,
                          core::ErrorCode::unsupported_parameter_schema,
                          "audio.analysis.parameters",
                          "schema_version");
    }
    if (parameters.sample_rate == 0U || parameters.frame_length_frames == 0U
        || parameters.hop_length_frames == 0U
        || parameters.fft_size != parameters.frame_length_frames
        || !is_power_of_two(parameters.fft_size)
        || parameters.onset_amplitude_threshold_ppm > core::norm_ppm_max
        || parameters.signal_floor_ppm > core::norm_ppm_max
        || parameters.period_tolerance_ppm > core::norm_ppm_max
        || parameters.minimum_rhythmic_onsets < 3U
        || parameters.minimum_tempo_millibpm == 0U
        || parameters.maximum_tempo_millibpm < parameters.minimum_tempo_millibpm) {
        return make_error(core::ErrorCategory::validation,
                          core::ErrorCode::invalid_analysis_parameters,
                          "audio.analysis.parameters",
                          "invalid_scalar_parameter");
    }
    std::uint64_t previous_high = 0U;
    for (const auto& band : parameters.bands) {
        if (band.id.empty() || band.low_millihz_inclusive < previous_high
            || band.high_millihz_exclusive <= band.low_millihz_inclusive
            || band.high_millihz_exclusive
                > static_cast<std::uint64_t>(parameters.sample_rate) * 500U) {
            return make_error(core::ErrorCategory::validation,
                              core::ErrorCode::invalid_analysis_parameters,
                              "audio.analysis.parameters",
                              "invalid_band_definition");
        }
        previous_high = band.high_millihz_exclusive;
    }
    if (!is_lower_sha256(parameters.window_coefficients_digest_sha256)
        || !is_lower_sha256(parameters.parameters_digest_sha256)
        || parameters.parameters_digest_sha256 != sha256_hex(canonical_parameters_json(parameters))) {
        return make_error(core::ErrorCategory::validation,
                          core::ErrorCode::invalid_analysis_parameters,
                          "audio.analysis.parameters",
                          "parameter_digest_mismatch");
    }
    return std::nullopt;
}

struct SegmentAccessor {
    std::span<const DspPcmBuffer> buffers;
    std::int64_t first_sample_index{};
    std::int64_t end_sample_index{};
    std::uint32_t channels{};

    [[nodiscard]] core::Result<float> sample(const std::int64_t index,
                                             const std::uint32_t channel) const
    {
        for (const auto& buffer : buffers) {
            if (index >= buffer.first_sample_index
                && static_cast<std::uint64_t>(index - buffer.first_sample_index)
                    < buffer.valid_frame_count) {
                return buffer.sample(static_cast<std::uint64_t>(index - buffer.first_sample_index),
                                     channel);
            }
        }
        return core::Result<float>::failure(make_error(core::ErrorCategory::validation,
                                                       core::ErrorCode::pcm_discontinuity,
                                                       "audio.analysis.read",
                                                       "sample_not_covered"));
    }
};

[[nodiscard]] ProducerSource make_source(const DspPcmBuffer& buffer,
                                         const AnalysisParameters& parameters)
{
    ProducerSource source;
    source.input_fingerprint_sha256 = buffer.input_fingerprint_sha256;
    source.parameters_digest_sha256 = parameters.parameters_digest_sha256;
    source.deterministic_seed = parameters.deterministic_seed;
    return source;
}

[[nodiscard]] std::string revision_value(const std::span<const DspPcmBuffer> buffers,
                                         const AnalysisParameters& parameters)
{
    std::ostringstream canonical;
    canonical << "algorithm=" << algorithm_id << '@' << algorithm_version
              << "|backend=" << fft_backend_id << '@' << fft_backend_version
              << "|parameters=" << parameters.parameters_digest_sha256
              << "|seed=" << parameters.deterministic_seed;
    for (const auto& buffer : buffers) {
        canonical << "|input=" << buffer.input_fingerprint_sha256
                  << ':' << buffer.stream_key.stream_index
                  << ':' << buffer.segment_id
                  << ':' << buffer.first_sample_index
                  << ':' << buffer.valid_frame_count
                  << ':' << buffer.format_epoch;
    }
    return "dsp-analysis-" + sha256_hex(canonical.str());
}

[[nodiscard]] std::string sample_token(const std::int64_t sample_index)
{
    if (sample_index >= 0) {
        return std::to_string(sample_index);
    }
    if (sample_index == std::numeric_limits<std::int64_t>::min()) {
        return "n9223372036854775808";
    }
    return "n" + std::to_string(-sample_index);
}

void append_unique(std::vector<std::string>& values, std::string value)
{
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

struct DetectedOnset {
    std::int64_t sample_index{};
    double amplitude{};
};

struct PeriodCluster {
    std::int64_t representative{};
    std::uint32_t count{};
};

[[nodiscard]] bool within_tolerance(const std::int64_t value,
                                    const std::int64_t reference,
                                    const std::uint32_t tolerance_ppm) noexcept
{
    const auto difference = static_cast<std::uint64_t>(value > reference ? value - reference
                                                                         : reference - value);
    const auto reference_unsigned = static_cast<std::uint64_t>(reference);
    return difference * 1'000'000ULL <= reference_unsigned * tolerance_ppm;
}

[[nodiscard]] std::uint32_t tempo_millibpm(const std::uint32_t sample_rate,
                                           const std::int64_t period_frames) noexcept
{
    const auto numerator = static_cast<std::uint64_t>(sample_rate) * 60'000ULL;
    const auto denominator = static_cast<std::uint64_t>(period_frames);
    return static_cast<std::uint32_t>((numerator + denominator / 2U) / denominator);
}

struct KissFftDeleter {
    void operator()(kiss_fft_state* state) const noexcept
    {
        kiss_fft_free(state);
    }
};

} // namespace

ResampleTrace ResampleTrace::identity(const std::uint32_t sample_rate)
{
    ResampleTrace trace;
    trace.input_sample_rate = sample_rate;
    trace.output_sample_rate = sample_rate;
    trace.implementation_id = "identity";
    trace.implementation_version = "1";
    trace.parameters_digest_sha256 = sha256_hex(
        std::string{kIdentityDigestPrefix} + std::to_string(sample_rate));
    return trace;
}

std::uint32_t DspPcmBuffer::channel_count() const noexcept
{
    return static_cast<std::uint32_t>(channel_order.size());
}

core::Result<float> DspPcmBuffer::sample(const std::uint64_t frame,
                                        const std::uint32_t channel) const
{
    if (frame >= valid_frame_count || channel >= channel_count()) {
        return core::Result<float>::failure(make_error(core::ErrorCategory::validation,
                                                       core::ErrorCode::invalid_pcm_buffer,
                                                       "audio.pcm.sample",
                                                       "sample_out_of_bounds"));
    }
    std::uint64_t frame_offset = 0U;
    if (!checked_multiply(frame, frame_stride_bytes, frame_offset)) {
        return core::Result<float>::failure(make_error(core::ErrorCategory::validation,
                                                       core::ErrorCode::invalid_pcm_buffer,
                                                       "audio.pcm.sample",
                                                       "sample_offset_overflow"));
    }
    const auto channel_offset = static_cast<std::uint64_t>(channel) * sizeof(float);
    if (offset_bytes > std::numeric_limits<std::uint64_t>::max() - frame_offset
        || offset_bytes + frame_offset
            > std::numeric_limits<std::uint64_t>::max() - channel_offset) {
        return core::Result<float>::failure(make_error(core::ErrorCategory::validation,
                                                       core::ErrorCode::invalid_pcm_buffer,
                                                       "audio.pcm.sample",
                                                       "sample_offset_overflow"));
    }
    const auto offset = offset_bytes + frame_offset + channel_offset;
    const auto bytes = lease.bytes();
    if (offset > bytes.size() || bytes.size() - offset < sizeof(float)) {
        return core::Result<float>::failure(make_error(core::ErrorCategory::validation,
                                                       core::ErrorCode::invalid_pcm_buffer,
                                                       "audio.pcm.sample",
                                                       "lease_view_out_of_bounds"));
    }
    float value = 0.0F;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return core::Result<float>::success(value);
}

core::Result<DspPcmBuffer> PcmNarrowAdapter::adapt(
    const media::PcmBuffer& source,
    const PcmAdapterContext& context)
{
    const auto fail = [](const core::ErrorCategory category,
                         const core::ErrorCode code,
                         const std::string& reason) {
        return core::Result<DspPcmBuffer>::failure(
            make_error(category, code, "audio.pcm.adapt", reason));
    };
    if (!context.resample_trace.has_value()) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::resample_timing_unavailable,
                    "missing_resample_trace");
    }
    if (source.sample_format != "flt" || source.planar) {
        return fail(core::ErrorCategory::compatibility,
                    core::ErrorCode::unsupported_pcm_format,
                    "requires_interleaved_flt");
    }
    std::vector<std::string> channel_order;
    if (source.channel_layout == "mono") {
        channel_order.emplace_back("FC");
    } else if (source.channel_layout == "stereo") {
        channel_order = {"FL", "FR"};
    } else {
        return fail(core::ErrorCategory::compatibility,
                    core::ErrorCode::unsupported_channel_layout,
                    "requires_mono_or_stereo");
    }
    if (source.sample_rate == 0U || source.sample_count == 0U || source.segment_id.empty()
        || !source.lease || source.planes.size() != 1U
        || !is_lower_sha256(source.stream_key.source_fingerprint_sha256)) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::invalid_pcm_buffer,
                    "invalid_scalar_or_identity_field");
    }
    const auto& plane = source.planes.front();
    const auto frame_stride = static_cast<std::uint32_t>(channel_order.size() * sizeof(float));
    std::uint64_t expected_bytes = 0U;
    if (!checked_multiply(source.sample_count, frame_stride, expected_bytes)
        || source.sample_count > std::numeric_limits<std::uint32_t>::max()
        || plane.row_bytes != frame_stride
        || plane.rows < source.sample_count
        || plane.valid_bytes != expected_bytes
        || plane.offset_bytes > source.lease.byte_size()
        || expected_bytes > source.lease.byte_size() - plane.offset_bytes) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::invalid_pcm_buffer,
                    "invalid_plane_or_lease_view");
    }

    const auto& trace = *context.resample_trace;
    if (trace.input_sample_rate == 0U || trace.output_sample_rate != source.sample_rate
        || trace.implementation_id.empty() || trace.implementation_version.empty()
        || !is_lower_sha256(trace.parameters_digest_sha256)
        || trace.delay_before_input_frames.numerator < 0
        || trace.delay_before_input_frames.denominator <= 0
        || trace.delay_unit != "input_frames"
        || !trace.delay_accounted_in_first_sample_index) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::resample_timing_unavailable,
                    "incomplete_or_inconsistent_resample_trace");
    }
    if (!trace.performed
        && (trace.input_sample_rate != trace.output_sample_rate
            || trace.implementation_id != "identity" || trace.implementation_version != "1"
            || trace.delay_before_input_frames != RationalFrames{0, 1}
            || trace.emitted_from_drain)) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::resample_timing_unavailable,
                    "invalid_identity_trace");
    }

    const auto expected_time = sample_time_ns(source.first_sample_index,
                                              context.segment_origin_sample_index,
                                              context.segment_origin_time_ns,
                                              source.sample_rate,
                                              core::RoundingMode::nearest_ties_to_even);
    const auto expected_duration = core::scale_ticks(
        static_cast<std::int64_t>(source.sample_count),
        {1, source.sample_rate},
        core::RoundingMode::nearest_ties_to_even);
    if (!expected_time || !expected_duration) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::time_overflow,
                    "sample_time_overflow");
    }
    if (source.time_ns != expected_time.value() || source.duration_ns != expected_duration.value()) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::timestamp_mismatch,
                    "media_time_does_not_match_sample_index");
    }

    if (previous_.has_value() && previous_->segment_id == source.segment_id) {
        if (previous_->stream_key != source.stream_key
            || previous_->format_epoch != source.lease.format_epoch()
            || previous_->segment_origin_time_ns != context.segment_origin_time_ns
            || previous_->segment_origin_sample_index != context.segment_origin_sample_index
            || previous_->next_sample_index != source.first_sample_index
            || previous_->sample_rate != source.sample_rate
            || previous_->channel_order != channel_order
            || previous_->resample_trace != trace) {
            return fail(core::ErrorCategory::validation,
                        core::ErrorCode::pcm_discontinuity,
                        "same_segment_metadata_or_index_changed");
        }
    }
    if (source.sample_count > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::time_overflow,
                    "next_sample_index_overflow");
    }
    const auto next_sample_index = core::checked_add(
        source.first_sample_index, static_cast<std::int64_t>(source.sample_count));
    if (!next_sample_index) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::time_overflow,
                    "next_sample_index_overflow");
    }

    DspPcmBuffer result;
    result.input_fingerprint_sha256 = source.stream_key.source_fingerprint_sha256;
    result.stream_key = source.stream_key;
    result.segment_id = source.segment_id;
    result.format_epoch = source.lease.format_epoch();
    result.segment_origin_time_ns = context.segment_origin_time_ns;
    result.segment_origin_sample_index = context.segment_origin_sample_index;
    result.first_sample_index = source.first_sample_index;
    result.valid_frame_count = source.sample_count;
    result.sample_rate = source.sample_rate;
    result.channel_order = channel_order;
    result.frame_stride_bytes = frame_stride;
    result.offset_bytes = plane.offset_bytes;
    result.valid_bytes = expected_bytes;
    result.resample_trace = trace;
    result.lease = source.lease;

    previous_ = PreviousBuffer{source.stream_key,
                               source.segment_id,
                               source.lease.format_epoch(),
                               context.segment_origin_time_ns,
                               context.segment_origin_sample_index,
                               next_sample_index.value(),
                               source.sample_rate,
                               std::move(channel_order),
                               trace};
    return core::Result<DspPcmBuffer>::success(std::move(result));
}

void PcmNarrowAdapter::reset() noexcept
{
    previous_.reset();
}

AnalysisParameters production_parameters(const std::uint32_t sample_rate)
{
    AnalysisParameters parameters;
    parameters.sample_rate = sample_rate;
    parameters.bands = {
        {"low", 0U, 200'000U},
        {"mid", 200'000U, 2'000'000U},
        {"high", 2'000'000U, 20'000'000U},
    };
    const auto window = hann_window(parameters.frame_length_frames);
    parameters.window_coefficients_digest_sha256 = window_digest(window);
    parameters.parameters_digest_sha256 = sha256_hex(canonical_parameters_json(parameters));
    return parameters;
}

std::string canonical_parameters_json(const AnalysisParameters& parameters)
{
    std::ostringstream json;
    json << "{\"algorithmId\":\"" << algorithm_id
         << "\",\"algorithmVersion\":\"" << algorithm_version << "\",\"bands\":[";
    for (std::size_t index = 0; index < parameters.bands.size(); ++index) {
        if (index != 0U) {
            json << ',';
        }
        const auto& band = parameters.bands[index];
        json << "{\"highMilliHzExclusive\":" << band.high_millihz_exclusive
             << ",\"id\":\"" << band.id << "\",\"lowMilliHzInclusive\":"
             << band.low_millihz_inclusive << '}';
    }
    json << "],\"beat\":{\"maximumTempoMilliBpm\":" << parameters.maximum_tempo_millibpm
         << ",\"minimumRhythmicOnsets\":" << parameters.minimum_rhythmic_onsets
         << ",\"minimumTempoMilliBpm\":" << parameters.minimum_tempo_millibpm
         << ",\"periodTolerancePpm\":" << parameters.period_tolerance_ppm
         << "},\"boundaryPolicy\":\"zero_pad\",\"channelAggregation\":\"mean_energy\""
         << ",\"confidence\":{\"id\":\"classic-ppm-v1\"}"
         << ",\"deterministicSeed\":" << parameters.deterministic_seed
         << ",\"energyDefinitionId\":\"mean-square-per-channel-v1\""
         << ",\"fft\":{\"backendId\":\"" << fft_backend_id
         << "\",\"backendVersion\":\"" << fft_backend_version
         << "\",\"feature\":\"" << fft_backend_feature
         << "\",\"normalization\":\"one-sided-window-power-v1\",\"size\":"
         << parameters.fft_size << "},\"frameLengthFrames\":"
         << parameters.frame_length_frames << ",\"hopLengthFrames\":"
         << parameters.hop_length_frames << ",\"onset\":{\"amplitudeThresholdPpm\":"
         << parameters.onset_amplitude_threshold_ppm << ",\"minimumSpacingFrames\":"
         << parameters.onset_minimum_spacing_frames << "},\"parameterSchemaVersion\":"
         << parameters.parameter_schema_version << ",\"parameterSetId\":\"" << parameters.id
         << "\",\"parameterSetVersion\":\"" << parameters.version
         << "\",\"requiredFeatures\":[\"band-energy-v1\",\"short-time-energy-v1\","
            "\"spectral-change-v1\"],\"sampleRate\":" << parameters.sample_rate
         << ",\"signalFloorPpm\":" << parameters.signal_floor_ppm
         << ",\"smoothing\":{\"id\":\"none\",\"radiusFrames\":0,\"version\":\"1\"}"
         << ",\"spectralChangeDefinitionId\":\"positive-magnitude-flux-v1\""
         << ",\"window\":{\"coefficientsDigestSha256\":\""
         << parameters.window_coefficients_digest_sha256
         << "\",\"id\":\"hann-periodic\",\"version\":\"1\"}}";
    return json.str();
}

AnalysisResult Analyzer::analyze(const std::span<const DspPcmBuffer> buffers,
                                 const AnalysisParameters& parameters,
                                 const AnalysisLimits& limits,
                                 const core::CancellationToken* cancellation) const
{
    AnalysisResult result;
    const auto fail = [&](const core::ErrorCategory category,
                          const core::ErrorCode code,
                          const std::string& reason) {
        result.status = AnalysisStatus::failed;
        result.error = make_error(category, code, "audio.analysis", reason);
        result.feature_frames.clear();
        result.candidates.clear();
        return result;
    };
    const auto cancel = [&]() {
        result.status = AnalysisStatus::cancelled;
        result.error = make_error(core::ErrorCategory::cancelled,
                                  core::ErrorCode::cancelled,
                                  "audio.analysis",
                                  "cancelled");
        result.reason_codes = {"cancelled"};
        result.feature_frames.clear();
        result.candidates.clear();
        return result;
    };

    if (const auto error = validate_parameters(parameters); error.has_value()) {
        result.error = *error;
        return result;
    }
    if (buffers.empty()) {
        result.status = AnalysisStatus::no_signal;
        result.reason_codes = {"silence"};
        result.overall_confidence_ppm = 0U;
        return result;
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        return cancel();
    }
    const auto frame_total = total_frames(buffers);
    const auto features_total = feature_count(buffers, parameters.hop_length_frames);
    if (!frame_total || !features_total) {
        return fail(core::ErrorCategory::validation,
                    core::ErrorCode::invalid_pcm_buffer,
                    "count_overflow");
    }
    if (frame_total.value() > limits.max_input_frames
        || features_total.value() > limits.max_feature_frames) {
        return fail(core::ErrorCategory::resource_limit,
                    core::ErrorCode::resource_limit,
                    "configured_analysis_limit_exceeded");
    }
    const auto spaced_candidate_count = frame_total.value()
        / std::max<std::uint32_t>(1U, parameters.onset_minimum_spacing_frames);
    if (spaced_candidate_count == std::numeric_limits<std::uint64_t>::max()) {
        return fail(core::ErrorCategory::resource_limit,
                    core::ErrorCode::resource_limit,
                    "analysis_size_overflow");
    }
    const auto candidate_capacity = spaced_candidate_count + 1U;
    const auto working_buffers = static_cast<std::uint64_t>(parameters.fft_size)
        * (sizeof(kiss_fft_cpx) * 2U + sizeof(double) * 3U);
    std::uint64_t feature_bytes = 0U;
    std::uint64_t candidate_bytes = 0U;
    if (!checked_multiply(
            features_total.value(),
            sizeof(AudioFeatureFrame) + parameters.bands.size() * sizeof(BandEnergy),
            feature_bytes)
        || !checked_multiply(candidate_capacity,
                             sizeof(AnalysisCandidate),
                             candidate_bytes)
        || feature_bytes > std::numeric_limits<std::uint64_t>::max() - candidate_bytes
        || working_buffers > std::numeric_limits<std::uint64_t>::max()
                - (feature_bytes + candidate_bytes)) {
        return fail(core::ErrorCategory::resource_limit,
                    core::ErrorCode::resource_limit,
                    "analysis_size_overflow");
    }
    const auto estimated_result_bytes = feature_bytes + candidate_bytes;
    result.estimated_peak_working_bytes = working_buffers + estimated_result_bytes;
    if (result.estimated_peak_working_bytes > limits.max_working_bytes) {
        return fail(core::ErrorCategory::resource_limit,
                    core::ErrorCode::resource_limit,
                    "configured_analysis_limit_exceeded");
    }

    for (std::size_t index = 0; index < buffers.size(); ++index) {
        const auto& buffer = buffers[index];
        if (buffer.schema_version != schema_version || buffer.dsp_contract_version != contract_version
            || buffer.media_contract_version != media::contract_version
            || buffer.sample_rate != parameters.sample_rate || buffer.valid_frame_count == 0U
            || buffer.sample_format != "f32_le" || !buffer.interleaved
            || !is_lower_sha256(buffer.input_fingerprint_sha256)) {
            return fail(core::ErrorCategory::validation,
                        core::ErrorCode::invalid_pcm_buffer,
                        "buffer_contract_or_sample_rate_mismatch");
        }
        if (index != 0U && buffers[index - 1U].segment_id == buffer.segment_id) {
            const auto& previous = buffers[index - 1U];
            if (previous.valid_frame_count
                    > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "analysis_input_index_overflow");
            }
            const auto expected_next = core::checked_add(
                previous.first_sample_index,
                static_cast<std::int64_t>(previous.valid_frame_count));
            if (!expected_next || expected_next.value() != buffer.first_sample_index
                || previous.sample_rate != buffer.sample_rate
                || previous.channel_order != buffer.channel_order
                || previous.format_epoch != buffer.format_epoch
                || previous.resample_trace != buffer.resample_trace) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::pcm_discontinuity,
                            "analysis_input_discontinuity");
            }
        }
    }

    const core::AnalysisRevision revision{revision_value(buffers, parameters)};
    const auto window = hann_window(parameters.frame_length_frames);
    std::unique_ptr<kiss_fft_state, KissFftDeleter> fft(
        kiss_fft_alloc(static_cast<int>(parameters.fft_size), 0, nullptr, nullptr));
    if (!fft) {
        return fail(core::ErrorCategory::resource_limit,
                    core::ErrorCode::resource_limit,
                    "kissfft_plan_allocation_failed");
    }
    std::vector<kiss_fft_cpx> fft_input(parameters.fft_size);
    std::vector<kiss_fft_cpx> fft_output(parameters.fft_size);
    std::vector<double> current_magnitudes(parameters.fft_size / 2U + 1U);
    std::vector<double> previous_magnitudes(current_magnitudes.size());
    std::vector<double> accumulated_power(current_magnitudes.size());

    std::uint64_t visited_samples = 0U;
    double global_peak = 0.0;
    double global_sum_squares = 0.0;
    std::vector<DetectedOnset> all_onsets;
    std::vector<std::vector<DetectedOnset>> segment_onsets;
    std::vector<std::span<const DspPcmBuffer>> segments;

    for (std::size_t begin = 0; begin < buffers.size();) {
        std::size_t end = begin + 1U;
        while (end < buffers.size() && buffers[end].segment_id == buffers[begin].segment_id) {
            ++end;
        }
        const auto segment_buffers = buffers.subspan(begin, end - begin);
        const auto& first_buffer = segment_buffers.front();
        const auto& last_buffer = segment_buffers.back();
        if (last_buffer.valid_frame_count
            > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            return fail(core::ErrorCategory::validation,
                        core::ErrorCode::time_overflow,
                        "segment_end_index_overflow");
        }
        const auto checked_segment_end = core::checked_add(
            last_buffer.first_sample_index,
            static_cast<std::int64_t>(last_buffer.valid_frame_count));
        if (!checked_segment_end) {
            return fail(core::ErrorCategory::validation,
                        core::ErrorCode::time_overflow,
                        "segment_end_index_overflow");
        }
        const auto segment_end = checked_segment_end.value();
        SegmentAccessor accessor{segment_buffers,
                                 first_buffer.first_sample_index,
                                 segment_end,
                                 first_buffer.channel_count()};
        segments.push_back(segment_buffers);

        std::vector<DetectedOnset> onsets;
        bool active_peak = false;
        DetectedOnset current_peak;
        const auto amplitude_threshold = static_cast<double>(
            parameters.onset_amplitude_threshold_ppm) / kPpmScale;
        for (auto sample_index = accessor.first_sample_index;
             sample_index < accessor.end_sample_index;) {
            if ((visited_samples++ & 0xffU) == 0U && cancellation != nullptr
                && cancellation->is_cancelled()) {
                return cancel();
            }
            double sum_squares = 0.0;
            for (std::uint32_t channel = 0; channel < accessor.channels; ++channel) {
                const auto sample_value = accessor.sample(sample_index, channel);
                if (!sample_value) {
                    return fail(sample_value.error().category,
                                sample_value.error().code,
                                "sample_read_failed");
                }
                const auto value = static_cast<double>(sample_value.value());
                if (!std::isfinite(value)) {
                    return fail(core::ErrorCategory::validation,
                                core::ErrorCode::non_finite_pcm,
                                "non_finite_sample");
                }
                if (value < -1.0 || value > 1.0) {
                    return fail(core::ErrorCategory::validation,
                                core::ErrorCode::pcm_out_of_range,
                                "sample_out_of_range");
                }
                sum_squares += value * value;
            }
            const auto mean_square = sum_squares / static_cast<double>(accessor.channels);
            const auto amplitude = std::sqrt(mean_square);
            global_peak = std::max(global_peak, amplitude);
            global_sum_squares += mean_square;
            if (amplitude >= amplitude_threshold) {
                if (!active_peak || amplitude > current_peak.amplitude) {
                    current_peak = {sample_index, amplitude};
                }
                active_peak = true;
            } else if (active_peak) {
                if (onsets.empty()
                    || current_peak.sample_index - onsets.back().sample_index
                        >= parameters.onset_minimum_spacing_frames) {
                    onsets.push_back(current_peak);
                } else if (current_peak.amplitude > onsets.back().amplitude) {
                    onsets.back() = current_peak;
                }
                active_peak = false;
            }
            ++sample_index;
        }
        if (active_peak) {
            if (onsets.empty()
                || current_peak.sample_index - onsets.back().sample_index
                    >= parameters.onset_minimum_spacing_frames) {
                onsets.push_back(current_peak);
            } else if (current_peak.amplitude > onsets.back().amplitude) {
                onsets.back() = current_peak;
            }
        }
        all_onsets.insert(all_onsets.end(), onsets.begin(), onsets.end());
        segment_onsets.push_back(std::move(onsets));

        bool have_previous_spectrum = false;
        std::uint64_t frame_ordinal = 0U;
        for (auto window_start = accessor.first_sample_index;
             window_start < accessor.end_sample_index;) {
            if (cancellation != nullptr && cancellation->is_cancelled()) {
                return cancel();
            }
            const auto remaining = static_cast<std::uint64_t>(accessor.end_sample_index - window_start);
            const auto valid_count = static_cast<std::uint32_t>(std::min<std::uint64_t>(
                remaining, parameters.frame_length_frames));
            double energy_sum = 0.0;
            std::ranges::fill(accumulated_power, 0.0);
            for (std::uint32_t channel = 0; channel < accessor.channels; ++channel) {
                for (std::uint32_t frame = 0; frame < parameters.fft_size; ++frame) {
                    double value = 0.0;
                    if (frame < valid_count) {
                        const auto sample_index = core::checked_add(
                            window_start, static_cast<std::int64_t>(frame));
                        if (!sample_index) {
                            return fail(core::ErrorCategory::validation,
                                        core::ErrorCode::time_overflow,
                                        "window_sample_index_overflow");
                        }
                        const auto sample_value = accessor.sample(sample_index.value(), channel);
                        if (!sample_value) {
                            return fail(sample_value.error().category,
                                        sample_value.error().code,
                                        "window_sample_read_failed");
                        }
                        value = sample_value.value();
                        energy_sum += value * value;
                    }
                    fft_input[frame].r = static_cast<kiss_fft_scalar>(value * window[frame]);
                    fft_input[frame].i = static_cast<kiss_fft_scalar>(0.0F);
                }
                kiss_fft(fft.get(), fft_input.data(), fft_output.data());
                for (std::size_t bin = 0; bin < accumulated_power.size(); ++bin) {
                    const auto real = static_cast<double>(fft_output[bin].r);
                    const auto imaginary = static_cast<double>(fft_output[bin].i);
                    accumulated_power[bin] += (real * real + imaginary * imaginary)
                        / static_cast<double>(accessor.channels);
                }
            }
            const auto energy = energy_sum
                / (static_cast<double>(parameters.frame_length_frames)
                   * static_cast<double>(accessor.channels));
            const auto fft_scale = static_cast<double>(parameters.fft_size)
                * static_cast<double>(parameters.fft_size);
            for (std::size_t bin = 0; bin < accumulated_power.size(); ++bin) {
                accumulated_power[bin] /= fft_scale;
                current_magnitudes[bin] = std::sqrt(accumulated_power[bin]);
            }

            double flux = 0.0;
            if (have_previous_spectrum) {
                for (std::size_t bin = 0; bin < current_magnitudes.size(); ++bin) {
                    flux += std::max(0.0, current_magnitudes[bin] - previous_magnitudes[bin]);
                }
                flux /= static_cast<double>(current_magnitudes.size());
            }

            AudioFeatureFrame feature;
            feature.id = "dsp-feature-" + first_buffer.segment_id + '-'
                + sample_token(window_start);
            feature.analysis_revision = revision;
            feature.input_fingerprint_sha256 = first_buffer.input_fingerprint_sha256;
            feature.segment_id = first_buffer.segment_id;
            feature.window_start_sample_index = window_start;
            feature.window_frame_count = parameters.frame_length_frames;
            feature.valid_input_frame_count = valid_count;
            feature.padding_after_frames = parameters.frame_length_frames - valid_count;
            const auto doubled_start = core::checked_add(window_start, window_start);
            const auto doubled_origin = core::checked_add(first_buffer.segment_origin_sample_index,
                                                          first_buffer.segment_origin_sample_index);
            if (!doubled_start || !doubled_origin) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_anchor_overflow");
            }
            const auto anchor_numerator = core::checked_add(
                doubled_start.value(),
                static_cast<std::int64_t>(parameters.frame_length_frames) - 1);
            if (!anchor_numerator) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_anchor_overflow");
            }
            feature.anchor_sample_numerator = anchor_numerator.value();
            const auto anchor_delta = core::checked_subtract(feature.anchor_sample_numerator,
                                                             doubled_origin.value());
            if (!anchor_delta) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_anchor_overflow");
            }
            const auto anchor_offset = core::scale_ticks(anchor_delta.value(),
                                                         {1, static_cast<std::int64_t>(parameters.sample_rate) * 2},
                                                         core::RoundingMode::nearest_ties_to_even);
            const auto coverage_start = sample_time_ns(window_start,
                                                       first_buffer.segment_origin_sample_index,
                                                       first_buffer.segment_origin_time_ns,
                                                       parameters.sample_rate,
                                                       core::RoundingMode::floor);
            const auto coverage_end_index = core::checked_add(
                window_start,
                static_cast<std::int64_t>(parameters.frame_length_frames));
            if (!coverage_end_index) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_coverage_end_overflow");
            }
            const auto coverage_end = sample_time_ns(coverage_end_index.value(),
                                                     first_buffer.segment_origin_sample_index,
                                                     first_buffer.segment_origin_time_ns,
                                                     parameters.sample_rate,
                                                     core::RoundingMode::ceil);
            if (!anchor_offset || !coverage_start || !coverage_end) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_time_overflow");
            }
            const auto anchor = core::checked_add(first_buffer.segment_origin_time_ns,
                                                  anchor_offset.value());
            if (!anchor) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "feature_anchor_overflow");
            }
            feature.anchor_time_ns = anchor.value();
            feature.coverage_start_time_ns = coverage_start.value();
            feature.coverage_end_time_ns = coverage_end.value();
            feature.short_time_energy = measurement("mean-square-per-channel-v1",
                                                    energy,
                                                    "linear_power",
                                                    true);
            for (const auto& band : parameters.bands) {
                double band_power = 0.0;
                for (std::size_t bin = 0; bin < accumulated_power.size(); ++bin) {
                    const auto frequency_millihz = static_cast<std::uint64_t>(bin)
                        * parameters.sample_rate * 1000ULL / parameters.fft_size;
                    if (frequency_millihz >= band.low_millihz_inclusive
                        && frequency_millihz < band.high_millihz_exclusive) {
                        band_power += accumulated_power[bin];
                    }
                }
                feature.band_energies.push_back(
                    {band.low_millihz_inclusive,
                     band.high_millihz_exclusive,
                     measurement("one-sided-window-power-v1",
                                 band_power,
                                 "linear_power",
                                 true)});
            }
            feature.spectral_change = have_previous_spectrum
                ? measurement("positive-magnitude-flux-v1", flux, "linear_magnitude", true)
                : invalid_measurement("positive-magnitude-flux-v1",
                                      "linear_magnitude",
                                      "no_previous_frame");
            feature.source = make_source(first_buffer, parameters);
            result.feature_frames.push_back(std::move(feature));
            previous_magnitudes = current_magnitudes;
            have_previous_spectrum = true;
            ++frame_ordinal;
            const auto next_window = core::checked_add(
                window_start,
                static_cast<std::int64_t>(parameters.hop_length_frames));
            if (!next_window) {
                break;
            }
            window_start = next_window.value();
        }
        begin = end;
    }

    const auto make_candidate = [&](const DetectedOnset& onset,
                                    const CandidateKind kind,
                                    const std::uint32_t confidence,
                                    const std::optional<std::uint32_t> tempo,
                                    const std::optional<RationalFrames> period,
                                    const DspPcmBuffer& buffer) -> core::Result<AnalysisCandidate> {
        const auto time = sample_time_ns(onset.sample_index,
                                         buffer.segment_origin_sample_index,
                                         buffer.segment_origin_time_ns,
                                         buffer.sample_rate,
                                         core::RoundingMode::nearest_ties_to_even);
        if (!time) {
            return core::Result<AnalysisCandidate>::failure(time.error());
        }
        AnalysisCandidate candidate;
        candidate.id.value = "dsp-" + std::string{to_string(kind)} + '-'
            + revision.value.substr(revision.value.size() - 12U) + '-'
            + sample_token(onset.sample_index);
        candidate.analysis_revision = revision;
        candidate.kind = kind;
        candidate.segment_id = buffer.segment_id;
        candidate.sample_index = onset.sample_index;
        candidate.time_ns = time.value();
        candidate.strength_ppm = to_ppm(onset.amplitude);
        candidate.confidence_ppm = confidence;
        candidate.tempo_millibpm = tempo;
        candidate.beat_period = period;
        candidate.source = make_source(buffer, parameters);
        const auto feature = std::ranges::find_if(result.feature_frames, [&](const auto& frame) {
            return frame.segment_id == buffer.segment_id
                && onset.sample_index >= frame.window_start_sample_index
                && onset.sample_index < frame.window_start_sample_index + frame.window_frame_count;
        });
        if (feature != result.feature_frames.end()) {
            candidate.supporting_feature_frame_ids.push_back(feature->id);
        }
        return core::Result<AnalysisCandidate>::success(std::move(candidate));
    };

    for (std::size_t segment_index = 0; segment_index < segments.size(); ++segment_index) {
        const auto& segment_buffers = segments[segment_index];
        const auto& buffer = segment_buffers.front();
        const auto& onsets = segment_onsets[segment_index];
        for (const auto& onset : onsets) {
            const auto onset_candidate = make_candidate(onset,
                                                        CandidateKind::onset,
                                                        to_ppm(onset.amplitude),
                                                        std::nullopt,
                                                        std::nullopt,
                                                        buffer);
            if (!onset_candidate) {
                return fail(core::ErrorCategory::validation,
                            core::ErrorCode::time_overflow,
                            "onset_time_overflow");
            }
            result.candidates.push_back(onset_candidate.value());
        }

        bool rhythmic = onsets.size() >= parameters.minimum_rhythmic_onsets;
        std::vector<std::int64_t> periods;
        std::vector<PeriodCluster> clusters;
        if (rhythmic) {
            for (std::size_t index = 1; index < onsets.size(); ++index) {
                const auto period = onsets[index].sample_index - onsets[index - 1U].sample_index;
                if (period <= 0) {
                    rhythmic = false;
                    break;
                }
                const auto tempo = tempo_millibpm(parameters.sample_rate, period);
                if (tempo < parameters.minimum_tempo_millibpm
                    || tempo > parameters.maximum_tempo_millibpm) {
                    rhythmic = false;
                    break;
                }
                periods.push_back(period);
                const auto cluster = std::ranges::find_if(clusters, [&](const PeriodCluster& item) {
                    return within_tolerance(period,
                                            item.representative,
                                            parameters.period_tolerance_ppm);
                });
                if (cluster == clusters.end()) {
                    clusters.push_back({period, 1U});
                } else {
                    ++cluster->count;
                }
            }
            rhythmic = rhythmic && clusters.size() <= 3U
                && std::ranges::all_of(clusters, [](const PeriodCluster& cluster) {
                       return cluster.count >= 2U;
                   });
        }
        if (rhythmic) {
            const auto confidence = static_cast<core::NormPpm>(
                std::max<std::int64_t>(700'000,
                                       1'000'000
                                           - static_cast<std::int64_t>(clusters.size() - 1U)
                                               * 75'000));
            for (std::size_t index = 0; index < onsets.size(); ++index) {
                const auto period_index = index == 0U ? 0U : std::min(index - 1U,
                                                                      periods.size() - 1U);
                const auto period = periods[period_index];
                const auto beat_candidate = make_candidate(
                    onsets[index],
                    CandidateKind::beat,
                    confidence,
                    tempo_millibpm(parameters.sample_rate, period),
                    RationalFrames{period, 1},
                    buffer);
                if (!beat_candidate) {
                    return fail(core::ErrorCategory::validation,
                                core::ErrorCode::time_overflow,
                                "beat_time_overflow");
                }
                result.candidates.push_back(beat_candidate.value());
            }
        }
    }

    std::ranges::sort(result.feature_frames, [](const auto& left, const auto& right) {
        return std::tie(left.anchor_time_ns, left.id) < std::tie(right.anchor_time_ns, right.id);
    });
    std::ranges::sort(result.candidates, [](const auto& left, const auto& right) {
        return std::tuple{left.time_ns, std::string{to_string(left.kind)}, left.id.value}
            < std::tuple{right.time_ns, std::string{to_string(right.kind)}, right.id.value};
    });

    const auto rms = frame_total.value() == 0U
        ? 0.0
        : std::sqrt(global_sum_squares / static_cast<double>(frame_total.value()));
    if (global_peak < static_cast<double>(parameters.signal_floor_ppm) / kPpmScale) {
        result.status = AnalysisStatus::no_signal;
        result.reason_codes = {"silence"};
        result.overall_confidence_ppm = 0U;
        result.candidates.clear();
    } else if (std::ranges::any_of(result.candidates, [](const AnalysisCandidate& candidate) {
                   return candidate.kind == CandidateKind::beat;
               })) {
        result.status = AnalysisStatus::success;
        core::NormPpm confidence = 0U;
        for (const auto& candidate : result.candidates) {
            confidence = std::max(confidence, candidate.confidence_ppm);
        }
        result.overall_confidence_ppm = confidence;
    } else {
        result.status = AnalysisStatus::low_confidence;
        core::NormPpm confidence = 0U;
        for (const auto& onset : all_onsets) {
            confidence = std::max(confidence, to_ppm(onset.amplitude));
        }
        result.overall_confidence_ppm = confidence;
        if (frame_total.value() < parameters.frame_length_frames) {
            append_unique(result.reason_codes, "insufficient_duration");
        }
        if (all_onsets.empty()) {
            append_unique(result.reason_codes,
                          rms > global_peak * 0.25 ? "noisy" : "weak_transients");
        } else {
            append_unique(result.reason_codes, "aperiodic");
        }
    }
    return result;
}

std::string_view to_string(const AnalysisStatus status) noexcept
{
    switch (status) {
    case AnalysisStatus::success:
        return "success";
    case AnalysisStatus::low_confidence:
        return "low_confidence";
    case AnalysisStatus::no_signal:
        return "no_signal";
    case AnalysisStatus::failed:
        return "failed";
    case AnalysisStatus::cancelled:
        return "cancelled";
    }
    return "failed";
}

std::string_view to_string(const CandidateKind kind) noexcept
{
    switch (kind) {
    case CandidateKind::onset:
        return "onset";
    case CandidateKind::beat:
        return "beat";
    }
    return "onset";
}

} // namespace space_rhythm::audio
