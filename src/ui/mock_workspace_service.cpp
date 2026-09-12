#include <space_rhythm/ui/workspace_service.hpp>

#include <QObject>
#include <QTimer>

#include <algorithm>
#include <cstdint>
#include <utility>

namespace space_rhythm::ui {
namespace {

QString parameter_label(const QString& name)
{
    if (name == QStringLiteral("amplitude-ppm")) {
        return QStringLiteral("幅度");
    }
    if (name == QStringLiteral("line-width-milli-px")) {
        return QStringLiteral("线宽");
    }
    if (name == QStringLiteral("primary-rgba")) {
        return QStringLiteral("主色");
    }
    if (name == QStringLiteral("secondary-rgba")) {
        return QStringLiteral("辅色");
    }
    if (name == QStringLiteral("event-marker-width-milli-px")) {
        return QStringLiteral("事件标记宽度");
    }
    return name;
}

QString parameter_unit(const QString& name)
{
    if (name.endsWith(QStringLiteral("-ppm"))) {
        return QStringLiteral("ppm");
    }
    if (name.endsWith(QStringLiteral("-milli-px"))) {
        return QStringLiteral("milli-px");
    }
    if (name.endsWith(QStringLiteral("-rgba"))) {
        return QStringLiteral("RGBA");
    }
    return {};
}

QVector<TemplateParameterDto> waveform_parameters()
{
    QVector<TemplateParameterDto> result;
    const auto& definition = rendering::template_definition(
        rendering::VisualTemplate::waveform_oscilloscope);
    result.reserve(static_cast<qsizetype>(definition.integer_parameters.size()));
    for (const auto& parameter : definition.integer_parameters) {
        const auto name = QString::fromStdString(parameter.name);
        result.push_back({
            name,
            parameter_label(name),
            parameter_unit(name),
            parameter.minimum,
            parameter.maximum,
            parameter.default_value,
            parameter.default_value,
            parameter.name == "max-total-vertices" ||
                parameter.name == "max-vertices-per-batch" ||
                parameter.name == "lod-samples-per-pixel",
        });
    }
    return result;
}

QVector<AssetDto> sample_assets()
{
    return {
        {QStringLiteral("asset-video-01"),
         QStringLiteral("neon-run.mp4"),
         QStringLiteral("video"),
         QStringLiteral("ready"),
         QStringLiteral("02:41.000"),
         QStringLiteral("视频 + 音频 · VFR")},
        {QStringLiteral("asset-audio-01"),
         QStringLiteral("pulse-guide.wav"),
         QStringLiteral("audio"),
         QStringLiteral("ready"),
         QStringLiteral("02:41.000"),
         QStringLiteral("48 kHz · stereo")},
    };
}

TaskDto analysis_task(system::JobStatus status, core::NormPpm progress)
{
    return {
        QStringLiteral("mock-analysis-001"),
        QStringLiteral("音频分析"),
        QStringLiteral("pulse-guide.wav"),
        status == system::JobStatus::cancelling ? QStringLiteral("正在取消")
                                                : QStringLiteral("谱变化"),
        status,
        progress,
        status == system::JobStatus::failed,
        status == system::JobStatus::failed ? QStringLiteral("UI-MOCK-ANALYSIS-001")
                                             : QString{},
    };
}

UiErrorDto recoverable_error()
{
    return {
        QStringLiteral("media"),
        QStringLiteral("decode_failed"),
        QStringLiteral("decode_audio"),
        QStringLiteral("ui.error.analysis_failed"),
        QStringLiteral("UI-MOCK-ANALYSIS-001"),
        true,
        true,
    };
}

UiErrorDto fatal_error()
{
    return {
        QStringLiteral("compatibility"),
        QStringLiteral("unsupported_project_schema"),
        QStringLiteral("load_project"),
        QStringLiteral("ui.error.fatal_project_schema"),
        QStringLiteral("UI-MOCK-FATAL-001"),
        false,
        true,
    };
}

UiErrorDto recoverable_worker_error()
{
    return {
        QStringLiteral("internal"),
        QStringLiteral("worker_crashed"),
        QStringLiteral("worker.transport"),
        QStringLiteral("ui.worker.disconnected"),
        QStringLiteral("UI-MOCK-WORKER-001"),
        true,
        true,
    };
}

class MockWorkspaceService final : public QObject, public WorkspaceService {
public:
    explicit MockWorkspaceService(MockScenario scenario)
    {
        snapshot_.template_parameters = waveform_parameters();
        apply_scenario(scenario);
    }

