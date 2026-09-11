#pragma once

#include <space_rhythm/core/timeline.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::rendering {

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::uint32_t vector_set_version = 1;
inline constexpr std::int64_t subpixels_per_logical_pixel = 1'024;

struct RenderRecipeIdTag;
struct RenderSnapshotIdTag;
struct RenderSeriesIdTag;
struct RenderTargetIdTag;

using RenderRecipeId = core::OpaqueId<RenderRecipeIdTag>;
using RenderSnapshotId = core::OpaqueId<RenderSnapshotIdTag>;
using RenderSeriesId = core::OpaqueId<RenderSeriesIdTag>;
using RenderTargetId = core::OpaqueId<RenderTargetIdTag>;
using FrameIndex = std::uint64_t;

enum class PixelFormat {
    rgba8_unorm,
};

enum class ColorPrimaries {
    srgb,
};

enum class TransferFunction {
    srgb,
};

enum class MatrixCoefficients {
    rgb,
};

enum class ColorRange {
    full,
};

enum class AlphaMode {
    straight,
    opaque,
};

enum class RowOrder {
    top_down,
};

struct ColorDescription {
    ColorPrimaries primaries{ColorPrimaries::srgb};
    TransferFunction transfer{TransferFunction::srgb};
    MatrixCoefficients matrix{MatrixCoefficients::rgb};
    ColorRange range{ColorRange::full};

    bool operator==(const ColorDescription&) const = default;
};

struct OutputSpec {
    std::uint32_t width_px{};
    std::uint32_t height_px{};
    PixelFormat pixel_format{PixelFormat::rgba8_unorm};
    ColorDescription color;
    AlphaMode alpha_mode{AlphaMode::straight};
    RowOrder row_order{RowOrder::top_down};

    bool operator==(const OutputSpec&) const = default;
};

struct FrameRate {
    std::uint32_t numerator{};
    std::uint32_t denominator{};

    bool operator==(const FrameRate&) const = default;
};

struct FeatureInputRevision {
    core::AnalysisRevision analysis_revision;
    std::string producer_contract_version;
    std::uint32_t feature_schema_version{};
    std::string input_fingerprint_sha256;
    std::string parameters_digest_sha256;
    std::string content_digest_sha256;

    bool operator==(const FeatureInputRevision&) const = default;
};

struct TemplateParameterBlock {
    std::uint32_t schema_version{rendering::schema_version};
    std::string template_id;
    std::string template_version;
    std::string parameters_digest_sha256;
    std::map<std::string, std::int64_t> integer_parameters;
    std::vector<std::string> required_features;
    std::map<std::string, core::VersionedOpaqueObject> extensions;

    bool operator==(const TemplateParameterBlock&) const = default;
};

struct RenderRecipe {
    std::uint32_t schema_version{rendering::schema_version};
    std::string render_contract_version{contract_version};
    RenderRecipeId recipe_id;
    core::ProjectId project_id;
    core::TimelineRevision timeline_revision{};
    std::vector<FeatureInputRevision> feature_inputs;
    TemplateParameterBlock template_parameters;
    OutputSpec output;
    core::TimeRange time_range;
    FrameRate frame_rate;
    std::uint64_t deterministic_seed{};
    std::vector<std::string> required_features;
    std::map<std::string, core::VersionedOpaqueObject> extensions;

    bool operator==(const RenderRecipe&) const = default;
};

struct RenderSeriesSample {
    core::TimeNs time_ns{};
    core::NormPpm primary_ppm{};
    std::optional<core::NormPpm> secondary_ppm;

    bool operator==(const RenderSeriesSample&) const = default;
};

struct RenderSeries {
    RenderSeriesId id;
    core::AnalysisRevision analysis_revision;
    std::string definition_id;
    std::string source_contract_version;
    std::uint32_t source_schema_version{};
    std::string input_fingerprint_sha256;
    std::string parameters_digest_sha256;
    std::string content_digest_sha256;
    std::vector<RenderSeriesSample> samples;

