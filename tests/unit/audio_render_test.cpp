#include <space_rhythm/audio/preview_qt.hpp>
#include <space_rhythm/audio/render.hpp>

#include <QAudioDevice>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace render = space_rhythm::audio::render;

std::filesystem::path fixture_path(const std::string_view name)
{
    return std::filesystem::path{SPACE_RHYTHM_GOLDEN_AUDIO_DIR} / name;
}

std::vector<std::byte> read_bytes(const std::filesystem::path& path)
{
    std::ifstream input{path, std::ios::binary};
    EXPECT_TRUE(input.good()) << path;
    const std::vector<char> characters{std::istreambuf_iterator<char>{input},
                                       std::istreambuf_iterator<char>{}};
    std::vector<std::byte> bytes(characters.size());
    std::transform(characters.begin(), characters.end(), bytes.begin(), [](const char value) {
        return static_cast<std::byte>(static_cast<unsigned char>(value));
    });
    return bytes;
}

std::vector<render::Timbre> load_timbres()
{
    std::vector<render::Timbre> result;
    for (const auto& registration : render::registered_test_timbres()) {
        const auto bytes = read_bytes(fixture_path(registration.output_file));
        const auto loaded = render::load_registered_test_timbre(registration.fixture_id, bytes);
        EXPECT_TRUE(loaded) << registration.fixture_id << " code="
                            << (loaded ? "none" : std::string{core::to_string(loaded.error().code)})
                            << " reason="
                            << (loaded ? "none" : loaded.error().context.at("reason"));
        if (loaded) {
            result.push_back(loaded.value());
        }
    }
    return result;
}

render::RenderParameters parameters()
{
    render::RenderParameters value;
    value.mapping = {
        {core::EventKind::onset, "AT-CLICK-001", 1'000'000, 0},
        {core::EventKind::beat, "AT-LOW-PULSE-001", 1'000'000, 0},
        {core::EventKind::manual, "AT-NOISE-HIT-001", 1'000'000, 0},
    };
    return value;
}

core::RhythmEvent event(const std::string& id,
                        const core::TimeNs time_ns,
                        const core::EventKind kind,
                        const core::NormPpm strength_ppm = core::norm_ppm_max)
{
    core::RhythmEvent value;
    value.id.value = id;
    value.track_id.value = "audio-render-track";
    value.time_ns = time_ns;
    value.kind = kind;
    value.strength_ppm = strength_ppm;
    return value;
}

