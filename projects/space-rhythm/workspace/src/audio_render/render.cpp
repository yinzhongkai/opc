#include <space_rhythm/audio/render.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>

namespace space_rhythm::audio::render {
namespace {

constexpr std::int64_t kQ23Scale = 8'388'608;
constexpr std::int64_t kNanosecondsPerSecond = 1'000'000'000;
constexpr std::int64_t kPpmScale = 1'000'000;

const std::array<RegisteredTestTimbre, 3> kRegisteredTimbres{{
    {"AT-CLICK-001", "test.click.linear-decay.v1",
     "timbre-click-48000-mono.f32le", "CC0-1.0",
     "c7c80114be72af053793540bc5b5b6b80e9923672670405719d0902f0618ce28",
     render_sample_rate, 960},
    {"AT-LOW-PULSE-001", "test.low-triangle.linear-decay.v1",
     "timbre-low-triangle-48000-mono.f32le", "CC0-1.0",
     "328fe2e5b80b188dfd3a3fc7af33f1801429b0754038eda9f77bc478cff75e82",
     render_sample_rate, 4'800},
    {"AT-NOISE-HIT-001", "test.noise-hit.linear-decay.v1",
     "timbre-noise-hit-48000-mono.f32le", "CC0-1.0",
     "bf7868e48350a6fcbf3b2509e15c67658f203807e141d77df077b4ac8629152a",
     render_sample_rate, 2'400},
}};

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

[[nodiscard]] core::ErrorInfo make_error(const core::ErrorCategory category,
                                         const core::ErrorCode code,
                                         std::string stage,
                                         std::string reason)
{
    core::ErrorInfo error;
    error.category = category;
    error.code = code;
    error.stage = std::move(stage);
    error.diagnostic_id = "audio-render-" + std::string{core::to_string(code)};
    error.message_key = "audio.render." + std::string{core::to_string(code)};
    error.context.emplace("reason", std::move(reason));
    return error;
}

[[nodiscard]] const RegisteredTestTimbre* find_registration(
    const std::string_view fixture_id) noexcept
{
    const auto iterator = std::ranges::find(kRegisteredTimbres, fixture_id,
                                            &RegisteredTestTimbre::fixture_id);
    return iterator == kRegisteredTimbres.end() ? nullptr : &*iterator;
}

[[nodiscard]] bool registration_matches(const RegisteredTestTimbre& left,
                                        const RegisteredTestTimbre& right) noexcept
{
    return left.fixture_id == right.fixture_id && left.timbre_id == right.timbre_id
        && left.output_file == right.output_file && left.license == right.license
        && left.pcm_sha256 == right.pcm_sha256 && left.sample_rate == right.sample_rate
        && left.frame_count == right.frame_count;
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

[[nodiscard]] bool checked_add(const std::uint64_t left,
                               const std::uint64_t right,
                               std::uint64_t& result) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        return false;
    }
    result = left + right;
    return true;
}

[[nodiscard]] std::int64_t rounded_divide_ties_to_even(const std::int64_t numerator,
                                                       const std::int64_t denominator)
{
    auto quotient = numerator / denominator;
    auto remainder = numerator % denominator;
    if (remainder < 0) {
        --quotient;
        remainder += denominator;
    }
    const auto twice_remainder = remainder * 2;
    if (twice_remainder > denominator
        || (twice_remainder == denominator && (quotient % 2) != 0)) {
        ++quotient;
    }
    return quotient;
}

[[nodiscard]] core::Result<std::int64_t> relative_frame_index(
    const core::TimeNs event_time_ns,
    const core::TimeNs start_time_ns,
    const std::uint32_t sample_rate)
{
    const auto delta_result = core::checked_subtract(event_time_ns, start_time_ns);
    if (!delta_result) {
        return core::Result<std::int64_t>::failure(delta_result.error());
    }
    const auto delta = delta_result.value();
    auto seconds = delta / kNanosecondsPerSecond;
    auto remainder = delta % kNanosecondsPerSecond;
    if (remainder < 0) {
        --seconds;
        remainder += kNanosecondsPerSecond;
    }
    if (seconds > std::numeric_limits<std::int64_t>::max()
                      / static_cast<std::int64_t>(sample_rate)
        || seconds < std::numeric_limits<std::int64_t>::min()
                         / static_cast<std::int64_t>(sample_rate)) {
        return core::Result<std::int64_t>::failure(
            make_error(core::ErrorCategory::validation, core::ErrorCode::time_overflow,
                       "audio.render.map-time", "whole-second sample index overflow"));
    }
    const auto whole_frames = seconds * static_cast<std::int64_t>(sample_rate);
    const auto fractional_numerator = remainder * static_cast<std::int64_t>(sample_rate);
    auto fractional_frames = fractional_numerator / kNanosecondsPerSecond;
    const auto fractional_remainder = fractional_numerator % kNanosecondsPerSecond;
    const auto total_floor = whole_frames + fractional_frames;
    const auto twice_remainder = fractional_remainder * 2;
    if (twice_remainder > kNanosecondsPerSecond
        || (twice_remainder == kNanosecondsPerSecond && (total_floor % 2) != 0)) {
        ++fractional_frames;
    }
    if (fractional_frames > 0
        && whole_frames > std::numeric_limits<std::int64_t>::max() - fractional_frames) {
        return core::Result<std::int64_t>::failure(
            make_error(core::ErrorCategory::validation, core::ErrorCode::time_overflow,
                       "audio.render.map-time", "fractional sample index overflow"));
    }
    return core::Result<std::int64_t>::success(whole_frames + fractional_frames);
}

[[nodiscard]] std::int64_t apply_gain(const std::int64_t sample_q23,
                                      const std::int32_t gain_ppm)
{
    return rounded_divide_ties_to_even(sample_q23 * static_cast<std::int64_t>(gain_ppm),
                                       kPpmScale);
}

void append_u16_le(std::vector<std::byte>& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<std::byte>(value & 0xffU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xffU));
}

void append_u32_le(std::vector<std::byte>& bytes, const std::uint32_t value)
{
    for (unsigned int shift = 0; shift < 32U; shift += 8U) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_fourcc(std::vector<std::byte>& bytes, const std::string_view value)
{
    for (const char character : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(character)));
    }
}

[[nodiscard]] bool is_cancelled(const core::CancellationToken* cancellation) noexcept
{
    return cancellation != nullptr && cancellation->is_cancelled();
}

} // namespace