    bool operator==(const RenderSeries&) const = default;
};

struct ContractDescriptor {
    std::uint32_t schema_version{rendering::schema_version};
    std::string render_contract_version{contract_version};
    std::vector<std::string> required_features;
    std::map<std::string, core::VersionedOpaqueObject> extensions;

    bool operator==(const ContractDescriptor&) const = default;
};

[[nodiscard]] const std::set<std::string>& supported_features();
core::Result<ContractDescriptor> negotiate_contract(ContractDescriptor descriptor);
core::Result<RenderRecipe> validate_render_recipe(RenderRecipe recipe);
core::Result<core::TimeNs> frame_time_ns(const RenderRecipe& recipe,
                                         FrameIndex frame_index);

class RenderSnapshot final {
public:
    [[nodiscard]] const RenderSnapshotId& id() const noexcept;
    [[nodiscard]] const RenderRecipe& recipe() const noexcept;
    [[nodiscard]] const core::TimelineSnapshot& timeline() const noexcept;
    [[nodiscard]] const std::vector<RenderSeries>& series() const noexcept;

private:
    RenderSnapshot(RenderSnapshotId id,
                   RenderRecipe recipe,
                   core::TimelineSnapshot timeline,
                   std::vector<RenderSeries> series);

    RenderSnapshotId id_;
    RenderRecipe recipe_;
    core::TimelineSnapshot timeline_;
    std::vector<RenderSeries> series_;

    friend core::Result<std::shared_ptr<const RenderSnapshot>> make_render_snapshot(
        RenderSnapshotId id,
        RenderRecipe recipe,
        core::TimelineSnapshot timeline,
        std::vector<RenderSeries> series);
};

core::Result<std::shared_ptr<const RenderSnapshot>> make_render_snapshot(
    RenderSnapshotId id,
    RenderRecipe recipe,
    core::TimelineSnapshot timeline,
    std::vector<RenderSeries> series);

struct ItemRectSp {
    std::int64_t x_sp{};
    std::int64_t y_sp{};
    std::int64_t width_sp{};
    std::int64_t height_sp{};

    bool operator==(const ItemRectSp&) const = default;
};

struct CoordinateTransform {
    RenderSnapshotId snapshot_id;
    core::TimelineRevision timeline_revision{};
    core::TimeRange time_range;
    ItemRectSp content_rect;

    bool operator==(const CoordinateTransform&) const = default;
};

core::Result<std::int64_t> time_to_item_x_sp(const CoordinateTransform& transform,
                                             core::TimeNs time_ns);
core::Result<core::TimeNs> item_x_sp_to_time(const CoordinateTransform& transform,
                                             std::int64_t item_x_sp);

enum class HitTargetKind {
    event,
    series_sample,
    track,
};

struct HitTestRequest {
    std::uint32_t schema_version{rendering::schema_version};
    RenderSnapshotId snapshot_id;
    core::TimelineRevision timeline_revision{};
    std::int64_t item_x_sp{};
    std::int64_t item_y_sp{};
    std::int64_t radius_sp{};

    bool operator==(const HitTestRequest&) const = default;
};

struct HitCandidate {
    HitTargetKind kind{HitTargetKind::event};
    RenderTargetId target_id;
    core::TimeNs time_ns{};
    std::uint64_t distance_squared_sp{};
    std::int32_t z_order{};

    bool operator==(const HitCandidate&) const = default;
};

struct HitTestResult {
    std::uint32_t schema_version{rendering::schema_version};
    RenderSnapshotId snapshot_id;
    core::TimelineRevision timeline_revision{};
    std::vector<HitCandidate> candidates;

    bool operator==(const HitTestResult&) const = default;
};

core::Result<HitTestRequest> validate_hit_test_request(
    HitTestRequest request,
    const RenderSnapshot& snapshot,
    const CoordinateTransform& transform);