render::RenderRequest request(std::vector<core::RhythmEvent> events,
                              const std::uint64_t frame_count = 9'600)
{
    render::RenderRequest value;
    value.frame_count = frame_count;
    value.parameters = parameters();
    value.events = std::move(events);
    return value;
}

std::uint32_t read_u32(const std::span<const std::byte> bytes, const std::size_t offset)
{
    return std::to_integer<std::uint32_t>(bytes[offset])
        | (std::to_integer<std::uint32_t>(bytes[offset + 1U]) << 8U)
        | (std::to_integer<std::uint32_t>(bytes[offset + 2U]) << 16U)
        | (std::to_integer<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

TEST(AudioRenderRegistry, AcceptsExactlyTheThreeA018Cc0Timbres)
{
    const auto registrations = render::registered_test_timbres();
    ASSERT_EQ(registrations.size(), 3U);
    EXPECT_EQ(registrations[0].fixture_id, "AT-CLICK-001");
    EXPECT_EQ(registrations[1].fixture_id, "AT-LOW-PULSE-001");
    EXPECT_EQ(registrations[2].fixture_id, "AT-NOISE-HIT-001");
    for (const auto& registration : registrations) {
        EXPECT_EQ(registration.license, "CC0-1.0");
        const auto bytes = read_bytes(fixture_path(registration.output_file));
        const auto loaded = render::load_registered_test_timbre(registration.fixture_id, bytes);
        ASSERT_TRUE(loaded) << registration.fixture_id << " code="
                            << std::string{core::to_string(loaded.error().code)} << " reason="
                            << loaded.error().context.at("reason");
        EXPECT_EQ(loaded.value().samples_q23.size(), registration.frame_count);
        EXPECT_EQ(render::sha256_hex(bytes), registration.pcm_sha256);
    }
    const std::array<std::byte, 4> unknown{};
    const auto rejected = render::load_registered_test_timbre("unknown", unknown);
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code, core::ErrorCode::unsupported_feature);
}

TEST(AudioRenderRegistry, RejectsNonFiniteAndAlteredPcmFailClosed)
{
    auto bytes = read_bytes(fixture_path("timbre-click-48000-mono.f32le"));
    const auto nan = std::bit_cast<std::array<std::byte, 4>>(std::numeric_limits<float>::quiet_NaN());
    std::copy(nan.begin(), nan.end(), bytes.begin());
    const auto non_finite = render::load_registered_test_timbre("AT-CLICK-001", bytes);
    ASSERT_FALSE(non_finite);
    EXPECT_EQ(non_finite.error().code, core::ErrorCode::non_finite_pcm);

    bytes = read_bytes(fixture_path("timbre-click-48000-mono.f32le"));
    bytes.back() ^= std::byte{1};
    const auto altered = render::load_registered_test_timbre("AT-CLICK-001", bytes);
    ASSERT_FALSE(altered);
    EXPECT_EQ(altered.error().code, core::ErrorCode::invalid_pcm_buffer);
}

TEST(AudioRenderMixer, MapsEventsAndTriggersAtNearestEvenSample)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    auto input = request({event("onset-a", 20'833, core::EventKind::onset)}, 16);
    const auto prepared = mixer.prepare(std::move(input), timbres);
    ASSERT_TRUE(prepared);
    const auto output = mixer.render(prepared.value());
    ASSERT_TRUE(output);
    ASSERT_EQ(output.value().interleaved_f32.size(), 32U);
    EXPECT_FLOAT_EQ(output.value().interleaved_f32[0], 0.0F);
    EXPECT_FLOAT_EQ(output.value().interleaved_f32[1], 0.0F);
    EXPECT_FLOAT_EQ(output.value().interleaved_f32[2], 0.75F);
    EXPECT_FLOAT_EQ(output.value().interleaved_f32[3], 0.75F);
}

TEST(AudioRenderMixer, PreservesTailsAcrossRenderOrigin)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    auto input = request({event("early-click", 0, core::EventKind::onset)}, 32);
    input.start_time_ns = 10'000'000; // source frame 480 is the first rendered frame
    const auto prepared = mixer.prepare(std::move(input), timbres);
    ASSERT_TRUE(prepared);
    const auto output = mixer.render(prepared.value());
    ASSERT_TRUE(output);
    const auto expected_q23 = timbres[0].samples_q23[480];
    EXPECT_FLOAT_EQ(output.value().interleaved_f32[0],
                    static_cast<float>(expected_q23) / 8'388'608.0F);
    EXPECT_NE(output.value().interleaved_f32[0], 0.0F);
}

TEST(AudioRenderMixer, AppliesOverlapGainPanAndHardClip)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    auto clipped_request = request({
        event("click-a", 0, core::EventKind::onset),
        event("click-b", 0, core::EventKind::onset),
    }, 8);
    const auto clipped_mix = mixer.prepare(std::move(clipped_request), timbres);
    ASSERT_TRUE(clipped_mix);
    const auto clipped = mixer.render(clipped_mix.value());
    ASSERT_TRUE(clipped);
    EXPECT_FLOAT_EQ(clipped.value().interleaved_f32[0], 1.0F);
    EXPECT_FLOAT_EQ(clipped.value().interleaved_f32[1], 1.0F);
    EXPECT_GT(clipped.value().statistics.clipped_sample_count, 0U);
    EXPECT_GT(clipped.value().statistics.peak_before_clip_q23, 8'388'608);

    auto panned_request = request({event("panned", 0, core::EventKind::onset)}, 1);
    panned_request.parameters.mapping[0].gain_ppm = 500'000;
    panned_request.parameters.mapping[0].pan_ppm = -1'000'000;
    const auto panned_mix = mixer.prepare(std::move(panned_request), timbres);
    ASSERT_TRUE(panned_mix);
    const auto panned = mixer.render(panned_mix.value());
    ASSERT_TRUE(panned);
    EXPECT_FLOAT_EQ(panned.value().interleaved_f32[0], 0.375F);
    EXPECT_FLOAT_EQ(panned.value().interleaved_f32[1], 0.0F);
}

TEST(AudioRenderMixer, RepeatAndInputOrderProduceIdenticalBytes)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    const std::vector events{
        event("event-c", 75'000'000, core::EventKind::manual, 700'000),
        event("event-a", 0, core::EventKind::beat, 800'000),
        event("event-b", 25'000'000, core::EventKind::onset, 900'000),
    };
    const auto first_mix = mixer.prepare(request(events), timbres);
    ASSERT_TRUE(first_mix);
    const auto first = mixer.render(first_mix.value());
    const auto second = mixer.render(first_mix.value());
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(render::pcm_f32le_bytes(first.value()), render::pcm_f32le_bytes(second.value()));

    auto reversed = events;
    std::ranges::reverse(reversed);
    const auto reversed_mix = mixer.prepare(request(std::move(reversed)), timbres);
    ASSERT_TRUE(reversed_mix);
    const auto reversed_output = mixer.render(reversed_mix.value());
    ASSERT_TRUE(reversed_output);
    EXPECT_EQ(render::pcm_f32le_bytes(first.value()),
              render::pcm_f32le_bytes(reversed_output.value()));
}

TEST(AudioRenderMixer, ArbitraryChunkBoundariesMatchSinglePass)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    const auto prepared = mixer.prepare(request({
        event("long-tail", 0, core::EventKind::beat),
        event("overlap", 19'979'167, core::EventKind::onset),
        event("noise", 99'979'167, core::EventKind::manual),
    }, 8'000), timbres);
    ASSERT_TRUE(prepared);
    const auto whole = mixer.render(prepared.value());
    ASSERT_TRUE(whole);

    std::vector<float> joined;
    std::uint64_t first = 0;
    const std::array chunk_sizes{1U, 257U, 1'024U, 17U, 3'333U};
    std::size_t chunk_index = 0;
    while (first < prepared.value().frame_count()) {
        const auto count = std::min<std::uint64_t>(
            chunk_sizes[chunk_index++ % chunk_sizes.size()],
            prepared.value().frame_count() - first);
        const auto chunk = mixer.render_chunk(prepared.value(), first, count);
        ASSERT_TRUE(chunk);
        joined.insert(joined.end(), chunk.value().interleaved_f32.begin(),
                      chunk.value().interleaved_f32.end());
        first += count;
    }
    EXPECT_EQ(joined, whole.value().interleaved_f32);
}

TEST(AudioRenderOffline, WavPayloadIsTheExactSharedCorePcm)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    const auto prepared = mixer.prepare(
        request({event("beat", 0, core::EventKind::beat)}, 4'800), timbres);
    ASSERT_TRUE(prepared);
    const auto output = mixer.render(prepared.value());
    ASSERT_TRUE(output);
    const auto pcm = render::pcm_f32le_bytes(output.value());
    const auto wav = render::wav_f32le_bytes(output.value());
    ASSERT_TRUE(wav);
    ASSERT_EQ(wav.value().size(), pcm.size() + 44U);
    EXPECT_EQ(read_u32(wav.value(), 16), 16U);
    EXPECT_EQ(read_u32(wav.value(), 24), 48'000U);
    EXPECT_EQ(read_u32(wav.value(), 40), pcm.size());
    EXPECT_TRUE(std::equal(pcm.begin(), pcm.end(), wav.value().begin() + 44));
    EXPECT_EQ(render::sha256_hex(pcm),
              "9a8211f7d16e8e42a69d76623c3b5f1d80e7fc78484c752017975e2f53f3d182");
    EXPECT_EQ(render::sha256_hex(wav.value()),
              "a2b9c9f33733e9c3e64486aaeabe706a44334a49155012bb2c05224a4de34614");
}

TEST(AudioRenderMixer, CancellationPublishesNoPartialPcm)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    const auto prepared = mixer.prepare(
        request({event("beat", 0, core::EventKind::beat)}, 48'000), timbres);
    ASSERT_TRUE(prepared);
    core::CancellationToken cancellation;
    cancellation.cancel();
    const auto output = mixer.render(prepared.value(), &cancellation);
    ASSERT_FALSE(output);
    EXPECT_EQ(output.error().category, core::ErrorCategory::cancelled);
    EXPECT_EQ(output.error().code, core::ErrorCode::cancelled);
}

TEST(AudioRenderMixer, RejectsResourceAndMappingViolations)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    render::RenderLimits limits;
    limits.max_output_frames = 10;
    const auto oversized = mixer.prepare(request({}, 11), timbres, limits);
    ASSERT_FALSE(oversized);
    EXPECT_EQ(oversized.error().code, core::ErrorCode::resource_limit);

    auto invalid = request({}, 1);
    invalid.parameters.mapping[0].fixture_id = "product.default.unconfirmed";
    const auto unmapped = mixer.prepare(std::move(invalid), timbres);
    ASSERT_FALSE(unmapped);
    EXPECT_EQ(unmapped.error().code, core::ErrorCode::invalid_analysis_parameters);

    auto altered_timbres = timbres;
    altered_timbres[0].samples_q23[0] -= 1;
    const auto altered = mixer.prepare(request({}, 1), altered_timbres);
    ASSERT_FALSE(altered);
    EXPECT_EQ(altered.error().code, core::ErrorCode::invalid_pcm_buffer);
}

TEST(AudioRenderPreview, UnavailableDeviceDoesNotChangeOfflinePcm)
{
    const auto timbres = load_timbres();
    render::DeterministicMixer mixer;
    const auto prepared = mixer.prepare(
        request({event("preview", 0, core::EventKind::onset)}, 960), timbres);
    ASSERT_TRUE(prepared);
    const auto output = mixer.render(prepared.value());
    ASSERT_TRUE(output);
    const auto before = render::pcm_f32le_bytes(output.value());

    render::QtAudioPreview preview;
    const auto start = preview.start(output.value(), QAudioDevice{});
    EXPECT_FALSE(start.started);
    EXPECT_EQ(start.error, render::PreviewError::device_unavailable);
    EXPECT_FALSE(preview.active());
    EXPECT_EQ(render::pcm_f32le_bytes(output.value()), before);
}

TEST(AudioRenderContract, ParameterSummaryFreezesVersionsAndMappingOrder)
{
    const auto value = parameters();
    const auto canonical = render::canonical_render_parameters(value);
    EXPECT_NE(canonical.find("space-rhythm.audio-render.integer-q23@1.0.0"),
              std::string::npos);
    EXPECT_NE(canonical.find("portable-cxx20@1.0.0"), std::string::npos);
    EXPECT_EQ(render::sha256_hex(std::as_bytes(std::span{canonical.data(), canonical.size()})),
              "3e0eb79a7a61b75bf671c6c17ece300408e2a0e0ea29f2f158f4b2b63ffa17c7");
}

} // namespace