struct PreparedMix::Impl final {
    struct Trigger {
        std::int64_t start_frame{};
        std::size_t timbre_index{};
        std::string event_id;
        std::int32_t rule_gain_ppm{};
        std::int32_t pan_ppm{};
        core::NormPpm strength_ppm{};
    };

    RenderRequest request;
    RenderLimits limits;
    std::vector<Timbre> timbres;
    std::vector<Trigger> triggers;
    std::uint64_t contribution_count{};
};

std::span<const RegisteredTestTimbre> registered_test_timbres() noexcept
{
    return kRegisteredTimbres;
}

std::string sha256_hex(const std::span<const std::byte> input)
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
            const auto s0 = std::rotr(words[index - 15U], 7)
                ^ std::rotr(words[index - 15U], 18) ^ (words[index - 15U] >> 3U);
            const auto s1 = std::rotr(words[index - 2U], 17)
                ^ std::rotr(words[index - 2U], 19) ^ (words[index - 2U] >> 10U);
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
            const auto sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
            const auto choice = (e & f) ^ ((~e) & g);
            const auto temporary1 = h + sum1 + choice + kSha256RoundConstants[index]
                + words[index];
            const auto sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
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

core::Result<Timbre> load_registered_test_timbre(
    const std::string_view fixture_id,
    const std::span<const std::byte> pcm_f32le)
{
    const auto* registration = find_registration(fixture_id);
    if (registration == nullptr) {
        return core::Result<Timbre>::failure(make_error(
            core::ErrorCategory::compatibility, core::ErrorCode::unsupported_feature,
            "audio.render.load-timbre", "fixture is not one of the three A-018 test timbres"));
    }
    std::uint64_t expected_bytes{};
    if (!checked_multiply(registration->frame_count, sizeof(float), expected_bytes)
        || pcm_f32le.size() != expected_bytes) {
        return core::Result<Timbre>::failure(make_error(
            core::ErrorCategory::validation, core::ErrorCode::invalid_pcm_buffer,
            "audio.render.load-timbre", "registered timbre byte length mismatch"));
    }
    Timbre timbre;
    timbre.registration = *registration;
    timbre.samples_q23.reserve(static_cast<std::size_t>(registration->frame_count));
    for (std::size_t offset = 0; offset < pcm_f32le.size(); offset += sizeof(float)) {
        std::uint32_t bits{};
        for (unsigned int byte = 0; byte < 4U; ++byte) {
            bits |= std::to_integer<std::uint32_t>(pcm_f32le[offset + byte]) << (byte * 8U);
        }
        const auto sample = std::bit_cast<float>(bits);
        if (!std::isfinite(sample)) {
            return core::Result<Timbre>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::non_finite_pcm,
                "audio.render.load-timbre", "registered timbre contains a non-finite sample"));
        }
        if (sample < -1.0F || sample > 1.0F) {
            return core::Result<Timbre>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::pcm_out_of_range,
                "audio.render.load-timbre", "registered timbre sample is outside [-1,1]"));
        }
        const auto scaled = static_cast<double>(sample) * static_cast<double>(kQ23Scale);
        const auto q23 = static_cast<std::int64_t>(scaled);
        if (scaled != static_cast<double>(q23) || q23 < -kQ23Scale || q23 > kQ23Scale) {
            return core::Result<Timbre>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::invalid_pcm_buffer,
                "audio.render.load-timbre", "registered timbre is not exact Q23 PCM"));
        }
        timbre.samples_q23.push_back(static_cast<std::int32_t>(q23));
    }
    if (sha256_hex(pcm_f32le) != registration->pcm_sha256) {
        return core::Result<Timbre>::failure(make_error(
            core::ErrorCategory::validation, core::ErrorCode::invalid_pcm_buffer,
            "audio.render.load-timbre", "registered timbre SHA-256 mismatch"));
    }
    return core::Result<Timbre>::success(std::move(timbre));
}

