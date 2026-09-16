#pragma once

#include <space_rhythm/rendering/geometry_core.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace space_rhythm::rendering {

inline constexpr std::uint32_t offscreen_schema_version = 1;
inline constexpr std::string_view offscreen_contract_version{"0.1.0"};

enum class GraphicsBackend {
    default_gpu_d3d11,
    qt_software,
};

enum class BackendPreference {
    default_gpu_with_software_fallback,
    default_gpu_only,
    software_only,
};

enum class CapabilityAvailability {
    available,
    unavailable,
};

struct GraphicsCapability {
    GraphicsBackend backend{GraphicsBackend::qt_software};
    CapabilityAvailability availability{CapabilityAvailability::unavailable};
    bool hardware_accelerated{false};
    std::string graphics_api;
    std::string adapter_name;
    std::uint32_t vendor_id{};
    std::uint32_t device_id{};
    std::optional<std::uint64_t> dedicated_video_memory_capacity_bytes;
    std::string diagnostic_id;
    std::string detail;

    bool operator==(const GraphicsCapability&) const = default;
};

struct GraphicsCapabilityReport {
    std::uint32_t schema_version{offscreen_schema_version};
    std::string contract_version{offscreen_contract_version};
    std::vector<GraphicsCapability> capabilities;

    bool operator==(const GraphicsCapabilityReport&) const = default;
};

[[nodiscard]] GraphicsCapabilityReport probe_graphics_capabilities();

struct BackendSelection {
    GraphicsBackend backend{GraphicsBackend::qt_software};
    bool used_fallback{false};
    std::string diagnostic_id;
    std::string detail;

    bool operator==(const BackendSelection&) const = default;
};

core::Result<BackendSelection> select_graphics_backend(
    BackendPreference preference,
    const GraphicsCapabilityReport& report);

enum class MetricAvailability {
    measured,
    unavailable,
};

struct UInt64Metric {
    MetricAvailability availability{MetricAvailability::unavailable};
    std::uint64_t value{};
    std::string unit;
    std::string reason;

    bool operator==(const UInt64Metric&) const = default;
};

enum class MeasurementEvaluation {
    measured_not_evaluated,
};

struct FrameMeasurements {
    MeasurementEvaluation evaluation{MeasurementEvaluation::measured_not_evaluated};
    UInt64Metric total_frame_time_ns;
    UInt64Metric process_cpu_time_ns;
    UInt64Metric geometry_build_time_ns;
    UInt64Metric polish_sync_render_readback_time_ns;
    UInt64Metric uploaded_vertices;
    UInt64Metric uploaded_bytes;
    UInt64Metric process_working_set_bytes;
    UInt64Metric gpu_frame_time_ns;
    UInt64Metric gpu_memory_usage_bytes;

    bool operator==(const FrameMeasurements&) const = default;
};

struct OffscreenDiagnostic {
    std::uint32_t schema_version{offscreen_schema_version};
    std::string diagnostic_id;
    FrameFailureCode failure_code{FrameFailureCode::render_failed};
    GraphicsBackend backend{GraphicsBackend::qt_software};
    std::uint64_t device_generation{};
    bool retryable{false};
    std::string stage;
    std::string detail;

    bool operator==(const OffscreenDiagnostic&) const = default;
};

struct OffscreenSessionConfig {
    std::uint32_t schema_version{offscreen_schema_version};
    std::shared_ptr<const RenderSnapshot> snapshot;
    RenderSnapshotId expected_snapshot_id;
    core::TimelineRevision expected_timeline_revision{};
    core::TimeRange viewport_time_range;
    BackendPreference backend_preference{
        BackendPreference::default_gpu_with_software_fallback};
    std::size_t max_outstanding_frames{3};
    std::uint64_t max_outstanding_bytes{};
    std::uint64_t minimum_device_generation{1};
};

struct OffscreenRenderResult {
    FramePublishStatus publish_status{FramePublishStatus::invalid};
    std::uint64_t device_generation{};
    FrameIndex frame_index{};
    core::TimeNs frame_time_ns{};
    GeometryBuildStats geometry_stats;
    FrameMeasurements measurements;
};

class OffscreenRenderSession final {
public:
    ~OffscreenRenderSession();

    OffscreenRenderSession(const OffscreenRenderSession&) = delete;
    OffscreenRenderSession& operator=(const OffscreenRenderSession&) = delete;
    OffscreenRenderSession(OffscreenRenderSession&&) = delete;
    OffscreenRenderSession& operator=(OffscreenRenderSession&&) = delete;

    static core::Result<std::unique_ptr<OffscreenRenderSession>> create(
        OffscreenSessionConfig config);

    core::Result<OffscreenRenderResult> render_frame(
        FrameIndex frame_index,
        const core::CancellationToken* cancellation = nullptr);
    [[nodiscard]] FrameTakeResult try_take();
    void finish();
    void cancel();
    void notify_device_lost(std::string diagnostic_id,
                            std::string detail = {});
    core::Result<std::unique_ptr<OffscreenRenderSession>> rebuild() const;

    [[nodiscard]] GraphicsBackend backend() const noexcept;
    [[nodiscard]] std::uint64_t device_generation() const noexcept;
    [[nodiscard]] FrameQueueState queue_state() const;
    [[nodiscard]] const BackendSelection& backend_selection() const noexcept;
    [[nodiscard]] const std::optional<OffscreenDiagnostic>& last_diagnostic() const noexcept;

private:
    class Impl;
    explicit OffscreenRenderSession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

struct PixelDifferenceMeasurement {
    MeasurementEvaluation evaluation{MeasurementEvaluation::measured_not_evaluated};
    std::string reference_sha256;
    std::string candidate_sha256;
    bool exact_hash_match{false};
    std::uint8_t maximum_channel_difference{};
    std::uint64_t different_pixel_count{};
    std::uint64_t compared_pixel_count{};

    bool operator==(const PixelDifferenceMeasurement&) const = default;
};

core::Result<PixelDifferenceMeasurement> measure_pixel_difference(
    const RenderedFrame& reference,
    const RenderedFrame& candidate);

// Captures the ordinary QQuickWindow path with QQuickWindow::grabWindow(). It
// is intentionally separate from QQuickRenderControl so tests can measure the
// real on-screen adapter against redirected rendering.
core::Result<RenderedFrame> render_onscreen_reference(
    std::shared_ptr<const RenderSnapshot> snapshot,
    core::TimeRange viewport_time_range,
    FrameIndex frame_index,
    std::uint64_t device_generation,
    GraphicsBackend backend,
    const core::CancellationToken* cancellation = nullptr);

} // namespace space_rhythm::rendering