    [[nodiscard]] UiBridgeDescriptor descriptor() const override
    {
        return {};
    }

    [[nodiscard]] WorkspaceSnapshotDto current_snapshot() const override
    {
        return snapshot_;
    }

    void set_observer(Observer observer) override
    {
        observer_ = std::move(observer);
    }

    void post(UiCommand command) override
    {
        ++transition_generation_;
        if (command.schema_version != bridge_schema_version ||
            command.contract_version != qt_string(bridge_contract_version)) {
            snapshot_.route = UiRoute::fatal;
            snapshot_.state = WorkspaceState::failed;
            snapshot_.error = fatal_error();
            publish();
            return;
        }

        switch (command.kind) {
        case UiCommandKind::create_project:
            begin_project_load(QStringLiteral("未命名节奏"), false, true);
            break;
        case UiCommandKind::open_project:
            begin_project_load(QStringLiteral("节奏实验 01"), true, false);
            break;
        case UiCommandKind::open_project_from:
            snapshot_.last_saved_path = command.argument;
            begin_project_load(QStringLiteral("节奏实验 01"), true, false);
            break;
        case UiCommandKind::return_to_start:
            snapshot_ = WorkspaceSnapshotDto{};
            snapshot_.template_parameters = waveform_parameters();
            publish();
            break;
        case UiCommandKind::activate_stage:
            if (snapshot_.route == UiRoute::workspace && !command.argument.isEmpty()) {
                snapshot_.active_stage = command.argument;
                publish();
            }
            break;
        case UiCommandKind::start_analysis:
            if (snapshot_.route == UiRoute::workspace && !snapshot_.read_only &&
                snapshot_.state == WorkspaceState::idle && !snapshot_.assets.isEmpty()) {
                snapshot_.active_stage = QStringLiteral("analyze");
                snapshot_.state = WorkspaceState::running;
                snapshot_.tasks = {analysis_task(system::JobStatus::running, 420'000)};
                snapshot_.error.reset();
                publish();
            }
            break;
        case UiCommandKind::import_asset:
            begin_mock_import();
            break;
        case UiCommandKind::cancel_active_task:
            request_cancel();
            break;
        case UiCommandKind::retry_failed_operation:
            if (snapshot_.state == WorkspaceState::failed) {
                snapshot_.state = WorkspaceState::running;
                snapshot_.tasks = {analysis_task(system::JobStatus::running, 0)};
                snapshot_.error.reset();
                publish();
            }
            break;
        case UiCommandKind::recover_autosave:
            begin_recovery(true);
            break;
        case UiCommandKind::open_primary:
            begin_recovery(false);
            break;
        case UiCommandKind::save_project:
            begin_save();
            break;
        case UiCommandKind::save_project_to:
            snapshot_.last_saved_path = command.argument;
            begin_save();
            break;
        case UiCommandKind::export_project:
            begin_export();
            break;
        case UiCommandKind::export_project_to:
            snapshot_.last_export_path = command.argument;
            begin_export();
            break;
        case UiCommandKind::toggle_preview:
            toggle_preview();
            break;
        case UiCommandKind::stop_preview:
            if (snapshot_.preview_state != PreviewState::stopped) {
                snapshot_.preview_state = PreviewState::stopped;
                publish();
            }
            break;
        case UiCommandKind::timeline_zoom:
        case UiCommandKind::timeline_pan:
            publish();
            break;
        case UiCommandKind::timeline_seek:
            snapshot_.preview_time_ns = 5'000'000'000;
            publish();
            break;
        case UiCommandKind::timeline_seek_begin:
            snapshot_.timeline_gesture_active = true;
            snapshot_.timeline_seek_pending = true;
            snapshot_.preview_state = PreviewState::seeking;
            publish();
            break;
        case UiCommandKind::timeline_seek_update:
            snapshot_.timeline_ghost_ratio = 0.5;
            publish();
            break;
        case UiCommandKind::timeline_seek_end:
            snapshot_.timeline_gesture_active = false;
            snapshot_.timeline_seek_pending = false;
            snapshot_.preview_time_ns = 5'000'000'000;
            snapshot_.preview_state = PreviewState::paused;
            publish();
            break;
        case UiCommandKind::timeline_select:
        case UiCommandKind::timeline_toggle_selection:
            snapshot_.selected_event_id = QStringLiteral("mock-event-1");
            snapshot_.selected_event_text = QStringLiteral("mock-event-1 · beat · 00:05.000");
            publish();
            break;
        case UiCommandKind::timeline_add_manual_event:
        case UiCommandKind::batch_offset_selected:
            apply_mock_edit();
            break;
        case UiCommandKind::timeline_drag_begin:
            snapshot_.timeline_gesture_active = true;
            publish();
            break;
        case UiCommandKind::timeline_drag_update:
            snapshot_.timeline_ghost_ratio = 0.5;
            publish();
            break;
        case UiCommandKind::timeline_drag_end:
            snapshot_.timeline_gesture_active = false;
            apply_mock_edit();
            break;
        case UiCommandKind::timeline_gesture_cancel:
            snapshot_.timeline_gesture_active = false;
            snapshot_.timeline_seek_pending = false;
            publish();
            break;
        case UiCommandKind::enable_development_audio_mapping:
            snapshot_.development_audio_mapping_enabled = true;
            snapshot_.audio_mapping_status_text =
                QStringLiteral("已显式启用 A-018 CC0 开发映射（非产品默认）");
            publish();
            break;
        case UiCommandKind::toggle_selected_event_lock:
            if (!snapshot_.selected_event_id.isEmpty() && editable()) {
                snapshot_.selected_event_locked = !snapshot_.selected_event_locked;
                apply_mock_edit();
            }
            break;
        case UiCommandKind::undo:
            if (editable() && snapshot_.can_undo) {
                snapshot_.can_undo = false;
                snapshot_.can_redo = true;
                ++snapshot_.timeline_revision;
                publish();
            }
            break;
        case UiCommandKind::redo:
            if (editable() && snapshot_.can_redo) {
                snapshot_.can_undo = true;
                snapshot_.can_redo = false;
                ++snapshot_.timeline_revision;
                publish();
            }
            break;
        case UiCommandKind::reconnect_worker:
            snapshot_.worker_connected = true;
            snapshot_.state = snapshot_.read_only ? WorkspaceState::read_only
                                                  : WorkspaceState::idle;
            snapshot_.error.reset();
            publish();
            break;
        case UiCommandKind::simulate_worker_disconnect:
            snapshot_.worker_connected = false;
            snapshot_.state = WorkspaceState::failed;
            snapshot_.error = recoverable_worker_error();
            publish();
            break;
        }
    }

private:
    bool editable() const noexcept
    {
        return snapshot_.route == UiRoute::workspace && !snapshot_.read_only &&
               snapshot_.state == WorkspaceState::idle;
    }

    void apply_mock_edit()
    {
        if (!editable()) {
            return;
        }
        ++snapshot_.timeline_revision;
        snapshot_.dirty = true;
        snapshot_.can_undo = true;
        snapshot_.can_redo = false;
        snapshot_.selected_event_id = QStringLiteral("mock-event-1");
        snapshot_.selected_event_text = QStringLiteral("mock-event-1 · manual · 00:05.000");
        publish();
    }

    void begin_mock_import()
    {
        if (!editable()) {
            return;
        }
        snapshot_.state = WorkspaceState::loading;
        snapshot_.active_stage = QStringLiteral("import");
        publish();
        const auto generation = transition_generation_;
        QTimer::singleShot(0, this, [this, generation]() {
            if (generation != transition_generation_) {
                return;
            }
            snapshot_.assets = sample_assets();
            snapshot_.active_stage = QStringLiteral("analyze");
            snapshot_.state = WorkspaceState::idle;
            publish();
        });
    }

    void apply_scenario(MockScenario scenario)
    {
        snapshot_ = WorkspaceSnapshotDto{};
        snapshot_.template_parameters = waveform_parameters();
        if (scenario == MockScenario::start) {
            return;
        }

        snapshot_.route = UiRoute::workspace;
        snapshot_.project_id = QStringLiteral("project-mock-001");
        snapshot_.project_title = QStringLiteral("节奏实验 01");
        snapshot_.timeline_revision = 42;
        snapshot_.preview_time_ns = 12'340'000'000;
        snapshot_.frame_index = 370;
        snapshot_.assets = sample_assets();

        switch (scenario) {
        case MockScenario::start:
        case MockScenario::idle:
            break;
        case MockScenario::loading:
            snapshot_.state = WorkspaceState::loading;
            break;
        case MockScenario::running:
            snapshot_.state = WorkspaceState::running;
            snapshot_.active_stage = QStringLiteral("analyze");
            snapshot_.tasks = {analysis_task(system::JobStatus::running, 420'000)};
            break;
        case MockScenario::cancelling:
            snapshot_.state = WorkspaceState::cancelling;
            snapshot_.active_stage = QStringLiteral("analyze");
            snapshot_.tasks = {analysis_task(system::JobStatus::cancelling, 420'000)};
            break;
        case MockScenario::failed:
            snapshot_.state = WorkspaceState::failed;
            snapshot_.tasks = {analysis_task(system::JobStatus::failed, 420'000)};
            snapshot_.error = recoverable_error();
            break;
        case MockScenario::recovery:
            snapshot_.route = UiRoute::start;
            snapshot_.state = WorkspaceState::recovery;
            snapshot_.recovery_available = true;
            snapshot_.dirty = true;
            break;
        case MockScenario::read_only:
            snapshot_.state = WorkspaceState::read_only;
            snapshot_.read_only = true;
            break;
        case MockScenario::fatal:
            snapshot_.route = UiRoute::fatal;
            snapshot_.state = WorkspaceState::failed;
            snapshot_.error = fatal_error();
            break;
        }
    }

    void begin_project_load(QString title, bool include_assets, bool dirty)
    {
        snapshot_.route = UiRoute::workspace;
        snapshot_.state = WorkspaceState::loading;
        snapshot_.project_id = QStringLiteral("project-mock-001");
        snapshot_.project_title = std::move(title);
        snapshot_.dirty = dirty;
        snapshot_.read_only = false;
        snapshot_.recovery_available = false;
        snapshot_.timeline_revision = include_assets ? 42U : 0U;
        snapshot_.preview_time_ns = include_assets ? 12'340'000'000 : 0;
        snapshot_.frame_index = include_assets ? 370U : 0U;
        snapshot_.assets.clear();
        snapshot_.tasks.clear();
        snapshot_.error.reset();
        publish();

        const auto generation = transition_generation_;
        QTimer::singleShot(0, this, [this, generation, include_assets]() {
            if (generation != transition_generation_) {
                return;
            }
            snapshot_.state = WorkspaceState::idle;
            snapshot_.assets = include_assets ? sample_assets() : QVector<AssetDto>{};
            publish();
        });
    }

    void request_cancel()
    {
        if (snapshot_.state != WorkspaceState::running || snapshot_.tasks.isEmpty()) {
            return;
        }
        snapshot_.state = WorkspaceState::cancelling;
        snapshot_.tasks.front().status = system::JobStatus::cancelling;
        snapshot_.tasks.front().stage = QStringLiteral("正在取消");
        publish();

        const auto generation = transition_generation_;
        QTimer::singleShot(0, this, [this, generation]() {
            if (generation != transition_generation_ || snapshot_.tasks.isEmpty()) {
                return;
            }
            snapshot_.tasks.front().status = system::JobStatus::cancelled;
            snapshot_.tasks.front().stage = QStringLiteral("已取消");
            snapshot_.state = snapshot_.read_only ? WorkspaceState::read_only
                                                  : WorkspaceState::idle;
            publish();
        });
    }

    void begin_recovery(bool autosave)
    {
        if (!snapshot_.recovery_available) {
            return;
        }
        snapshot_.route = UiRoute::workspace;
        snapshot_.state = WorkspaceState::loading;
        snapshot_.recovery_available = false;
        snapshot_.project_title = QStringLiteral("节奏实验（已恢复）");
        snapshot_.error.reset();
        publish();

        const auto generation = transition_generation_;
        QTimer::singleShot(0, this, [this, generation, autosave]() {
            if (generation != transition_generation_) {
                return;
            }
            snapshot_.state = WorkspaceState::idle;
            snapshot_.dirty = autosave;
            snapshot_.assets = sample_assets();
            publish();
        });
    }

    void begin_save()
    {
        if (snapshot_.route != UiRoute::workspace || snapshot_.read_only || !snapshot_.dirty ||
            snapshot_.state == WorkspaceState::loading ||
            snapshot_.state == WorkspaceState::cancelling) {
            return;
        }
        snapshot_.state = WorkspaceState::loading;
        publish();
        const auto generation = transition_generation_;
        QTimer::singleShot(0, this, [this, generation]() {
            if (generation != transition_generation_) {
                return;
            }
            snapshot_.state = WorkspaceState::idle;
            snapshot_.dirty = false;
            publish();
        });
    }

    void begin_export()
    {
        const auto stable_state = snapshot_.state == WorkspaceState::idle ||
                                  snapshot_.state == WorkspaceState::read_only;
        if (snapshot_.route != UiRoute::workspace || snapshot_.assets.isEmpty() ||
            !stable_state) {
            return;
        }
        snapshot_.active_stage = QStringLiteral("export");
        snapshot_.state = WorkspaceState::running;
        snapshot_.tasks = {{
            QStringLiteral("mock-export-001"),
            QStringLiteral("导出"),
            snapshot_.project_title,
            QStringLiteral("格式待确认"),
            system::JobStatus::running,
            0,
            false,
            {},
        }};
        publish();
    }

    void toggle_preview()
    {
        if (snapshot_.route != UiRoute::workspace || snapshot_.assets.isEmpty()) {
            return;
        }
        if (snapshot_.preview_state == PreviewState::playing) {
            snapshot_.preview_state = PreviewState::paused;
        } else {
            snapshot_.preview_state = PreviewState::playing;
        }
        snapshot_.active_stage = QStringLiteral("preview");
        publish();
    }

    void publish()
    {
        if (observer_) {
            observer_(snapshot_);
        }
    }

    WorkspaceSnapshotDto snapshot_;
    Observer observer_;
    std::uint64_t transition_generation_{};
};

} // namespace

MockScenario parse_mock_scenario(QString value) noexcept
{
    value = value.trimmed().toLower();
    if (value == QStringLiteral("idle")) {
        return MockScenario::idle;
    }
    if (value == QStringLiteral("loading")) {
        return MockScenario::loading;
    }
    if (value == QStringLiteral("running")) {
        return MockScenario::running;
    }
    if (value == QStringLiteral("cancelling")) {
        return MockScenario::cancelling;
    }
    if (value == QStringLiteral("failed")) {
        return MockScenario::failed;
    }
    if (value == QStringLiteral("recovery")) {
        return MockScenario::recovery;
    }
    if (value == QStringLiteral("readonly") || value == QStringLiteral("read-only")) {
        return MockScenario::read_only;
    }
    if (value == QStringLiteral("fatal")) {
        return MockScenario::fatal;
    }
    return MockScenario::start;
}

std::unique_ptr<WorkspaceService> make_mock_workspace_service(MockScenario scenario)
{
    return std::make_unique<MockWorkspaceService>(scenario);
}

} // namespace space_rhythm::ui
