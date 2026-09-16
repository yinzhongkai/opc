#pragma once

#include <space_rhythm/rendering/render_contract.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::rendering {

inline constexpr std::uint32_t geometry_schema_version = 1;
inline constexpr std::string_view geometry_contract_version{"0.1.0"};

enum class VisualTemplate {
    waveform_oscilloscope,
    spectrum_geometry,
    rhythm_line_pulse,
};

struct IntegerParameterSpec {
    std::string name;
    std::int64_t minimum{};
    std::int64_t maximum{};
    std::int64_t default_value{};

    bool operator==(const IntegerParameterSpec&) const = default;
};

struct VisualTemplateDefinition {
    VisualTemplate kind{VisualTemplate::waveform_oscilloscope};
    std::string template_id;
    std::string template_version;
    std::vector<IntegerParameterSpec> integer_parameters;

    bool operator==(const VisualTemplateDefinition&) const = default;
};

[[nodiscard]] const VisualTemplateDefinition& template_definition(VisualTemplate kind);
[[nodiscard]] std::optional<VisualTemplate> parse_visual_template(std::string_view template_id);
[[nodiscard]] std::string canonical_template_parameters(
    const TemplateParameterBlock& parameters);
[[nodiscard]] std::string template_parameters_sha256(
    const TemplateParameterBlock& parameters);

core::Result<TemplateParameterBlock> make_template_parameters(
    VisualTemplate kind,
    std::map<std::string, std::int64_t> overrides = {});
core::Result<VisualTemplate> validate_template_parameters(
    const TemplateParameterBlock& parameters);

enum class GeometryBatchKind {
    event_timeline,
    waveform,
    spectrum,
    rhythm_line_pulse,
};

struct GeometryVertex {
    float x{};
    float y{};
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{};

    bool operator==(const GeometryVertex&) const = default;
};

struct GeometryBatch {
    GeometryBatchKind kind{GeometryBatchKind::event_timeline};
    std::vector<GeometryVertex> vertices;

    bool operator==(const GeometryBatch&) const = default;
};

struct GeometryBuildStats {
    std::uint64_t input_event_count{};
    std::uint64_t visible_event_count{};
    std::uint64_t input_sample_count{};
    std::uint64_t visible_sample_count{};
    std::uint64_t emitted_primitive_count{};
    std::uint32_t lod_level{1};
    std::uint32_t batch_count{};
    std::uint32_t maximum_batch_vertices{};

    bool operator==(const GeometryBuildStats&) const = default;
};

struct GeometryBuildRequest {
    std::uint32_t schema_version{geometry_schema_version};
    std::shared_ptr<const RenderSnapshot> snapshot;
    RenderSnapshotId expected_snapshot_id;
    core::TimelineRevision expected_timeline_revision{};
    core::TimeRange viewport_time_range;
    std::uint32_t target_width_px{};
    std::uint32_t target_height_px{};
    FrameIndex frame_index{};
    std::uint64_t device_generation{};
};

struct GeometryFrame {
    std::uint32_t schema_version{geometry_schema_version};
    std::string geometry_contract_version{rendering::geometry_contract_version};
    RenderSnapshotId snapshot_id;
    core::TimelineRevision timeline_revision{};
    std::uint64_t device_generation{};
    FrameIndex frame_index{};
    core::TimeNs frame_time_ns{};
    core::TimeRange viewport_time_range;
    std::uint32_t target_width_px{};
    std::uint32_t target_height_px{};
    VisualTemplate visual_template{VisualTemplate::waveform_oscilloscope};
    std::string template_id;
    std::string template_version;
    std::string parameters_digest_sha256;
    std::vector<GeometryBatch> batches;
    GeometryBuildStats stats;

    bool operator==(const GeometryFrame&) const = default;
};

core::Result<GeometryFrame> build_geometry_frame(const GeometryBuildRequest& request);

// Backend-neutral update state shared by the on-screen scene-graph adapter and
// future off-screen consumers. It contains no Qt or GPU handle.
struct GeometryBatchSyncState {
    std::uint64_t device_generation{};
    std::uint32_t batch_count{};

    bool operator==(const GeometryBatchSyncState&) const = default;
};

struct GeometryBatchUpdatePlan {
    bool rebuild_for_device_generation{false};
    std::uint32_t create_batch_count{};
    std::uint32_t reuse_batch_count{};
    std::uint32_t remove_batch_count{};
    std::uint64_t upload_vertex_count{};
    GeometryBatchSyncState next_state;

    bool operator==(const GeometryBatchUpdatePlan&) const = default;
};

core::Result<GeometryBatchUpdatePlan> plan_geometry_batch_update(
    std::optional<GeometryBatchSyncState> previous_state,
    const GeometryFrame& frame);

} // namespace space_rhythm::rendering