PreparedMix::PreparedMix(std::shared_ptr<const Impl> implementation)
    : implementation_(std::move(implementation))
{
}

std::uint64_t PreparedMix::frame_count() const noexcept
{
    return implementation_ ? implementation_->request.frame_count : 0U;
}

std::uint32_t PreparedMix::sample_rate() const noexcept
{
    return implementation_ ? implementation_->request.parameters.sample_rate : 0U;
}

std::uint32_t PreparedMix::channel_count() const noexcept
{
    return implementation_ ? implementation_->request.parameters.channel_count : 0U;
}

std::size_t PreparedMix::mapped_event_count() const noexcept
{
    return implementation_ ? implementation_->triggers.size() : 0U;
}

core::Result<PreparedMix> DeterministicMixer::prepare(
    RenderRequest request,
    const std::span<const Timbre> timbres,
    const RenderLimits& limits) const
{
    const auto& parameters = request.parameters;
    if (parameters.schema_version != schema_version
        || parameters.contract_version != contract_version
        || parameters.algorithm_id != algorithm_id
        || parameters.algorithm_version != algorithm_version
        || parameters.backend_id != backend_id
        || parameters.backend_version != backend_version
        || parameters.parameter_set_id != parameter_set_id
        || parameters.parameter_set_version != parameter_set_version) {
        return core::Result<PreparedMix>::failure(make_error(
            core::ErrorCategory::compatibility, core::ErrorCode::unsupported_schema,
            "audio.render.prepare", "render contract, algorithm, backend or parameter version mismatch"));
    }
    if (parameters.sample_rate != render_sample_rate
        || (parameters.channel_count != 1U && parameters.channel_count != 2U)
        || parameters.master_gain_ppm < 0 || parameters.master_gain_ppm > gain_ppm_max
        || request.frame_count > limits.max_output_frames
        || request.events.size() > limits.max_events) {
        return core::Result<PreparedMix>::failure(make_error(
            request.frame_count > limits.max_output_frames || request.events.size() > limits.max_events
                ? core::ErrorCategory::resource_limit : core::ErrorCategory::validation,
            request.frame_count > limits.max_output_frames || request.events.size() > limits.max_events
                ? core::ErrorCode::resource_limit : core::ErrorCode::invalid_analysis_parameters,
            "audio.render.prepare", "render format, gain or resource limit is invalid"));
    }
    std::uint64_t output_samples{};
    if (!checked_multiply(request.frame_count, parameters.channel_count, output_samples)
        || output_samples > limits.max_output_samples
        || output_samples > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return core::Result<PreparedMix>::failure(make_error(
            core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
            "audio.render.prepare", "output sample limit exceeded"));
    }

    std::map<std::string_view, std::size_t, std::less<>> timbre_indices;
    for (std::size_t index = 0; index < timbres.size(); ++index) {
        const auto* expected = find_registration(timbres[index].registration.fixture_id);
        const auto samples_valid = std::ranges::all_of(timbres[index].samples_q23, [](const auto sample) {
            return sample >= -kQ23Scale && sample <= kQ23Scale;
        });
        std::vector<std::byte> canonical_bytes;
        canonical_bytes.reserve(timbres[index].samples_q23.size() * sizeof(float));
        if (samples_valid) {
            for (const auto sample : timbres[index].samples_q23) {
                const auto as_float = static_cast<float>(sample)
                    / static_cast<float>(kQ23Scale);
                append_u32_le(canonical_bytes, std::bit_cast<std::uint32_t>(as_float));
            }
        }
        if (expected == nullptr || !registration_matches(timbres[index].registration, *expected)
            || timbres[index].samples_q23.size() != expected->frame_count
            || timbres[index].samples_q23.size() > limits.max_timbre_frames
            || !samples_valid
            || sha256_hex(canonical_bytes) != expected->pcm_sha256
            || !timbre_indices.emplace(expected->fixture_id, index).second) {
            return core::Result<PreparedMix>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::invalid_pcm_buffer,
                "audio.render.prepare", "timbre catalog is unregistered, altered, duplicate or oversized"));
        }
    }

    std::map<core::EventKind, const EventMappingRule*> rules;
    for (const auto& rule : parameters.mapping) {
        if (rule.gain_ppm < 0 || rule.gain_ppm > gain_ppm_max
            || rule.pan_ppm < -pan_ppm_limit || rule.pan_ppm > pan_ppm_limit
            || !timbre_indices.contains(rule.fixture_id)
            || !rules.emplace(rule.event_kind, &rule).second) {
            return core::Result<PreparedMix>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::invalid_analysis_parameters,
                "audio.render.prepare", "mapping rule is invalid, duplicate or names an unavailable timbre"));
        }
    }

    auto implementation = std::make_shared<PreparedMix::Impl>();
    implementation->request = std::move(request);
    implementation->limits = limits;
    implementation->timbres.assign(timbres.begin(), timbres.end());
    std::set<std::string, std::less<>> event_ids;
    for (const auto& event : implementation->request.events) {
        if (event.time_ns < 0 || event.duration_ns < 0
            || event.strength_ppm > core::norm_ppm_max
            || !core::validate_identifier(event.id.value)
            || !event_ids.emplace(event.id.value).second) {
            return core::Result<PreparedMix>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::invalid_event,
                "audio.render.prepare", "event schema, id, time, duration or strength is invalid"));
        }
        const auto rule_iterator = rules.find(event.kind);
        if (rule_iterator == rules.end()) {
            continue;
        }
        const auto start = relative_frame_index(event.time_ns,
                                                implementation->request.start_time_ns,
                                                parameters.sample_rate);
        if (!start) {
            return core::Result<PreparedMix>::failure(start.error());
        }
        const auto timbre_index = timbre_indices.at(rule_iterator->second->fixture_id);
        const auto timbre_frames = implementation->timbres[timbre_index].samples_q23.size();
        if (start.value() > std::numeric_limits<std::int64_t>::max()
                                - static_cast<std::int64_t>(timbre_frames)) {
            return core::Result<PreparedMix>::failure(make_error(
                core::ErrorCategory::validation, core::ErrorCode::time_overflow,
                "audio.render.prepare", "trigger tail endpoint overflow"));
        }
        const auto end = start.value() + static_cast<std::int64_t>(timbre_frames);
        const auto range_end = static_cast<std::int64_t>(implementation->request.frame_count);
        const auto first = std::max<std::int64_t>(0, start.value());
        const auto last = std::min(range_end, end);
        if (first < last) {
            std::uint64_t contribution_count = static_cast<std::uint64_t>(last - first);
            if (!checked_multiply(contribution_count, parameters.channel_count,
                                  contribution_count)
                || !checked_add(implementation->contribution_count, contribution_count,
                                implementation->contribution_count)
                || implementation->contribution_count > limits.max_contributions) {
                return core::Result<PreparedMix>::failure(make_error(
                    core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
                    "audio.render.prepare", "mix contribution limit exceeded"));
            }
            implementation->triggers.push_back({
                start.value(), timbre_index, event.id.value, rule_iterator->second->gain_ppm,
                rule_iterator->second->pan_ppm, event.strength_ppm});
        }
    }
    std::ranges::sort(implementation->triggers, [](const auto& left, const auto& right) {
        return std::tie(left.start_frame, left.event_id)
            < std::tie(right.start_frame, right.event_id);
    });
    return core::Result<PreparedMix>::success(PreparedMix{std::move(implementation)});
}

