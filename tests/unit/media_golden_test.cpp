#include <space_rhythm/media/media.hpp>

#include "ffmpeg_color_range.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

namespace media = space_rhythm::media;
namespace core = space_rhythm::core;

namespace {

TEST(MediaColorRange, NormalizesFfmpegRangeToClosedPublicVocabulary)
{
    EXPECT_EQ(media::detail::normalize_color_range(AVCOL_RANGE_MPEG), "limited");
    EXPECT_EQ(media::detail::normalize_color_range(AVCOL_RANGE_JPEG), "full");
    EXPECT_EQ(media::detail::normalize_color_range(AVCOL_RANGE_UNSPECIFIED), "unknown");
    EXPECT_EQ(media::detail::normalize_color_range(AVCOL_RANGE_NB), "unknown");
    EXPECT_EQ(media::detail::normalize_color_range(static_cast<AVColorRange>(-1)), "unknown");
}

std::filesystem::path golden(std::string_view name)
{
    return std::filesystem::path{SPACE_RHYTHM_GOLDEN_MEDIA_DIR} / name;
}

media::StreamSelectionRequest required_stream()
{
    return {media::SelectionMode::required_default_then_lowest_index, std::nullopt, false};
}

media::StreamSelectionRequest optional_stream()
{
    return {media::SelectionMode::optional_default_then_lowest_index, std::nullopt, false};
}

media::MediaSelection select_video(const std::shared_ptr<media::MediaSource>& source)
{
    auto selected = source->select(required_stream(), optional_stream());
    if (!selected) {
        ADD_FAILURE() << core::to_string(selected.error().code);
        return {};
    }
    return selected.value();
}

media::MediaSelection select_audio(const std::shared_ptr<media::MediaSource>& source)
{
    auto selected = source->select(optional_stream(), required_stream());
    if (!selected) {
        ADD_FAILURE() << core::to_string(selected.error().code);
        return {};
    }
    return selected.value();
}

TEST(MediaTime, UsesCoreTimeNsWithExactRationalRounding)
{
    const media::PresentationOrigin zero{{0, {1, 1}, media::TimestampOrigin::format_start},
                                         false};
    const auto ntsc = media::map_presentation_time(
        {1, {1001, 30000}, media::TimestampOrigin::frame_pts}, zero);
    ASSERT_TRUE(ntsc);
    EXPECT_EQ(ntsc.value(), 33'366'667);

    const auto half_negative = media::map_presentation_time(
        {-1, {1, 2'000'000'000}, media::TimestampOrigin::frame_pts},
        zero,
        core::RoundingMode::floor);
    ASSERT_TRUE(half_negative);
    EXPECT_EQ(half_negative.value(), -1);

    const media::PresentationOrigin different_origin{
        {-80, {1, 1000}, media::TimestampOrigin::stream_start}, false};
    const auto different_base = media::map_presentation_time(
        {0, {1, 48000}, media::TimestampOrigin::frame_pts}, different_origin);
    ASSERT_TRUE(different_base);
    EXPECT_EQ(different_base.value(), 80'000'000);
}

TEST(MediaTime, ConvertsSampleIndicesWithPurposeSpecificRounding)
{
    const auto sample = media::sample_index_to_time_ns(
        1, 44100, 0, core::RoundingMode::nearest_ties_to_even);
    ASSERT_TRUE(sample);
    EXPECT_EQ(sample.value(), 22'676);

    const auto first_not_before = media::time_ns_to_sample_index(
        50'000'001, 0, 44100, core::RoundingMode::ceil);
    const auto last_not_after = media::time_ns_to_sample_index(
        50'000'001, 0, 44100, core::RoundingMode::floor);
    ASSERT_TRUE(first_not_before);
    ASSERT_TRUE(last_not_after);
    EXPECT_EQ(first_not_before.value(), 2206);
    EXPECT_EQ(last_not_after.value(), 2205);
}

TEST(FfmpegRuntime, ReportsPinnedLgplConfiguration)
{
    const auto info = media::query_ffmpeg_build_info();
    ASSERT_TRUE(info);
    EXPECT_EQ(info.value().ffmpeg_version, "8.1.2");
    EXPECT_NE(info.value().license.find("LGPL version 3"), std::string::npos);
    EXPECT_NE(info.value().build_configuration.find("--enable-version3"), std::string::npos);
    EXPECT_NE(info.value().build_configuration.find("--enable-avdevice"), std::string::npos);
    EXPECT_EQ(info.value().build_configuration.find("--enable-gpl"), std::string::npos);
    EXPECT_EQ(info.value().build_configuration.find("--enable-nonfree"), std::string::npos);
    EXPECT_EQ(info.value().library_versions.at("avcodec"), "62.28.102");
    EXPECT_EQ(info.value().library_versions.at("avformat"), "62.12.102");
    EXPECT_EQ(info.value().build_configuration_sha256.size(), 64U);
}

TEST(MediaProbe, RejectsCorruptMediaWithoutPartialSuccess)
{
    const auto source = media::MediaSource::open(golden("corrupt_ebml.mkv"));
    ASSERT_FALSE(source);
    EXPECT_EQ(source.error().category, core::ErrorCategory::media);
    EXPECT_EQ(source.error().code, core::ErrorCode::corrupt_media);
}

TEST(MediaProbe, SelectsMultipleStreamsDeterministically)
{
    const auto source = media::MediaSource::open(golden("multi_stream.mkv"));
    ASSERT_TRUE(source);
    ASSERT_EQ(source.value()->info().streams.size(), 3U);

    auto selected = source.value()->select(required_stream(), required_stream());
    ASSERT_TRUE(selected);
    ASSERT_EQ(selected.value().video.selected.size(), 1U);
    ASSERT_EQ(selected.value().audio.selected.size(), 1U);
    const auto default_audio_index = selected.value().audio.selected.front().stream_index;
    const auto& default_audio = source.value()->info().streams.at(
        static_cast<std::size_t>(default_audio_index));
    ASSERT_TRUE(default_audio.language);
    EXPECT_EQ(*default_audio.language, "eng");
    ASSERT_TRUE(default_audio.audio);
    EXPECT_EQ(default_audio.audio->sample_rate, 48000U);

    const auto alternate = std::ranges::find_if(source.value()->info().streams,
                                                [](const media::StreamInfo& stream) {
                                                    return stream.language
                                                        && *stream.language == "jpn";
                                                });
    ASSERT_NE(alternate, source.value()->info().streams.end());
    media::StreamSelectionRequest explicit_audio{
        media::SelectionMode::explicit_stream, alternate->key, false};
    selected = source.value()->select(required_stream(), explicit_audio);
    ASSERT_TRUE(selected);
    EXPECT_EQ(selected.value().audio.selected.front(), alternate->key);
}

TEST(MediaProbe, MissingRequiredStreamsHaveStableErrors)
{
    const auto audio_only = media::MediaSource::open(golden("audio_only.mka"));
    ASSERT_TRUE(audio_only);
    const auto no_video = audio_only.value()->select(required_stream(), optional_stream());
    ASSERT_FALSE(no_video);
    EXPECT_EQ(no_video.error().code, core::ErrorCode::missing_required_stream);

    const auto video_only = media::MediaSource::open(golden("video_only.mkv"));
    ASSERT_TRUE(video_only);
    const auto no_audio = video_only.value()->select(optional_stream(), required_stream());
    ASSERT_FALSE(no_audio);
    EXPECT_EQ(no_audio.error().code, core::ErrorCode::missing_required_stream);
}

TEST(MediaDecode, MapsCfrAndVfrFromPresentationTimestamps)
{
    for (const auto& [file, expected] : std::array{
             std::pair{"cfr_av.mkv",
                       std::vector<core::TimeNs>{0, 40'000'000, 80'000'000, 120'000'000,
                                                 160'000'000}},
             std::pair{"vfr_video.mkv",
                       std::vector<core::TimeNs>{0, 40'000'000, 100'000'000, 140'000'000,
                                                 240'000'000}}}) {
        const auto source = media::MediaSource::open(golden(file));
        ASSERT_TRUE(source);
        const auto selection = select_video(source.value());
        std::vector<core::TimeNs> actual;
        std::vector<std::uint64_t> epochs;
        const media::VideoCallbacks callbacks{
            [&](const media::FormatChanged& changed) {
                epochs.push_back(changed.format_epoch);
                return media::PublishResult::accepted;
            },
            [&](media::VideoFrame frame) {
                actual.push_back(frame.time_ns);
                EXPECT_EQ(frame.pixel_format, "bgra");
                EXPECT_EQ(frame.lease.byte_size(), 16U * 16U * 4U);
                return media::PublishResult::accepted;
            }};
        const auto decoded = source.value()->decode_video(selection,
                                                          selection.video.selected.front(),
                                                          {16, 16, false},
                                                          media::DecodeLimits{},
                                                          callbacks);
        ASSERT_TRUE(decoded) << core::to_string(decoded.error().code);
        EXPECT_TRUE(decoded.value().end_of_stream);
        EXPECT_EQ(actual, expected);
        ASSERT_EQ(epochs.size(), 1U);
        EXPECT_EQ(epochs.front(), 1U);
    }
}

TEST(MediaDecode, PreservesNegativeSourceTimestampsAndSharedOrigin)
{
    const auto source = media::MediaSource::open(golden("negative_start.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    std::vector<core::TimeNs> mapped;
    std::vector<std::int64_t> raw;
    const auto result = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {16, 16, false},
        media::DecodeLimits{},
        {{}, [&](media::VideoFrame frame) {
             mapped.push_back(frame.time_ns);
             EXPECT_TRUE(frame.source_pts);
             if (frame.source_pts) {
                 raw.push_back(frame.source_pts->ticks);
             }
             return media::PublishResult::accepted;
         }});
    ASSERT_TRUE(result);
    EXPECT_EQ(mapped,
              (std::vector<core::TimeNs>{0, 40'000'000, 80'000'000, 120'000'000}));
    EXPECT_LT(raw.front(), 0);
}

TEST(MediaDecode, NormalizesRotationSarAndColorForThumbnail)
{
    const auto source = media::MediaSource::open(golden("rotated_sar.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    const auto index = selection.video.selected.front().stream_index;
    const auto& stream = source.value()->info().streams.at(static_cast<std::size_t>(index));
    ASSERT_TRUE(stream.video);
    ASSERT_TRUE(stream.video->geometry.sample_aspect_ratio);
    EXPECT_EQ(*stream.video->geometry.sample_aspect_ratio, (media::Rational{4, 3}));
    ASSERT_TRUE(stream.video->geometry.display_transform);
    EXPECT_EQ(stream.video->geometry.display_transform->clockwise_rotation_degrees, 90);
    EXPECT_EQ(stream.video->color.primaries, "bt709");
    EXPECT_EQ(stream.video->color.range, "limited");

    const auto thumbnail = source.value()->thumbnail(
        selection, selection.video.selected.front(), 0, 8, 16);
    ASSERT_TRUE(thumbnail) << core::to_string(thumbnail.error().code);
    EXPECT_EQ(thumbnail.value().geometry.coded_width, 8);
    EXPECT_EQ(thumbnail.value().geometry.coded_height, 16);
    EXPECT_EQ(thumbnail.value().lease.byte_size(), 8U * 16U * 4U);
    EXPECT_EQ(thumbnail.value().color.range, "full");

    const auto proxy = source.value()->proxy_frame(
        selection, selection.video.selected.front(), 0, 8, 8);
    ASSERT_TRUE(proxy) << core::to_string(proxy.error().code);
    EXPECT_EQ(proxy.value().geometry.coded_width, 4);
    EXPECT_EQ(proxy.value().geometry.coded_height, 8);
    EXPECT_LE(proxy.value().lease.byte_size(), 8U * 8U * 4U);
    EXPECT_EQ(proxy.value().color.range, "full");
}

TEST(MediaDecode, AnnouncesDynamicFormatEpochBeforeAffectedFrame)
{
    const auto source = media::MediaSource::open(golden("dynamic_format.ts"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    std::vector<std::pair<std::uint64_t, media::Rational>> announced;
    std::vector<std::uint64_t> frame_epochs;
    const auto decoded = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {0, 0, false},
        media::DecodeLimits{},
        {[&](const media::FormatChanged& change) {
             EXPECT_TRUE(change.video);
             if (change.video) {
                 announced.emplace_back(
                     change.format_epoch,
                     media::Rational{change.video->geometry.coded_width,
                                     change.video->geometry.coded_height});
             }
             return media::PublishResult::accepted;
         },
         [&](media::VideoFrame frame) {
             frame_epochs.push_back(frame.lease.format_epoch());
             EXPECT_TRUE(std::ranges::any_of(announced, [&](const auto& entry) {
                 return entry.first == frame.lease.format_epoch();
             }));
             return media::PublishResult::accepted;
         }});
    ASSERT_TRUE(decoded) << core::to_string(decoded.error().code);
    ASSERT_GE(announced.size(), 2U);
    EXPECT_EQ(announced[0], (std::pair<std::uint64_t, media::Rational>{1, {16, 16}}));
    EXPECT_EQ(announced[1], (std::pair<std::uint64_t, media::Rational>{2, {32, 16}}));
    EXPECT_TRUE(std::ranges::find(frame_epochs, 2U) != frame_epochs.end());
}

TEST(MediaDecode, ProducesContinuousWaveformSourcePcmAtDeclaredRate)
{
    for (const auto& [file, input_rate] : std::array{
             std::pair{"audio_44100.wav", 44100U},
             std::pair{"audio_48000.wav", 48000U}}) {
        const auto source = media::MediaSource::open(golden(file));
        ASSERT_TRUE(source);
        const auto selection = select_audio(source.value());
        std::uint64_t samples = 0;
        std::optional<std::int64_t> next;
        std::string segment;
        const auto decoded = source.value()->decode_audio(
            selection,
            selection.audio.selected.front(),
            {44100, 1},
            media::DecodeLimits{},
            {{}, [&](media::PcmBuffer pcm) {
                 if (next) {
                     EXPECT_EQ(pcm.first_sample_index, *next);
                     EXPECT_EQ(pcm.segment_id, segment);
                 } else {
                     segment = pcm.segment_id;
                 }
                 next = pcm.first_sample_index + static_cast<std::int64_t>(pcm.sample_count);
                 samples += pcm.sample_count;
                 EXPECT_EQ(pcm.sample_format, "flt");
                 EXPECT_EQ(pcm.sample_rate, 44100U);
                 EXPECT_EQ(pcm.lease.byte_size(), pcm.sample_count * sizeof(float));
                 return media::PublishResult::accepted;
             }});
        ASSERT_TRUE(decoded) << "input rate " << input_rate << ": "
                             << core::to_string(decoded.error().code);
        EXPECT_EQ(samples, 4410U) << "input rate " << input_rate;
        EXPECT_EQ(decoded.value().samples_published, 4410U)
            << "input rate " << input_rate;
    }
}

TEST(MediaLifecycle, CancellationIsDistinctAndLeasesOutliveDecoder)
{
    const auto source = media::MediaSource::open(golden("cfr_av.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    core::CancellationToken cancellation;
    media::BufferLease retained;
    const auto decoded = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {16, 16, false},
        media::DecodeLimits{},
        {{}, [&](media::VideoFrame frame) {
             retained = frame.lease;
             cancellation.cancel();
             return media::PublishResult::accepted;
         }},
        &cancellation);
    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().category, core::ErrorCategory::cancelled);
    EXPECT_TRUE(retained);
    EXPECT_EQ(retained.byte_size(), 16U * 16U * 4U);
}

TEST(MediaLifecycle, BackpressureAndResourceLimitsAreExplicit)
{
    const auto source = media::MediaSource::open(golden("cfr_av.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    const auto blocked = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {16, 16, false},
        media::DecodeLimits{},
        {{}, [](media::VideoFrame) { return media::PublishResult::would_block; }});
    ASSERT_TRUE(blocked);
    EXPECT_EQ(blocked.value().terminal_publish_result, media::PublishResult::would_block);
    EXPECT_FALSE(blocked.value().end_of_stream);

    auto limits = media::DecodeLimits{};
    limits.max_frames = 2;
    const auto limited = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {16, 16, false},
        limits,
        {{}, [](media::VideoFrame) { return media::PublishResult::accepted; }});
    ASSERT_FALSE(limited);
    EXPECT_EQ(limited.error().category, core::ErrorCategory::resource_limit);
    EXPECT_EQ(limited.error().code, core::ErrorCode::resource_limit);
}

TEST(MediaLifecycle, QueueRetainsByteQuotaUntilLeaseRelease)
{
    media::BoundedMediaQueue queue{2, 4};
    media::VideoFrame first;
    first.lease = media::BufferLease::from_bytes(
        std::vector<std::byte>(4), 1, "queue-first");
    EXPECT_EQ(queue.publish(std::move(first)), media::PublishResult::accepted);
    EXPECT_EQ(queue.outstanding_bytes(), 4U);

    media::VideoFrame blocked;
    blocked.lease = media::BufferLease::from_bytes(
        std::vector<std::byte>(1), 1, "queue-blocked");
    EXPECT_EQ(queue.publish(std::move(blocked)), media::PublishResult::would_block);

    auto taken = queue.try_take();
    ASSERT_TRUE(taken);
    EXPECT_EQ(queue.queued_items(), 0U);
    EXPECT_EQ(queue.outstanding_bytes(), 4U);
    queue.drain();
    EXPECT_EQ(queue.state(), media::ChannelState::draining);
    taken.reset();
    EXPECT_EQ(queue.outstanding_bytes(), 0U);
    EXPECT_EQ(queue.state(), media::ChannelState::ended);

    media::VideoFrame closed;
    closed.lease = media::BufferLease::from_bytes(
        std::vector<std::byte>(1), 2, "queue-closed");
    EXPECT_EQ(queue.publish(std::move(closed)), media::PublishResult::closed);
}

TEST(MediaSeek, DecodesPrerollAndUsesVfrNearestTieBreak)
{
    const auto source = media::MediaSource::open(golden("vfr_video.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    const media::SeekRequest request{selection.video.selected.front(),
                                     120'000'000,
                                     media::SeekMode::nearest,
                                     100'000'000,
                                     100'000'000};
    const auto result = source.value()->seek_video(
        selection, request, {16, 16, false}, media::DecodeLimits{});
    ASSERT_TRUE(result) << core::to_string(result.error().code);
    EXPECT_EQ(result.value().actual_frame_time_ns, 100'000'000);
    EXPECT_EQ(result.value().error_ns, -20'000'000);
    EXPECT_FALSE(result.value().exact);
    EXPECT_GT(result.value().decoded_preroll_frames, 0U);
}

TEST(MediaResource, LongMaterialKeepsWorkingBuffersBounded)
{
    const auto source = media::MediaSource::open(golden("long_bounded.mkv"));
    ASSERT_TRUE(source);
    const auto selection = select_video(source.value());
    std::uint64_t frames = 0;
    const auto decoded = source.value()->decode_video(
        selection,
        selection.video.selected.front(),
        {16, 16, false},
        media::DecodeLimits{},
        {{}, [&](media::VideoFrame frame) {
             ++frames;
             EXPECT_EQ(frame.lease.byte_size(), 1024U);
             return media::PublishResult::accepted;
         }});
    ASSERT_TRUE(decoded) << core::to_string(decoded.error().code);
    EXPECT_EQ(frames, 500U);
    EXPECT_EQ(decoded.value().peak_single_buffer_bytes, 1024U);
    EXPECT_EQ(decoded.value().bytes_published, frames * 1024U);
}

} // namespace
