#pragma once

#include <space_rhythm/core/timeline.hpp>
#include <space_rhythm/audio/analysis.hpp>
#include <space_rhythm/audio/render.hpp>
#include <space_rhythm/media/playback_export.hpp>
#include <space_rhythm/rendering/geometry_core.hpp>
#include <space_rhythm/system/runtime.hpp>

#include <QString>
#include <QVector>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace space_rhythm::ui {

inline constexpr std::uint32_t bridge_schema_version = 3;
inline constexpr std::string_view bridge_contract_version{"0.3.0"};

inline QString qt_string(std::string_view value)
{
    return QString::fromLatin1(value.data(), static_cast<qsizetype>(value.size()));
}

enum class UiRoute {
    start,
    workspace,
    fatal,
};

enum class WorkspaceState {
    idle,
    loading,
    running,
    cancelling,
    failed,
    recovery,
    read_only,
};

enum class UiCommandKind {
    create_project,
    open_project,
    return_to_start,
    activate_stage,
    start_analysis,
    cancel_active_task,
    retry_failed_operation,
    recover_autosave,
    open_primary,
    save_project,
    export_project,
    toggle_preview,
    stop_preview,
    open_project_from,
    import_asset,
    save_project_to,
    export_project_to,
    timeline_zoom,
    timeline_pan,
    timeline_seek,
    timeline_select,
    timeline_add_manual_event,
    timeline_toggle_selection,
    timeline_drag_begin,
    timeline_drag_update,
    timeline_drag_end,
    timeline_gesture_cancel,
    timeline_seek_begin,
    timeline_seek_update,
    timeline_seek_end,
    enable_development_audio_mapping,
    toggle_selected_event_lock,
    batch_offset_selected,
    undo,
    redo,
    reconnect_worker,
    simulate_worker_disconnect,
};

enum class PreviewState {
    stopped,
    priming,
    playing,
    paused,
    seeking,
    error,
};

enum class MockScenario {
    start,
    idle,
    loading,
    running,
    cancelling,
    failed,
    recovery,
    read_only,
    fatal,
};

struct UiBridgeDescriptor {
    std::uint32_t schema_version{bridge_schema_version};
    QString contract_version{qt_string(bridge_contract_version)};
    QString core_contract_version{qt_string(core::contract_version)};
    std::uint32_t system_ipc_schema_version{system::ipc_schema_version};
    std::uint32_t project_schema_version{system::project_schema_version};
    QString rendering_contract_version{qt_string(rendering::contract_version)};
    QString media_contract_version{qt_string(media::contract_version)};
    QString audio_analysis_contract_version{qt_string(audio::contract_version)};
    QString audio_render_contract_version{qt_string(audio::render::contract_version)};
    QString playback_export_contract_version{
        qt_string(media::playback_export::contract_version)};

    bool operator==(const UiBridgeDescriptor&) const = default;
};

struct UiErrorDto {
    QString category;
    QString code;
    QString stage;
    QString message_key;
    QString diagnostic_id;
    bool retryable{false};
    bool project_safe{true};

    bool operator==(const UiErrorDto&) const = default;
};

struct AssetDto {
    QString asset_id;
    QString display_name;
    QString kind;
    QString state;
    QString duration_text;
    QString detail_text;

    bool operator==(const AssetDto&) const = default;
};

struct TaskDto {
    QString request_id;
    QString operation;
    QString subject;
    QString stage;
    system::JobStatus status{system::JobStatus::queued};
    core::NormPpm progress_ppm{};
    bool retryable{false};
    QString diagnostic_id;

    bool operator==(const TaskDto&) const = default;
};

struct TemplateParameterDto {
    QString name;
    QString label;
    QString unit;
    std::int64_t minimum{};
    std::int64_t maximum{};
    std::int64_t engineering_default{};
    std::int64_t value{};
    bool advanced{false};

    bool operator==(const TemplateParameterDto&) const = default;
};

struct WorkspaceSnapshotDto {
    std::uint32_t schema_version{bridge_schema_version};
    QString contract_version{qt_string(bridge_contract_version)};
    UiRoute route{UiRoute::start};
    WorkspaceState state{WorkspaceState::idle};
    QString project_id;
    QString project_title;
    QString active_stage{"import"};
    bool dirty{false};
    bool read_only{false};
    bool recovery_available{false};
    core::TimelineRevision timeline_revision{};
    core::TimeNs preview_time_ns{};
    rendering::FrameIndex frame_index{};
    PreviewState preview_state{PreviewState::stopped};
    core::TimeRange timeline_range{0, 10'000'000'000};
    core::TimeRange viewport_range{0, 10'000'000'000};
    std::shared_ptr<const rendering::RenderSnapshot> render_snapshot;
    QString selected_event_id;
    QString selected_event_text;
    bool selected_event_locked{false};
    bool can_undo{false};
    bool can_redo{false};
    bool worker_connected{true};
    bool timeline_gesture_active{false};
    bool timeline_seek_pending{false};
    double timeline_ghost_ratio{};
    QString interaction_status_text;
    bool development_audio_mapping_enabled{false};
    QString audio_mapping_status_text;
    QString last_saved_path;
    QString last_export_path;
    QVector<AssetDto> assets;
    QVector<TaskDto> tasks;
    QVector<TemplateParameterDto> template_parameters;
    std::optional<UiErrorDto> error;

    bool operator==(const WorkspaceSnapshotDto&) const = default;
};

struct UiCommand {
    std::uint32_t schema_version{bridge_schema_version};
    QString contract_version{qt_string(bridge_contract_version)};
    QString request_id;
    UiCommandKind kind{UiCommandKind::return_to_start};
    QString argument;

    bool operator==(const UiCommand&) const = default;
};

// Production adapters and the deterministic mock both implement this contract.
// post() must return without waiting for media, worker, disk or rendering work;
// later immutable snapshots are delivered through Observer.
class WorkspaceService {
public:
    using Observer = std::function<void(WorkspaceSnapshotDto)>;

    virtual ~WorkspaceService() = default;
    [[nodiscard]] virtual UiBridgeDescriptor descriptor() const = 0;
    [[nodiscard]] virtual WorkspaceSnapshotDto current_snapshot() const = 0;
    virtual void set_observer(Observer observer) = 0;
    virtual void post(UiCommand command) = 0;
};

[[nodiscard]] MockScenario parse_mock_scenario(QString value) noexcept;
[[nodiscard]] std::unique_ptr<WorkspaceService> make_mock_workspace_service(
    MockScenario scenario = MockScenario::start);

struct IntegratedWorkspaceOptions {
    QString project_path;
    QString import_path;
    QString export_path;
    bool headless{false};
    QString worker_executable_path;
    std::uint32_t worker_test_delay_ms{};
    std::uint32_t project_io_test_delay_ms{};
};

// Real core/media/audio/rendering adapter. The options are primarily useful to
// deterministic headless integration tests; ordinary UI paths arrive through
// versioned UiCommand values.
[[nodiscard]] std::unique_ptr<WorkspaceService> make_integrated_workspace_service(
    IntegratedWorkspaceOptions options = {});

} // namespace space_rhythm::ui
