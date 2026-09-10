#include <gtest/gtest.h>

#include <space_rhythm/media/media.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace media = space_rhythm::media;

std::filesystem::path golden(const char* file)
{
    return std::filesystem::path{SPACE_RHYTHM_GOLDEN_MEDIA_DIR} / file;
}

media::StreamSelectionRequest required_video()
{
    return {media::SelectionMode::required_default_then_lowest_index,
            std::nullopt,
            false};
}

media::StreamSelectionRequest optional_audio()
{
    return {media::SelectionMode::optional_default_then_lowest_index,
            std::nullopt,
            false};
}

media::MediaSelection select_video(const std::shared_ptr<media::MediaSource>& source)
{
    const auto selected = source->select(required_video(), optional_audio());
    if (!selected) {
        ADD_FAILURE() << core::to_string(selected.error().code);
        return {};
    }
    return selected.value();
}

class TemporaryMediaFile final {
public:
    explicit TemporaryMediaFile(std::filesystem::path path)
        : path_(std::move(path))
    {
    }

    ~TemporaryMediaFile()
    {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    TemporaryMediaFile(const TemporaryMediaFile&) = delete;
    TemporaryMediaFile& operator=(const TemporaryMediaFile&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

TemporaryMediaFile write_bmp_without_color_metadata()
{
    const auto path = std::filesystem::temp_directory_path()
        / "t021-public-color-range-unknown.bmp";
    constexpr std::array<unsigned char, 58> bytes{
        0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
        0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
        0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00};
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    output.close();
    return TemporaryMediaFile{path};
}

TEST(T021MediaContract, PresentationTimeUsesLiteralPtsOraclesAndReportsOverflow)
{
    const media::PresentationOrigin zero{
        {0, {1, 1}, media::TimestampOrigin::format_start}, false};
    const auto ntsc = media::map_presentation_time(
        {1, {1001, 30000}, media::TimestampOrigin::frame_pts}, zero);
    ASSERT_TRUE(ntsc);
    EXPECT_EQ(ntsc.value(), 33'366'667) << "A-014 MT-TIME-004";

    const auto overflow = media::map_presentation_time(
        {std::numeric_limits<std::int64_t>::max(),
         {1, 1},
         media::TimestampOrigin::frame_pts},
        zero);
    ASSERT_FALSE(overflow);
    EXPECT_EQ(overflow.error().code, core::ErrorCode::time_overflow);
}

TEST(T021MediaContract, CfrAndVfrFollowPublishedPtsInsteadOfFrameIndex)
{
    struct FixtureOracle {
        const char* fixture_id;
        const char* file;
        std::vector<core::TimeNs> expected;
    };
    const std::vector<FixtureOracle> fixtures{
        {"GM-CFR-001", "cfr_av.mkv", {0, 40'000'000, 80'000'000, 120'000'000,
                                        160'000'000}},
        {"GM-VFR-001", "vfr_video.mkv", {0, 40'000'000, 100'000'000, 140'000'000,
                                           240'000'000}},
    };

    for (const auto& fixture : fixtures) {
        SCOPED_TRACE(fixture.fixture_id);
        const auto source = media::MediaSource::open(golden(fixture.file));
        ASSERT_TRUE(source);
        const auto selection = select_video(source.value());
        ASSERT_EQ(selection.video.selected.size(), 1U);
        std::vector<core::TimeNs> actual;
        const auto decoded = source.value()->decode_video(
            selection,
            selection.video.selected.front(),
            {16, 16, false},
            media::DecodeLimits{},
            {{}, [&](media::VideoFrame frame) {
                 actual.push_back(frame.time_ns);
                 return media::PublishResult::accepted;
             }});
        ASSERT_TRUE(decoded) << core::to_string(decoded.error().code);
        EXPECT_EQ(actual, fixture.expected);
    }
}

TEST(T021MediaContract, RotationSarAndColorAreExposedAsNormalizedMetadata)
{
    const auto source = media::MediaSource::open(golden("rotated_sar.mkv"));
    ASSERT_TRUE(source) << "GM-ROT-SAR-001";
    const auto selection = select_video(source.value());
    ASSERT_EQ(selection.video.selected.size(), 1U);
    const auto stream_index = selection.video.selected.front().stream_index;
    ASSERT_GE(stream_index, 0);
    ASSERT_LT(static_cast<std::size_t>(stream_index), source.value()->info().streams.size());
    const auto& stream = source.value()->info().streams[static_cast<std::size_t>(stream_index)];
    ASSERT_TRUE(stream.video);
    ASSERT_TRUE(stream.video->geometry.sample_aspect_ratio);
    ASSERT_TRUE(stream.video->geometry.display_transform);

    EXPECT_EQ(stream.video->geometry.coded_width, 16);
    EXPECT_EQ(stream.video->geometry.coded_height, 8);
    EXPECT_EQ(*stream.video->geometry.sample_aspect_ratio, (media::Rational{4, 3}));
    EXPECT_EQ(stream.video->geometry.display_transform->clockwise_rotation_degrees, 90);
    EXPECT_EQ(stream.video->color.primaries, "bt709");
    EXPECT_EQ(stream.video->color.transfer, "bt709");
    EXPECT_EQ(stream.video->color.matrix, "bt709");
    EXPECT_EQ(stream.video->color.range, "limited");

    const auto thumbnail = source.value()->thumbnail(
        selection, selection.video.selected.front(), 0, 8, 16);
    ASSERT_TRUE(thumbnail) << core::to_string(thumbnail.error().code);
    EXPECT_EQ(thumbnail.value().color.range, "full");
}

TEST(T021MediaContract, MissingSourceColorRangeIsExposedAsUnknown)
{
    const auto fixture = write_bmp_without_color_metadata();
    const auto source = media::MediaSource::open(fixture.path());
    ASSERT_TRUE(source) << core::to_string(source.error().code);
    ASSERT_EQ(source.value()->info().streams.size(), 1U);
    const auto& stream = source.value()->info().streams.front();
    ASSERT_TRUE(stream.video);
    EXPECT_EQ(stream.video->color.range, "unknown");
}

} // namespace