core::Result<HitTestResult> validate_and_sort_hit_test_result(
    HitTestResult result,
    const HitTestRequest& request,
    const RenderSnapshot& snapshot);

enum class SceneGraphState {
    detached,
    awaiting_initialization,
    ready,
    cleanup_pending,
    invalidated,
};

enum class SceneGraphEvent {
    attach_window,
    initialize_scene_graph,
    request_cleanup,
    complete_cleanup,
    invalidate_scene_graph,
    detach_window,
};

struct SceneGraphEpoch {
    SceneGraphState state{SceneGraphState::detached};
    std::uint64_t device_generation{};

    bool operator==(const SceneGraphEpoch&) const = default;
};

class SceneGraphLifecycle final {
public:
    [[nodiscard]] SceneGraphEpoch current() const noexcept;
    core::Result<SceneGraphEpoch> apply(SceneGraphEvent event);

private:
    SceneGraphEpoch current_;
};

class FrameLease final {
public:
    FrameLease();
    explicit FrameLease(std::vector<std::byte> bytes);

    [[nodiscard]] const std::byte* data() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t use_count() const noexcept;

private:
    struct State;
    std::shared_ptr<State> state_;

    [[nodiscard]] bool attach_lifetime_guard(std::shared_ptr<void> guard);
    friend class BoundedFrameQueue;
};

struct RenderedFrame {
    std::uint32_t schema_version{rendering::schema_version};
    std::string render_contract_version{contract_version};
    RenderSnapshotId snapshot_id;
    core::TimelineRevision timeline_revision{};
    std::uint64_t device_generation{};
    FrameIndex frame_index{};
    core::TimeNs time_ns{};
    std::uint32_t width_px{};
    std::uint32_t height_px{};
    PixelFormat pixel_format{PixelFormat::rgba8_unorm};
    ColorDescription color;
    AlphaMode alpha_mode{AlphaMode::straight};
    RowOrder row_order{RowOrder::top_down};
    std::uint64_t stride_bytes{};
    std::uint64_t valid_bytes{};
    FrameLease bytes;
};

core::Result<RenderedFrame> validate_rendered_frame(RenderedFrame frame,
                                                    const RenderSnapshot& snapshot);

enum class FrameQueueState {
    open,
    draining,
    ended,
    cancelled,
    failed,
};

enum class FramePublishStatus {
    accepted,
    would_block,
    closed,
    cancelled,
    failed,
    invalid,
};

enum class FrameTakeStatus {
    frame,
    empty,
    ended,
    cancelled,
    failed,
};

enum class FrameFailureCode {
    device_lost,
    render_failed,
};

struct FrameFailure {
    FrameFailureCode code{FrameFailureCode::render_failed};
    std::uint64_t device_generation{};
    std::string diagnostic_id;

    bool operator==(const FrameFailure&) const = default;
};

struct FrameTakeResult {
    FrameTakeStatus status{FrameTakeStatus::empty};
    std::optional<RenderedFrame> frame;
    std::optional<FrameFailure> failure;
};

class BoundedFrameQueue final {
public:
    BoundedFrameQueue(std::size_t max_outstanding_frames,
                      std::uint64_t max_outstanding_bytes);
    ~BoundedFrameQueue();

    BoundedFrameQueue(const BoundedFrameQueue&) = delete;
    BoundedFrameQueue& operator=(const BoundedFrameQueue&) = delete;
    BoundedFrameQueue(BoundedFrameQueue&&) noexcept;
    BoundedFrameQueue& operator=(BoundedFrameQueue&&) noexcept;

    [[nodiscard]] FramePublishStatus publish(RenderedFrame frame);
    [[nodiscard]] FrameTakeResult try_take();
    void drain();
    void cancel();
    void fail(FrameFailure failure);

    [[nodiscard]] FrameQueueState state() const;
    [[nodiscard]] std::size_t outstanding_frames() const;
    [[nodiscard]] std::uint64_t outstanding_bytes() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

} // namespace space_rhythm::rendering
