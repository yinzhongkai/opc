#pragma once

#include <space_rhythm/core/timeline.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::audio::render {

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::string_view algorithm_id{"space-rhythm.audio-render.integer-q23"};
inline constexpr std::string_view algorithm_version{"1.0.0"};
inline constexpr std::string_view backend_id{"portable-cxx20"};
inline constexpr std::string_view backend_version{"1.0.0"};
inline constexpr std::string_view parameter_set_id{"space-rhythm.audio-render.test-timbres"};
inline constexpr std::string_view parameter_set_version{"1.0.0"};
inline constexpr std::uint32_t render_sample_rate = 48'000;
inline constexpr std::int32_t gain_ppm_max = 4'000'000;
inline constexpr std::int32_t pan_ppm_limit = 1'000'000;

struct RegisteredTestTimbre {
    std::string fixture_id;
    std::string timbre_id;
    std::string output_file;
    std::string license;
    std::string pcm_sha256;
    std::uint32_t sample_rate{};
    std::uint64_t frame_count{};
};

[[nodiscard]] std::span<const RegisteredTestTimbre> registered_test_timbres() noexcept;

struct Timbre final {
    RegisteredTestTimbre registration;
    std::vector<std::int32_t> samples_q23;
};

// Accepts only the three A-018 manifest-v1 mono f32_le CC0 fixtures. The bytes,
// metadata, finite range and exact Q23 representation are all verified.
[[nodiscard]] core::Result<Timbre> load_registered_test_timbre(
    std::string_view fixture_id,
    std::span<const std::byte> pcm_f32le);

struct EventMappingRule {
    core::EventKind event_kind{core::EventKind::onset};
    std::string fixture_id;
    std::int32_t gain_ppm{1'000'000};
    std::int32_t pan_ppm{};

    bool operator==(const EventMappingRule&) const = default;
};

struct RenderParameters {
    std::uint32_t schema_version{render::schema_version};
    std::string contract_version{render::contract_version};
    std::string algorithm_id{render::algorithm_id};
    std::string algorithm_version{render::algorithm_version};
    std::string backend_id{render::backend_id};
    std::string backend_version{render::backend_version};
    std::string parameter_set_id{render::parameter_set_id};
    std::string parameter_set_version{render::parameter_set_version};
    std::uint32_t sample_rate{render_sample_rate};
    std::uint32_t channel_count{2};
    std::int32_t master_gain_ppm{1'000'000};
    std::uint64_t deterministic_seed{0x5350414345524859ULL};
    std::vector<EventMappingRule> mapping;

    bool operator==(const RenderParameters&) const = default;
};

struct RenderLimits {
    std::uint64_t max_events{100'000};
    std::uint64_t max_output_frames{28'800'000}; // ten minutes at 48 kHz
    std::uint64_t max_output_samples{57'600'000};
    std::uint64_t max_timbre_frames{48'000};
    std::uint64_t max_contributions{500'000'000};
};

struct RenderRequest {
    core::TimeNs start_time_ns{};
    std::uint64_t frame_count{};
    RenderParameters parameters;
    std::vector<core::RhythmEvent> events;
};

struct RenderStatistics {
    std::uint64_t mapped_event_count{};
    std::uint64_t contribution_count{};
    std::uint64_t clipped_sample_count{};
    std::int64_t peak_before_clip_q23{};
    std::int32_t peak_after_clip_q23{};

    bool operator==(const RenderStatistics&) const = default;
};

struct RenderedPcm {
    std::uint32_t sample_rate{render_sample_rate};
    std::uint32_t channel_count{2};
    std::uint64_t first_frame{};
    std::uint64_t frame_count{};
    std::vector<float> interleaved_f32;
    RenderStatistics statistics;

    bool operator==(const RenderedPcm&) const = default;
};

class PreparedMix final {
public:
    PreparedMix() = default;

    [[nodiscard]] std::uint64_t frame_count() const noexcept;
    [[nodiscard]] std::uint32_t sample_rate() const noexcept;
    [[nodiscard]] std::uint32_t channel_count() const noexcept;
    [[nodiscard]] std::size_t mapped_event_count() const noexcept;

private:
    struct Impl;
    explicit PreparedMix(std::shared_ptr<const Impl> implementation);
    std::shared_ptr<const Impl> implementation_;
    friend class DeterministicMixer;
};

class DeterministicMixer final {
public:
    [[nodiscard]] core::Result<PreparedMix> prepare(
        RenderRequest request,
        std::span<const Timbre> timbres,
        const RenderLimits& limits = {}) const;

    [[nodiscard]] core::Result<RenderedPcm> render(
        const PreparedMix& mix,
        const core::CancellationToken* cancellation = nullptr) const;

    [[nodiscard]] core::Result<RenderedPcm> render_chunk(
        const PreparedMix& mix,
        std::uint64_t first_frame,
        std::uint64_t frame_count,
        const core::CancellationToken* cancellation = nullptr) const;
};

[[nodiscard]] std::vector<std::byte> pcm_f32le_bytes(const RenderedPcm& pcm);
[[nodiscard]] core::Result<std::vector<std::byte>> wav_f32le_bytes(const RenderedPcm& pcm);
[[nodiscard]] std::string canonical_render_parameters(const RenderParameters& parameters);
[[nodiscard]] std::string sha256_hex(std::span<const std::byte> bytes);

} // namespace space_rhythm::audio::render
