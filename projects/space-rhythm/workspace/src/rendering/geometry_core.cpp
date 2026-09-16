#include <space_rhythm/rendering/geometry_core.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ranges>
#include <sstream>
#include <tuple>
#include <utility>

namespace space_rhythm::rendering {
namespace {

constexpr std::uint32_t maximum_target_extent_px = 32'768;
constexpr std::uint64_t ppm_denominator = 1'000'000;
constexpr std::size_t vertices_per_quad = 6;

constexpr std::array<std::uint32_t, 64> sha256_round_constants{
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

std::atomic_uint64_t next_diagnostic_id{1};

core::ErrorInfo make_error(core::ErrorCategory category,
                           core::ErrorCode code,
                           std::string_view stage,
                           bool retryable = false)
{
    const auto sequence = next_diagnostic_id.fetch_add(1, std::memory_order_relaxed);
    return core::ErrorInfo{core::schema_version,
                           category,
                           code,
                           std::string{stage},
                           "geometry:" + std::to_string(sequence),
                           retryable,
                           std::string{core::to_string(code)},
                           {},
                           {}};
}

core::ErrorInfo invalid(std::string_view stage)
{
    return make_error(
        core::ErrorCategory::validation, core::ErrorCode::invalid_dto, stage);
}

core::ErrorInfo unsupported(std::string_view stage)
{
    return make_error(core::ErrorCategory::compatibility,
                      core::ErrorCode::unsupported_feature,
                      stage);
}

core::ErrorInfo stale(std::string_view stage)
{
    return make_error(
        core::ErrorCategory::conflict, core::ErrorCode::stale_revision, stage, true);
}

std::string sha256_hex(std::string_view input)
{
    std::array<std::uint32_t, 8> hash{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    std::vector<std::byte> padded;
    padded.reserve(input.size() + 72U);
    for (const char character : input) {
        padded.push_back(static_cast<std::byte>(
            static_cast<unsigned char>(character)));
    }
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
            const auto temporary1 = h + sum1 + choice
                + sha256_round_constants[index] + words[index];
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

std::vector<IntegerParameterSpec> common_parameters()
{
    return {
        {"event-marker-width-milli-px", 500, 16'000, 2'000},
        {"event-rgba", 0, 0xffff'ffffLL, 0xffd1'66ffLL},
        {"lod-samples-per-pixel", 1, 64, 4},
        {"max-total-vertices", 6'000, 4'000'000, 786'432},
        {"max-vertices-per-batch", 600, 1'200'000, 65'532},
        {"primary-rgba", 0, 0xffff'ffffLL, 0x36d8'ffffLL},
        {"secondary-rgba", 0, 0xffff'ffffLL, 0x9068'ffffLL},
        {"timeline-height-ppm", 50'000, 400'000, 180'000},
    };
}

VisualTemplateDefinition make_waveform_definition()
{
    auto parameters = common_parameters();
    parameters.push_back({"amplitude-ppm", 100'000, 1'000'000, 900'000});
    parameters.push_back({"line-width-milli-px", 250, 8'000, 1'500});
    return VisualTemplateDefinition{VisualTemplate::waveform_oscilloscope,
                                    "space-rhythm.waveform-oscilloscope",
                                    "1.0.0",
                                    std::move(parameters)};
}

VisualTemplateDefinition make_spectrum_definition()
{
    auto parameters = common_parameters();
    parameters.push_back({"amplitude-ppm", 100'000, 1'000'000, 900'000});
    parameters.push_back({"bar-gap-ppm", 0, 900'000, 150'000});
    parameters.push_back({"minimum-bar-height-milli-px", 0, 10'000, 1'000});
    return VisualTemplateDefinition{VisualTemplate::spectrum_geometry,
                                    "space-rhythm.spectrum-geometry",
                                    "1.0.0",
                                    std::move(parameters)};
}

VisualTemplateDefinition make_pulse_definition()
{
    auto parameters = common_parameters();
    parameters.push_back({"jitter-ppm", 0, 1'000'000, 250'000});
    parameters.push_back({"lifetime-ms", 16, 5'000, 750});
    parameters.push_back({"line-width-milli-px", 250, 8'000, 1'500});
    parameters.push_back({"particles-per-event", 1, 32, 8});
    parameters.push_back({"travel-distance-ppm", 10'000, 1'000'000, 250'000});
    return VisualTemplateDefinition{VisualTemplate::rhythm_line_pulse,
                                    "space-rhythm.rhythm-line-pulse",
                                    "1.0.0",
                                    std::move(parameters)};
}

const std::array<VisualTemplateDefinition, 3>& definitions()
{
    static const std::array<VisualTemplateDefinition, 3> values{
        make_waveform_definition(), make_spectrum_definition(), make_pulse_definition()};
    return values;
}

const IntegerParameterSpec* find_spec(const VisualTemplateDefinition& definition,
                                      std::string_view name)
{
    const auto found = std::ranges::find(definition.integer_parameters,
                                         name,
                                         &IntegerParameterSpec::name);
    return found == definition.integer_parameters.end() ? nullptr : &*found;
}

std::int64_t parameter(const TemplateParameterBlock& parameters,
                       std::string_view name)
{
    return parameters.integer_parameters.at(std::string{name});
}

struct Color final {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{};
};

Color unpack_rgba(std::int64_t packed)
{
    const auto value = static_cast<std::uint32_t>(packed);
    return Color{static_cast<std::uint8_t>((value >> 24U) & 0xffU),
                 static_cast<std::uint8_t>((value >> 16U) & 0xffU),
                 static_cast<std::uint8_t>((value >> 8U) & 0xffU),
                 static_cast<std::uint8_t>(value & 0xffU)};
}

GeometryVertex vertex(float x, float y, Color color)
{
    return GeometryVertex{x, y, color.red, color.green, color.blue, color.alpha};
}

struct VertexBudget final {
    explicit VertexBudget(std::size_t maximum)
        : remaining(maximum)
    {
    }

    bool consume_quad()
    {
        if (remaining < vertices_per_quad) {
            return false;
        }
        remaining -= vertices_per_quad;
        return true;
    }

    std::size_t remaining{};
};

bool append_rect(std::vector<GeometryVertex>& vertices,
                 float left,
                 float top,
                 float right,
                 float bottom,
                 float clip_width,
                 float clip_height,
                 Color color,
                 VertexBudget& budget)
{
    left = std::clamp(left, 0.0F, clip_width);
    right = std::clamp(right, 0.0F, clip_width);
    top = std::clamp(top, 0.0F, clip_height);
    bottom = std::clamp(bottom, 0.0F, clip_height);
    if (!(right > left && bottom > top) || !budget.consume_quad()) {
        return false;
    }
    vertices.push_back(vertex(left, top, color));
    vertices.push_back(vertex(right, top, color));
    vertices.push_back(vertex(right, bottom, color));
    vertices.push_back(vertex(left, top, color));
    vertices.push_back(vertex(right, bottom, color));
    vertices.push_back(vertex(left, bottom, color));
    return true;
}

bool clip_line(float& x0,
               float& y0,
               float& x1,
               float& y1,
               float width,
               float height)
{
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    float entering = 0.0F;
    float leaving = 1.0F;
    const std::array<float, 4> p{-dx, dx, -dy, dy};
    const std::array<float, 4> q{x0, width - x0, y0, height - y0};
    for (std::size_t index = 0; index < p.size(); ++index) {
        if (p[index] == 0.0F) {
            if (q[index] < 0.0F) {
                return false;
            }
            continue;
        }
        const float ratio = q[index] / p[index];
        if (p[index] < 0.0F) {
            entering = std::max(entering, ratio);
        } else {
            leaving = std::min(leaving, ratio);
        }
        if (entering > leaving) {
            return false;
        }
    }
    const float original_x = x0;
    const float original_y = y0;
    x0 = original_x + entering * dx;
    y0 = original_y + entering * dy;
    x1 = original_x + leaving * dx;
    y1 = original_y + leaving * dy;
    return true;
}

bool append_line_quad(std::vector<GeometryVertex>& vertices,
                      float x0,
                      float y0,
                      float x1,
                      float y1,
                      float thickness,
                      float clip_width,
                      float clip_height,
                      Color color,
                      VertexBudget& budget)
{
    if (!clip_line(x0, y0, x1, y1, clip_width, clip_height)) {
        return false;
    }
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (!(length > 0.0F) || !std::isfinite(length) || !budget.consume_quad()) {
        return false;
    }
    const float half = std::max(0.25F, thickness * 0.5F);
    const float px = -dy / length * half;
    const float py = dx / length * half;
    const auto clamped_vertex = [&](float x, float y) {
        return vertex(std::clamp(x, 0.0F, clip_width),
                      std::clamp(y, 0.0F, clip_height),
                      color);
    };
    const auto a = clamped_vertex(x0 + px, y0 + py);
    const auto b = clamped_vertex(x0 - px, y0 - py);
    const auto c = clamped_vertex(x1 - px, y1 - py);
    const auto d = clamped_vertex(x1 + px, y1 + py);
    vertices.insert(vertices.end(), {a, b, c, a, c, d});
    return true;
}

std::optional<std::uint32_t> time_pixel(const RenderSnapshot& snapshot,
                                        core::TimeRange viewport,
                                        std::uint32_t width,
                                        core::TimeNs time_ns)
{
    const CoordinateTransform transform{snapshot.id(),
                                        snapshot.recipe().timeline_revision,
                                        viewport,
                                        ItemRectSp{0,
                                                   0,
                                                   static_cast<std::int64_t>(width)
                                                       * subpixels_per_logical_pixel,
                                                   subpixels_per_logical_pixel}};
    auto mapped = time_to_item_x_sp(transform, time_ns);
    if (!mapped) {
        return std::nullopt;
    }
    const auto pixel = mapped.value() / subpixels_per_logical_pixel;
    return static_cast<std::uint32_t>(std::clamp<std::int64_t>(
        pixel, 0, static_cast<std::int64_t>(width - 1)));
}

struct EventBucket final {
    bool occupied{false};
    core::NormPpm strength_ppm{};
    core::TimeNs time_ns{};
    std::string id;
    const core::RhythmEvent* event{};
    std::uint32_t count{};
};

std::vector<EventBucket> visible_event_buckets(const GeometryBuildRequest& request,
                                               GeometryBuildStats& stats)
{
    std::vector<EventBucket> buckets(request.target_width_px);
    const auto& events = request.snapshot->timeline().events;
    stats.input_event_count = events.size();
    for (const auto& event : events) {
        if (!request.viewport_time_range.contains(event.time_ns)) {
            continue;
        }
        ++stats.visible_event_count;
        const auto x = time_pixel(*request.snapshot,
                                  request.viewport_time_range,
                                  request.target_width_px,
                                  event.time_ns);
        if (!x) {
            continue;
        }
        auto& bucket = buckets[*x];
        ++bucket.count;
        if (!bucket.occupied
            || std::tie(event.strength_ppm, event.id.value)
                > std::tie(bucket.strength_ppm, bucket.id)) {
            bucket.occupied = true;
            bucket.strength_ppm = event.strength_ppm;
            bucket.time_ns = event.time_ns;
            bucket.id = event.id.value;
            bucket.event = &event;
        }
    }
    return buckets;
}

std::vector<GeometryVertex> build_event_timeline(
    const GeometryBuildRequest& request,
    const TemplateParameterBlock& parameters,
    const std::vector<EventBucket>& buckets,
    VertexBudget& budget,
    GeometryBuildStats& stats)
{
    std::vector<GeometryVertex> vertices;
    const float width = static_cast<float>(request.target_width_px);
    const float height = static_cast<float>(request.target_height_px);
    const float timeline_height = height
        * static_cast<float>(parameter(parameters, "timeline-height-ppm"))
        / static_cast<float>(ppm_denominator);
    const float marker_width = static_cast<float>(
                                   parameter(parameters, "event-marker-width-milli-px"))
        / 1'000.0F;
    const auto color = unpack_rgba(parameter(parameters, "event-rgba"));
    for (std::size_t index = 0; index < buckets.size(); ++index) {
        const auto& bucket = buckets[index];
        if (!bucket.occupied) {
            continue;
        }
        const float marker_height = std::max(
            1.0F,
            timeline_height * static_cast<float>(bucket.strength_ppm)
                / static_cast<float>(core::norm_ppm_max));
        if (append_rect(vertices,
                        static_cast<float>(index) + 0.5F - marker_width * 0.5F,
                        height - marker_height,
                        static_cast<float>(index) + 0.5F + marker_width * 0.5F,
                        height,
                        width,
                        height,
                        color,
                        budget)) {
            ++stats.emitted_primitive_count;
        }
    }
    return vertices;
}

struct WaveBucket final {
    bool occupied{false};
    float minimum{1.0F};
    float maximum{-1.0F};
    std::uint32_t count{};
};

float normalized_wave_value(core::NormPpm value)
{
    return static_cast<float>(value) / 500'000.0F - 1.0F;
}

std::vector<GeometryVertex> build_waveform(const GeometryBuildRequest& request,
                                           const TemplateParameterBlock& parameters,
                                           VertexBudget& budget,
                                           GeometryBuildStats& stats)
{
    std::vector<WaveBucket> buckets(request.target_width_px);
    std::uint32_t maximum_bucket_count = 0;
    for (const auto& series : request.snapshot->series()) {
        stats.input_sample_count += series.samples.size();
        auto current = std::lower_bound(
            series.samples.begin(),
            series.samples.end(),
            request.viewport_time_range.start_ns,
            [](const RenderSeriesSample& sample, core::TimeNs time) {
                return sample.time_ns < time;
            });
        while (current != series.samples.end()
               && current->time_ns < request.viewport_time_range.end_ns) {
            ++stats.visible_sample_count;
            const auto x = time_pixel(*request.snapshot,
                                      request.viewport_time_range,
                                      request.target_width_px,
                                      current->time_ns);
            if (x) {
                auto& bucket = buckets[*x];
                const float first = normalized_wave_value(current->primary_ppm);
                const float second = current->secondary_ppm
                    ? normalized_wave_value(*current->secondary_ppm)
                    : first;
                bucket.occupied = true;
                bucket.minimum = std::min({bucket.minimum, first, second});
                bucket.maximum = std::max({bucket.maximum, first, second});
                ++bucket.count;
                maximum_bucket_count = std::max(maximum_bucket_count, bucket.count);
            }
            ++current;
        }
    }
    const auto samples_per_pixel = static_cast<std::uint32_t>(
        parameter(parameters, "lod-samples-per-pixel"));
    if (maximum_bucket_count > samples_per_pixel) {
        stats.lod_level = std::max(
            stats.lod_level,
            static_cast<std::uint32_t>((maximum_bucket_count + samples_per_pixel - 1)
                                       / samples_per_pixel));
    }

    std::vector<GeometryVertex> vertices;
    const float width = static_cast<float>(request.target_width_px);
    const float height = static_cast<float>(request.target_height_px);
    const float visual_bottom = height
        * (1.0F
           - static_cast<float>(parameter(parameters, "timeline-height-ppm"))
               / static_cast<float>(ppm_denominator));
    const float center = visual_bottom * 0.5F;
    const float amplitude = center
        * static_cast<float>(parameter(parameters, "amplitude-ppm"))
        / static_cast<float>(ppm_denominator);
    const float thickness = static_cast<float>(
                                parameter(parameters, "line-width-milli-px"))
        / 1'000.0F;
    const auto color = unpack_rgba(parameter(parameters, "primary-rgba"));
    for (std::size_t index = 0; index < buckets.size(); ++index) {
        const auto& bucket = buckets[index];
        if (!bucket.occupied) {
            continue;
        }
        const float y0 = center - bucket.maximum * amplitude;
        float y1 = center - bucket.minimum * amplitude;
        if (std::abs(y1 - y0) < 0.5F) {
            y1 = y0 + 0.5F;
        }
        if (append_line_quad(vertices,
                             static_cast<float>(index) + 0.5F,
                             y0,
                             static_cast<float>(index) + 0.5F,
                             y1,
                             thickness,
                             width,
                             visual_bottom,
                             color,
                             budget)) {
            ++stats.emitted_primitive_count;
        }
    }
    return vertices;
}

std::vector<GeometryVertex> build_spectrum(const GeometryBuildRequest& request,
                                           const TemplateParameterBlock& parameters,
                                           core::TimeNs frame_time,
                                           VertexBudget& budget,
                                           GeometryBuildStats& stats)
{
    std::vector<core::NormPpm> magnitudes;
    magnitudes.reserve(request.snapshot->series().size());
    for (const auto& series : request.snapshot->series()) {
        stats.input_sample_count += series.samples.size();
        const auto upper = std::upper_bound(
            series.samples.begin(),
            series.samples.end(),
            frame_time,
            [](core::TimeNs time, const RenderSeriesSample& sample) {
                return time < sample.time_ns;
            });
        if (upper == series.samples.begin()) {
            continue;
        }
        magnitudes.push_back(std::prev(upper)->primary_ppm);
    }
    stats.visible_sample_count += magnitudes.size();
    if (magnitudes.empty()) {
        return {};
    }

    const std::size_t bar_count = std::min<std::size_t>(magnitudes.size(),
                                                        request.target_width_px);
    const std::size_t group_size = (magnitudes.size() + bar_count - 1) / bar_count;
    const auto samples_per_pixel = static_cast<std::uint32_t>(
        parameter(parameters, "lod-samples-per-pixel"));
    if (group_size > samples_per_pixel) {
        stats.lod_level = std::max(
            stats.lod_level,
            static_cast<std::uint32_t>((group_size + samples_per_pixel - 1)
                                       / samples_per_pixel));
    }

    std::vector<GeometryVertex> vertices;
    const float width = static_cast<float>(request.target_width_px);
    const float height = static_cast<float>(request.target_height_px);
    const float visual_bottom = height
        * (1.0F
           - static_cast<float>(parameter(parameters, "timeline-height-ppm"))
               / static_cast<float>(ppm_denominator));
    const float slot_width = width / static_cast<float>(bar_count);
    const float gap_ratio = static_cast<float>(parameter(parameters, "bar-gap-ppm"))
        / static_cast<float>(ppm_denominator);
    const float amplitude_ratio = static_cast<float>(
                                      parameter(parameters, "amplitude-ppm"))
        / static_cast<float>(ppm_denominator);
    const float minimum_height = static_cast<float>(
                                     parameter(parameters,
                                               "minimum-bar-height-milli-px"))
        / 1'000.0F;
    const auto primary = unpack_rgba(parameter(parameters, "primary-rgba"));
    const auto secondary = unpack_rgba(parameter(parameters, "secondary-rgba"));
    for (std::size_t bar = 0; bar < bar_count; ++bar) {
        const auto begin = bar * group_size;
        const auto end = std::min(begin + group_size, magnitudes.size());
        if (begin >= end) {
            break;
        }
        const auto peak = *std::max_element(magnitudes.begin() + begin,
                                            magnitudes.begin() + end);
        const float bar_height = std::max(
            minimum_height,
            visual_bottom * amplitude_ratio * static_cast<float>(peak)
                / static_cast<float>(core::norm_ppm_max));
        const float gap = slot_width * gap_ratio;
        const Color color = (bar % 2U == 0U) ? primary : secondary;
        if (append_rect(vertices,
                        static_cast<float>(bar) * slot_width + gap * 0.5F,
                        visual_bottom - bar_height,
                        static_cast<float>(bar + 1U) * slot_width - gap * 0.5F,
                        visual_bottom,
                        width,
                        visual_bottom,
                        color,
                        budget)) {
            ++stats.emitted_primitive_count;
        }
    }
    return vertices;
}

std::uint64_t stable_hash(std::uint64_t seed,
                          std::string_view id,
                          std::uint32_t particle)
{
    std::uint64_t hash = 1469598103934665603ULL ^ seed;
    for (const unsigned char byte : id) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    hash ^= particle;
    hash *= 1099511628211ULL;
    hash ^= hash >> 30U;
    hash *= 0xbf58476d1ce4e5b9ULL;
    hash ^= hash >> 27U;
    hash *= 0x94d049bb133111ebULL;
    return hash ^ (hash >> 31U);
}

std::vector<GeometryVertex> build_pulses(
    const GeometryBuildRequest& request,
    const TemplateParameterBlock& parameters,
    core::TimeNs frame_time,
    const std::vector<EventBucket>& event_buckets,
    VertexBudget& budget,
    GeometryBuildStats& stats)
{
    constexpr std::array<std::pair<float, float>, 16> directions{
        std::pair{1.0F, 0.0F},       std::pair{0.9239F, 0.3827F},
        std::pair{0.7071F, 0.7071F}, std::pair{0.3827F, 0.9239F},
        std::pair{0.0F, 1.0F},       std::pair{-0.3827F, 0.9239F},
        std::pair{-0.7071F, 0.7071F}, std::pair{-0.9239F, 0.3827F},
        std::pair{-1.0F, 0.0F},      std::pair{-0.9239F, -0.3827F},
        std::pair{-0.7071F, -0.7071F}, std::pair{-0.3827F, -0.9239F},
        std::pair{0.0F, -1.0F},      std::pair{0.3827F, -0.9239F},
        std::pair{0.7071F, -0.7071F}, std::pair{0.9239F, -0.3827F},
    };
    std::vector<GeometryVertex> vertices;
    const auto lifetime_ms = parameter(parameters, "lifetime-ms");
    if (lifetime_ms > std::numeric_limits<core::TimeNs>::max() / 1'000'000) {
        return vertices;
    }
    const auto lifetime_ns = lifetime_ms * 1'000'000;
    const auto particles = static_cast<std::uint32_t>(
        parameter(parameters, "particles-per-event"));
    const float width = static_cast<float>(request.target_width_px);
    const float height = static_cast<float>(request.target_height_px);
    const float visual_bottom = height
        * (1.0F
           - static_cast<float>(parameter(parameters, "timeline-height-ppm"))
               / static_cast<float>(ppm_denominator));
    const float distance = visual_bottom
        * static_cast<float>(parameter(parameters, "travel-distance-ppm"))
        / static_cast<float>(ppm_denominator);
    const float jitter = static_cast<float>(parameter(parameters, "jitter-ppm"))
        / static_cast<float>(ppm_denominator);
    const float thickness = static_cast<float>(
                                parameter(parameters, "line-width-milli-px"))
        / 1'000.0F;
    const auto color = unpack_rgba(parameter(parameters, "primary-rgba"));
    std::uint32_t maximum_bucket_count = 0;
    for (std::size_t x = 0; x < event_buckets.size(); ++x) {
        const auto& bucket = event_buckets[x];
        maximum_bucket_count = std::max(maximum_bucket_count, bucket.count);
        if (!bucket.occupied || bucket.event == nullptr || bucket.time_ns > frame_time) {
            continue;
        }
        const auto age = frame_time - bucket.time_ns;
        if (age < 0 || age >= lifetime_ns) {
            continue;
        }
        const float progress = static_cast<float>(age)
            / static_cast<float>(lifetime_ns);
        const float origin_x = static_cast<float>(x) + 0.5F;
        for (std::uint32_t particle = 0; particle < particles; ++particle) {
            const auto hash = stable_hash(request.snapshot->recipe().deterministic_seed,
                                          bucket.id,
                                          particle);
            const auto& direction = directions[hash % directions.size()];
            const float jitter_unit = static_cast<float>((hash >> 8U) & 0xffffU)
                    / 65'535.0F
                - 0.5F;
            const float origin_y = visual_bottom * 0.5F
                + jitter_unit * jitter * visual_bottom;
            const float scaled_distance = distance * progress
                * (0.5F
                   + 0.5F
                       * static_cast<float>(bucket.strength_ppm)
                       / static_cast<float>(core::norm_ppm_max));
            const float end_x = origin_x + direction.first * scaled_distance;
            const float end_y = origin_y + direction.second * scaled_distance;
            if (!append_line_quad(vertices,
                                  origin_x,
                                  origin_y,
                                  end_x,
                                  end_y,
                                  thickness,
                                  width,
                                  visual_bottom,
                                  color,
                                  budget)) {
                if (budget.remaining < vertices_per_quad) {
                    break;
                }
                continue;
            }
            ++stats.emitted_primitive_count;
        }
        if (budget.remaining < vertices_per_quad) {
            break;
        }
    }
    const auto samples_per_pixel = static_cast<std::uint32_t>(
        parameter(parameters, "lod-samples-per-pixel"));
    if (maximum_bucket_count > samples_per_pixel) {
        stats.lod_level = std::max(
            stats.lod_level,
            static_cast<std::uint32_t>((maximum_bucket_count + samples_per_pixel - 1)
                                       / samples_per_pixel));
    }
    return vertices;
}

void append_batches(GeometryFrame& frame,
                    GeometryBatchKind kind,
                    std::vector<GeometryVertex> vertices,
                    std::size_t maximum_batch_vertices)
{
    maximum_batch_vertices = std::max(vertices_per_quad,
                                      maximum_batch_vertices
                                          - maximum_batch_vertices % vertices_per_quad);
    std::size_t offset = 0;
    while (offset < vertices.size()) {
        const auto count = std::min(maximum_batch_vertices, vertices.size() - offset);
        GeometryBatch batch;
        batch.kind = kind;
        batch.vertices.insert(batch.vertices.end(),
                              std::make_move_iterator(vertices.begin() + offset),
                              std::make_move_iterator(vertices.begin() + offset + count));
        frame.stats.maximum_batch_vertices = std::max(
            frame.stats.maximum_batch_vertices,
            static_cast<std::uint32_t>(batch.vertices.size()));
        frame.batches.push_back(std::move(batch));
        offset += count;
    }
}

} // namespace

const VisualTemplateDefinition& template_definition(VisualTemplate kind)
{
    return definitions().at(static_cast<std::size_t>(kind));
}

std::optional<VisualTemplate> parse_visual_template(std::string_view template_id)
{
    const auto found = std::ranges::find(definitions(),
                                         template_id,
                                         &VisualTemplateDefinition::template_id);
    if (found == definitions().end()) {
        return std::nullopt;
    }
    return found->kind;
}

std::string canonical_template_parameters(const TemplateParameterBlock& parameters)
{
    std::ostringstream canonical;
    canonical << "schema=" << parameters.schema_version << '\n'
              << "template=" << parameters.template_id << '\n'
              << "version=" << parameters.template_version << '\n';
    for (const auto& [name, value] : parameters.integer_parameters) {
        canonical << name << '=' << value << '\n';
    }
    return canonical.str();
}

std::string template_parameters_sha256(const TemplateParameterBlock& parameters)
{
    return sha256_hex(canonical_template_parameters(parameters));
}

core::Result<TemplateParameterBlock> make_template_parameters(
    VisualTemplate kind,
    std::map<std::string, std::int64_t> overrides)
{
    const auto& definition = template_definition(kind);
    TemplateParameterBlock parameters;
    parameters.template_id = definition.template_id;
    parameters.template_version = definition.template_version;
    for (const auto& spec : definition.integer_parameters) {
        parameters.integer_parameters.emplace(spec.name, spec.default_value);
    }
    for (const auto& [name, value] : overrides) {
        const auto* spec = find_spec(definition, name);
        if (spec == nullptr || value < spec->minimum || value > spec->maximum) {
            return core::Result<TemplateParameterBlock>::failure(
                invalid("geometry.parameters.override"));
        }
        parameters.integer_parameters[name] = value;
    }
    if (parameter(parameters, "max-total-vertices")
        < parameter(parameters, "max-vertices-per-batch")) {
        return core::Result<TemplateParameterBlock>::failure(
            invalid("geometry.parameters.limits"));
    }
    parameters.parameters_digest_sha256 = template_parameters_sha256(parameters);
    return core::Result<TemplateParameterBlock>::success(std::move(parameters));
}

core::Result<VisualTemplate> validate_template_parameters(
    const TemplateParameterBlock& parameters)
{
    const auto kind = parse_visual_template(parameters.template_id);
    if (!kind) {
        return core::Result<VisualTemplate>::failure(
            unsupported("geometry.parameters.template"));
    }
    const auto& definition = template_definition(*kind);
    if (parameters.schema_version != schema_version
        || parameters.template_version != definition.template_version
        || !parameters.required_features.empty() || !parameters.extensions.empty()
        || parameters.integer_parameters.size() != definition.integer_parameters.size()) {
        return core::Result<VisualTemplate>::failure(
            unsupported("geometry.parameters.version"));
    }
    for (const auto& spec : definition.integer_parameters) {
        const auto found = parameters.integer_parameters.find(spec.name);
        if (found == parameters.integer_parameters.end() || found->second < spec.minimum
            || found->second > spec.maximum) {
            return core::Result<VisualTemplate>::failure(
                invalid("geometry.parameters.range"));
        }
    }
    if (parameter(parameters, "max-total-vertices")
            < parameter(parameters, "max-vertices-per-batch")
        || parameters.parameters_digest_sha256
            != template_parameters_sha256(parameters)) {
        return core::Result<VisualTemplate>::failure(
            invalid("geometry.parameters.digest"));
    }
    return core::Result<VisualTemplate>::success(*kind);
}

core::Result<GeometryFrame> build_geometry_frame(const GeometryBuildRequest& request)
{
    if (request.schema_version != geometry_schema_version || !request.snapshot
        || request.target_width_px == 0 || request.target_height_px == 0
        || request.target_width_px > maximum_target_extent_px
        || request.target_height_px > maximum_target_extent_px
        || request.device_generation == 0) {
        return core::Result<GeometryFrame>::failure(invalid("geometry.request"));
    }
    if (request.expected_snapshot_id != request.snapshot->id()
        || request.expected_timeline_revision
            != request.snapshot->recipe().timeline_revision) {
        return core::Result<GeometryFrame>::failure(stale("geometry.request.snapshot"));
    }
    const auto& recipe = request.snapshot->recipe();
    if (request.viewport_time_range.start_ns < recipe.time_range.start_ns
        || request.viewport_time_range.end_ns > recipe.time_range.end_ns
        || request.viewport_time_range.end_ns <= request.viewport_time_range.start_ns) {
        return core::Result<GeometryFrame>::failure(invalid("geometry.request.viewport"));
    }
    auto valid_parameters = validate_template_parameters(recipe.template_parameters);
    if (!valid_parameters) {
        return core::Result<GeometryFrame>::failure(valid_parameters.error());
    }
    auto frame_time = frame_time_ns(recipe, request.frame_index);
    if (!frame_time) {
        return core::Result<GeometryFrame>::failure(frame_time.error());
    }

    GeometryFrame frame;
    frame.snapshot_id = request.snapshot->id();
    frame.timeline_revision = recipe.timeline_revision;
    frame.device_generation = request.device_generation;
    frame.frame_index = request.frame_index;
    frame.frame_time_ns = frame_time.value();
    frame.viewport_time_range = request.viewport_time_range;
    frame.target_width_px = request.target_width_px;
    frame.target_height_px = request.target_height_px;
    frame.visual_template = valid_parameters.value();
    frame.template_id = recipe.template_parameters.template_id;
    frame.template_version = recipe.template_parameters.template_version;
    frame.parameters_digest_sha256 = recipe.template_parameters.parameters_digest_sha256;

    const auto maximum_total_vertices = static_cast<std::size_t>(
        parameter(recipe.template_parameters, "max-total-vertices"));
    const auto maximum_batch_vertices = static_cast<std::size_t>(
        parameter(recipe.template_parameters, "max-vertices-per-batch"));
    VertexBudget budget{maximum_total_vertices};
    const auto event_buckets = visible_event_buckets(request, frame.stats);
    const auto maximum_event_bucket_count = std::ranges::max(
        event_buckets | std::views::transform(&EventBucket::count));
    const auto samples_per_pixel = static_cast<std::uint32_t>(
        parameter(recipe.template_parameters, "lod-samples-per-pixel"));
    if (maximum_event_bucket_count > samples_per_pixel) {
        frame.stats.lod_level = static_cast<std::uint32_t>(
            (maximum_event_bucket_count + samples_per_pixel - 1)
            / samples_per_pixel);
    }
    auto event_vertices = build_event_timeline(request,
                                               recipe.template_parameters,
                                               event_buckets,
                                               budget,
                                               frame.stats);
    append_batches(frame,
                   GeometryBatchKind::event_timeline,
                   std::move(event_vertices),
                   maximum_batch_vertices);

    switch (frame.visual_template) {
    case VisualTemplate::waveform_oscilloscope:
        append_batches(frame,
                       GeometryBatchKind::waveform,
                       build_waveform(request,
                                      recipe.template_parameters,
                                      budget,
                                      frame.stats),
                       maximum_batch_vertices);
        break;
    case VisualTemplate::spectrum_geometry:
        append_batches(frame,
                       GeometryBatchKind::spectrum,
                       build_spectrum(request,
                                      recipe.template_parameters,
                                      frame.frame_time_ns,
                                      budget,
                                      frame.stats),
                       maximum_batch_vertices);
        break;
    case VisualTemplate::rhythm_line_pulse:
        append_batches(frame,
                       GeometryBatchKind::rhythm_line_pulse,
                       build_pulses(request,
                                    recipe.template_parameters,
                                    frame.frame_time_ns,
                                    event_buckets,
                                    budget,
                                    frame.stats),
                       maximum_batch_vertices);
        break;
    }
    frame.stats.batch_count = static_cast<std::uint32_t>(frame.batches.size());
    return core::Result<GeometryFrame>::success(std::move(frame));
}

core::Result<GeometryBatchUpdatePlan> plan_geometry_batch_update(
    std::optional<GeometryBatchSyncState> previous_state,
    const GeometryFrame& frame)
{
    if (frame.schema_version != geometry_schema_version
        || frame.geometry_contract_version != geometry_contract_version
        || frame.device_generation == 0
        || frame.batches.size() > std::numeric_limits<std::uint32_t>::max()) {
        return core::Result<GeometryBatchUpdatePlan>::failure(
            invalid("geometry.batch-update.frame"));
    }
    std::uint64_t vertex_count = 0;
    for (const auto& batch : frame.batches) {
        if (batch.vertices.empty() || batch.vertices.size() % vertices_per_quad != 0
            || batch.vertices.size()
                > static_cast<std::size_t>(std::numeric_limits<int>::max())
            || vertex_count > std::numeric_limits<std::uint64_t>::max()
                    - batch.vertices.size()) {
            return core::Result<GeometryBatchUpdatePlan>::failure(
                invalid("geometry.batch-update.vertices"));
        }
        vertex_count += batch.vertices.size();
    }
    if (previous_state && previous_state->device_generation == 0) {
        return core::Result<GeometryBatchUpdatePlan>::failure(
            invalid("geometry.batch-update.previous-state"));
    }

    const auto next_count = static_cast<std::uint32_t>(frame.batches.size());
    GeometryBatchUpdatePlan plan;
    plan.upload_vertex_count = vertex_count;
    plan.next_state = GeometryBatchSyncState{frame.device_generation, next_count};
    if (!previous_state) {
        plan.create_batch_count = next_count;
        return core::Result<GeometryBatchUpdatePlan>::success(plan);
    }
    if (previous_state->device_generation != frame.device_generation) {
        plan.rebuild_for_device_generation = true;
        plan.remove_batch_count = previous_state->batch_count;
        plan.create_batch_count = next_count;
        return core::Result<GeometryBatchUpdatePlan>::success(plan);
    }
    plan.reuse_batch_count = std::min(previous_state->batch_count, next_count);
    plan.create_batch_count = next_count - plan.reuse_batch_count;
    plan.remove_batch_count = previous_state->batch_count - plan.reuse_batch_count;
    return core::Result<GeometryBatchUpdatePlan>::success(plan);
}

} // namespace space_rhythm::rendering