core::Result<RenderedPcm> DeterministicMixer::render(
    const PreparedMix& mix,
    const core::CancellationToken* cancellation) const
{
    return render_chunk(mix, 0U, mix.frame_count(), cancellation);
}

core::Result<RenderedPcm> DeterministicMixer::render_chunk(
    const PreparedMix& mix,
    const std::uint64_t first_frame,
    const std::uint64_t frame_count,
    const core::CancellationToken* cancellation) const
{
    if (!mix.implementation_) {
        return core::Result<RenderedPcm>::failure(make_error(
            core::ErrorCategory::validation, core::ErrorCode::invalid_dto,
            "audio.render.chunk", "prepared mix is empty"));
    }
    const auto& implementation = *mix.implementation_;
    std::uint64_t end_frame{};
    if (!checked_add(first_frame, frame_count, end_frame)
        || end_frame > implementation.request.frame_count) {
        return core::Result<RenderedPcm>::failure(make_error(
            core::ErrorCategory::validation, core::ErrorCode::invalid_dto,
            "audio.render.chunk", "chunk range is outside the prepared render"));
    }
    if (is_cancelled(cancellation)) {
        return core::Result<RenderedPcm>::failure(make_error(
            core::ErrorCategory::cancelled, core::ErrorCode::cancelled,
            "audio.render.chunk", "cancelled before rendering"));
    }

    const auto channels = implementation.request.parameters.channel_count;
    std::uint64_t sample_count{};
    if (!checked_multiply(frame_count, channels, sample_count)
        || sample_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return core::Result<RenderedPcm>::failure(make_error(
            core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
            "audio.render.chunk", "chunk allocation size overflow"));
    }
    std::vector<std::int64_t> accumulator(static_cast<std::size_t>(sample_count), 0);
    RenderStatistics statistics;

    const auto chunk_first = static_cast<std::int64_t>(first_frame);
    const auto chunk_last = static_cast<std::int64_t>(end_frame);
    std::uint64_t cancellation_counter{};
    for (const auto& trigger : implementation.triggers) {
        const auto& timbre = implementation.timbres[trigger.timbre_index];
        const auto trigger_last = trigger.start_frame
            + static_cast<std::int64_t>(timbre.samples_q23.size());
        const auto overlap_first = std::max(chunk_first, trigger.start_frame);
        const auto overlap_last = std::min(chunk_last, trigger_last);
        if (overlap_first >= overlap_last) {
            continue;
        }
        ++statistics.mapped_event_count;
        for (auto output_frame = overlap_first; output_frame < overlap_last; ++output_frame) {
            if ((cancellation_counter++ & 0xffU) == 0U && is_cancelled(cancellation)) {
                return core::Result<RenderedPcm>::failure(make_error(
                    core::ErrorCategory::cancelled, core::ErrorCode::cancelled,
                    "audio.render.chunk", "cancelled while rendering"));
            }
            const auto source_frame = static_cast<std::size_t>(output_frame - trigger.start_frame);
            auto sample = static_cast<std::int64_t>(timbre.samples_q23[source_frame]);
            sample = apply_gain(sample, static_cast<std::int32_t>(trigger.strength_ppm));
            sample = apply_gain(sample, trigger.rule_gain_ppm);
            sample = apply_gain(sample, implementation.request.parameters.master_gain_ppm);
            const auto destination = static_cast<std::size_t>(output_frame - chunk_first)
                * channels;
            if (channels == 1U) {
                accumulator[destination] += sample;
                ++statistics.contribution_count;
            } else {
                const auto left_pan_gain = trigger.pan_ppm > 0
                    ? pan_ppm_limit - trigger.pan_ppm : pan_ppm_limit;
                const auto right_pan_gain = trigger.pan_ppm < 0
                    ? pan_ppm_limit + trigger.pan_ppm : pan_ppm_limit;
                accumulator[destination] += apply_gain(sample, left_pan_gain);
                accumulator[destination + 1U] += apply_gain(sample, right_pan_gain);
                statistics.contribution_count += 2U;
            }
        }
    }

    RenderedPcm output;
    output.sample_rate = implementation.request.parameters.sample_rate;
    output.channel_count = channels;
    output.first_frame = first_frame;
    output.frame_count = frame_count;
    output.interleaved_f32.reserve(static_cast<std::size_t>(sample_count));
    for (const auto value : accumulator) {
        const auto magnitude = value < 0 ? -value : value;
        statistics.peak_before_clip_q23 = std::max(statistics.peak_before_clip_q23, magnitude);
        auto clipped = value;
        if (value > kQ23Scale) {
            clipped = kQ23Scale;
            ++statistics.clipped_sample_count;
        } else if (value < -kQ23Scale) {
            clipped = -kQ23Scale;
            ++statistics.clipped_sample_count;
        }
        const auto clipped_magnitude = static_cast<std::int32_t>(clipped < 0 ? -clipped : clipped);
        statistics.peak_after_clip_q23 = std::max(statistics.peak_after_clip_q23,
                                                  clipped_magnitude);
        output.interleaved_f32.push_back(static_cast<float>(clipped)
                                         / static_cast<float>(kQ23Scale));
    }
    output.statistics = statistics;
    return core::Result<RenderedPcm>::success(std::move(output));
}

std::vector<std::byte> pcm_f32le_bytes(const RenderedPcm& pcm)
{
    std::vector<std::byte> bytes;
    bytes.reserve(pcm.interleaved_f32.size() * sizeof(float));
    for (const auto sample : pcm.interleaved_f32) {
        const auto value = std::bit_cast<std::uint32_t>(sample);
        append_u32_le(bytes, value);
    }
    return bytes;
}

core::Result<std::vector<std::byte>> wav_f32le_bytes(const RenderedPcm& pcm)
{
    std::uint64_t expected_samples{};
    if ((pcm.channel_count != 1U && pcm.channel_count != 2U)
        || pcm.sample_rate != render_sample_rate
        || !checked_multiply(pcm.frame_count, pcm.channel_count, expected_samples)
        || expected_samples != pcm.interleaved_f32.size()
        || !std::ranges::all_of(pcm.interleaved_f32, [](const float sample) {
               return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F;
           })) {
        return core::Result<std::vector<std::byte>>::failure(make_error(
            core::ErrorCategory::validation, core::ErrorCode::invalid_pcm_buffer,
            "audio.render.wav", "rendered PCM metadata does not match its sample array"));
    }
    const auto pcm_bytes = pcm_f32le_bytes(pcm);
    if (pcm_bytes.size() > std::numeric_limits<std::uint32_t>::max() - 36U) {
        return core::Result<std::vector<std::byte>>::failure(make_error(
            core::ErrorCategory::resource_limit, core::ErrorCode::resource_limit,
            "audio.render.wav", "RIFF size exceeds the WAV schema-1 limit"));
    }
    const auto data_size = static_cast<std::uint32_t>(pcm_bytes.size());
    const auto byte_rate = pcm.sample_rate * pcm.channel_count
        * static_cast<std::uint32_t>(sizeof(float));
    const auto block_align = static_cast<std::uint16_t>(pcm.channel_count * sizeof(float));
    std::vector<std::byte> bytes;
    bytes.reserve(44U + pcm_bytes.size());
    append_fourcc(bytes, "RIFF");
    append_u32_le(bytes, 36U + data_size);
    append_fourcc(bytes, "WAVE");
    append_fourcc(bytes, "fmt ");
    append_u32_le(bytes, 16U);
    append_u16_le(bytes, 3U); // WAVE_FORMAT_IEEE_FLOAT
    append_u16_le(bytes, static_cast<std::uint16_t>(pcm.channel_count));
    append_u32_le(bytes, pcm.sample_rate);
    append_u32_le(bytes, byte_rate);
    append_u16_le(bytes, block_align);
    append_u16_le(bytes, 32U);
    append_fourcc(bytes, "data");
    append_u32_le(bytes, data_size);
    bytes.insert(bytes.end(), pcm_bytes.begin(), pcm_bytes.end());
    return core::Result<std::vector<std::byte>>::success(std::move(bytes));
}

std::string canonical_render_parameters(const RenderParameters& parameters)
{
    std::ostringstream output;
    output << "schema=" << parameters.schema_version << "|contract=" << parameters.contract_version
           << "|algorithm=" << parameters.algorithm_id << '@' << parameters.algorithm_version
           << "|backend=" << parameters.backend_id << '@' << parameters.backend_version
           << "|parameter-set=" << parameters.parameter_set_id << '@'
           << parameters.parameter_set_version << "|rate=" << parameters.sample_rate
           << "|channels=" << parameters.channel_count << "|master-gain-ppm="
           << parameters.master_gain_ppm << "|seed=" << parameters.deterministic_seed;
    for (const auto& rule : parameters.mapping) {
        output << "|map=" << core::to_string(rule.event_kind) << ',' << rule.fixture_id << ','
               << rule.gain_ppm << ',' << rule.pan_ppm;
    }
    return output.str();
}

} // namespace space_rhythm::audio::render
