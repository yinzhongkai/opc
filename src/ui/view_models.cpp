#include <space_rhythm/ui/view_models.hpp>

#include <QMetaObject>
#include <QThread>
#include <QVariant>

#include <algorithm>
#include <cstdint>
#include <utility>

namespace space_rhythm::ui {
namespace {

QString route_name(UiRoute route)
{
    switch (route) {
    case UiRoute::start:
        return QStringLiteral("start");
    case UiRoute::workspace:
        return QStringLiteral("workspace");
    case UiRoute::fatal:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("fatal");
}

QString state_name(WorkspaceState state)
{
    switch (state) {
    case WorkspaceState::idle:
        return QStringLiteral("idle");
    case WorkspaceState::loading:
        return QStringLiteral("loading");
    case WorkspaceState::running:
        return QStringLiteral("running");
    case WorkspaceState::cancelling:
        return QStringLiteral("cancelling");
    case WorkspaceState::failed:
        return QStringLiteral("failed");
    case WorkspaceState::recovery:
        return QStringLiteral("recovery");
    case WorkspaceState::read_only:
        return QStringLiteral("readOnly");
    }
    return QStringLiteral("failed");
}

QString job_status_name(system::JobStatus status)
{
    switch (status) {
    case system::JobStatus::queued:
        return QStringLiteral("queued");
    case system::JobStatus::running:
        return QStringLiteral("running");
    case system::JobStatus::cancelling:
        return QStringLiteral("cancelling");
    case system::JobStatus::succeeded:
        return QStringLiteral("succeeded");
    case system::JobStatus::failed:
        return QStringLiteral("failed");
    case system::JobStatus::cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("failed");
}

QString preview_state_name(PreviewState state)
{
    switch (state) {
    case PreviewState::stopped:
        return QStringLiteral("stopped");
    case PreviewState::priming:
        return QStringLiteral("priming");
    case PreviewState::playing:
        return QStringLiteral("playing");
    case PreviewState::paused:
        return QStringLiteral("paused");
    case PreviewState::error:
        return QStringLiteral("error");
    }
    return QStringLiteral("error");
}

QString format_time_ns(core::TimeNs time_ns)
{
    const auto non_negative = std::max<core::TimeNs>(0, time_ns);
    const auto total_ms = static_cast<std::uint64_t>(non_negative / 1'000'000);
    const auto milliseconds = total_ms % 1'000U;
    const auto total_seconds = total_ms / 1'000U;
    const auto seconds = total_seconds % 60U;
    const auto total_minutes = total_seconds / 60U;
    const auto minutes = total_minutes % 60U;
    const auto hours = total_minutes / 60U;
    if (hours > 0U) {
        return QStringLiteral("%1:%2:%3.%4")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(milliseconds, 3, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2.%3")
        .arg(total_minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(milliseconds, 3, 10, QLatin1Char('0'));
}

bool valid_index(const QModelIndex& index, qsizetype size)
{
    return index.isValid() && !index.parent().isValid() && index.row() >= 0 &&
           index.row() < size;
}

} // namespace

AssetListModel::AssetListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AssetListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(assets_.size());
}

QVariant AssetListModel::data(const QModelIndex& index, int role) const
{
    if (!valid_index(index, assets_.size())) {
        return {};
    }
    const auto& asset = assets_.at(index.row());
    switch (role) {
    case AssetIdRole:
        return asset.asset_id;
    case DisplayNameRole:
        return asset.display_name;
    case KindRole:
        return asset.kind;
    case StateRole:
        return asset.state;
    case DurationTextRole:
        return asset.duration_text;
    case DetailTextRole:
        return asset.detail_text;
    default:
        return {};
    }
}

QHash<int, QByteArray> AssetListModel::roleNames() const
{
    return {
        {AssetIdRole, "assetId"},
        {DisplayNameRole, "displayName"},
        {KindRole, "kind"},
        {StateRole, "assetState"},
        {DurationTextRole, "durationText"},
        {DetailTextRole, "detailText"},
    };
}

void AssetListModel::replace(QVector<AssetDto> assets)
{
    beginResetModel();
    assets_ = std::move(assets);
    endResetModel();
}

TaskListModel::TaskListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TaskListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(tasks_.size());
}

QVariant TaskListModel::data(const QModelIndex& index, int role) const
{
    if (!valid_index(index, tasks_.size())) {
        return {};
    }
    const auto& task = tasks_.at(index.row());
    switch (role) {
    case RequestIdRole:
        return task.request_id;
    case OperationRole:
        return task.operation;
    case SubjectRole:
        return task.subject;
    case StageRole:
        return task.stage;
    case StatusRole:
        return job_status_name(task.status);
    case ProgressRole:
        return static_cast<double>(task.progress_ppm) /
               static_cast<double>(core::norm_ppm_max);
    case ProgressPpmRole:
        return task.progress_ppm;
    case RetryableRole:
        return task.retryable;
    case DiagnosticIdRole:
        return task.diagnostic_id;
    default:
        return {};
    }
}

QHash<int, QByteArray> TaskListModel::roleNames() const
{
    return {
        {RequestIdRole, "requestId"},
        {OperationRole, "operation"},
        {SubjectRole, "subject"},
        {StageRole, "stage"},
        {StatusRole, "status"},
        {ProgressRole, "progress"},
        {ProgressPpmRole, "progressPpm"},
        {RetryableRole, "retryable"},
        {DiagnosticIdRole, "diagnosticId"},
    };
}

void TaskListModel::replace(QVector<TaskDto> tasks)
{
    beginResetModel();
    tasks_ = std::move(tasks);
    endResetModel();
}

TemplateParameterListModel::TemplateParameterListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TemplateParameterListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(parameters_.size());
}

QVariant TemplateParameterListModel::data(const QModelIndex& index, int role) const
{
    if (!valid_index(index, parameters_.size())) {
        return {};
    }
    const auto& parameter = parameters_.at(index.row());
    switch (role) {
    case NameRole:
        return parameter.name;
    case LabelRole:
        return parameter.label;
    case UnitRole:
        return parameter.unit;
    case MinimumTextRole:
        return QString::number(parameter.minimum);
    case MaximumTextRole:
        return QString::number(parameter.maximum);
    case EngineeringDefaultTextRole:
        return QString::number(parameter.engineering_default);
    case ValueTextRole:
        return QString::number(parameter.value);
    case AdvancedRole:
        return parameter.advanced;
    default:
        return {};
    }
}

QHash<int, QByteArray> TemplateParameterListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {LabelRole, "label"},
        {UnitRole, "unit"},
        {MinimumTextRole, "minimumText"},
        {MaximumTextRole, "maximumText"},
        {EngineeringDefaultTextRole, "engineeringDefaultText"},
        {ValueTextRole, "valueText"},
        {AdvancedRole, "advanced"},
    };
}

void TemplateParameterListModel::replace(QVector<TemplateParameterDto> parameters)
{
    beginResetModel();
    parameters_ = std::move(parameters);
    endResetModel();
}

ApplicationViewModel::ApplicationViewModel(std::unique_ptr<WorkspaceService> service,
                                           QObject* parent)
    : QObject(parent)
    , service_(std::move(service))
    , assets_(this)
    , tasks_(this)
    , template_parameters_(this)
{
    Q_ASSERT(service_ != nullptr);
    snapshot_ = service_->current_snapshot();
    assets_.replace(snapshot_.assets);
    tasks_.replace(snapshot_.tasks);
    template_parameters_.replace(snapshot_.template_parameters);
    service_->set_observer(
        [this](WorkspaceSnapshotDto snapshot) { receive_snapshot(std::move(snapshot)); });
}

ApplicationViewModel::~ApplicationViewModel()
{
    if (service_) {
        service_->set_observer({});
    }
}

QString ApplicationViewModel::bridgeContractVersion() const
{
    return service_->descriptor().contract_version;
}

int ApplicationViewModel::bridgeSchemaVersion() const noexcept
{
    return static_cast<int>(service_->descriptor().schema_version);
}

QString ApplicationViewModel::route() const
{
    return route_name(snapshot_.route);
}

QString ApplicationViewModel::workspaceState() const
{
    return state_name(snapshot_.state);
}

QString ApplicationViewModel::projectTitle() const
{
    return snapshot_.project_title;
}

QString ApplicationViewModel::windowTitle() const
{
    const auto base = snapshot_.project_title.isEmpty() ? QStringLiteral("Space Rhythm")
                                                        : snapshot_.project_title +
                                                              QStringLiteral(" — Space Rhythm");
    return snapshot_.dirty ? base + QStringLiteral(" *") : base;
}

QString ApplicationViewModel::activeStage() const
{
    return snapshot_.active_stage;
}

QString ApplicationViewModel::timelineRevisionText() const
{
    return QStringLiteral("r%1").arg(snapshot_.timeline_revision);
}

QString ApplicationViewModel::previewTimeText() const
{
    return format_time_ns(snapshot_.preview_time_ns);
}

QString ApplicationViewModel::previewState() const
{
    return preview_state_name(snapshot_.preview_state);
}

QString ApplicationViewModel::statusMessage() const
{
    switch (snapshot_.state) {
    case WorkspaceState::idle:
        return snapshot_.route == UiRoute::start ? QStringLiteral("请选择新建或打开项目")
                                                 : QStringLiteral("就绪");
    case WorkspaceState::loading:
        return QStringLiteral("正在加载项目");
    case WorkspaceState::running:
        return QStringLiteral("任务正在后台运行，可继续浏览工作区");
    case WorkspaceState::cancelling:
        return QStringLiteral("正在取消任务，等待 worker 确认");
    case WorkspaceState::failed:
        return QStringLiteral("操作失败，项目与原素材安全");
    case WorkspaceState::recovery:
        return QStringLiteral("检测到较新的自动保存，可选择恢复");
    case WorkspaceState::read_only:
        return QStringLiteral("项目以只读方式打开，可试听或导出当前快照");
    }
    return QStringLiteral("未知状态");
}

QString ApplicationViewModel::statusTone() const
{
    switch (snapshot_.state) {
    case WorkspaceState::failed:
        return QStringLiteral("error");
    case WorkspaceState::recovery:
    case WorkspaceState::cancelling:
    case WorkspaceState::read_only:
        return QStringLiteral("warning");
    case WorkspaceState::loading:
    case WorkspaceState::running:
        return QStringLiteral("info");
    case WorkspaceState::idle:
        return QStringLiteral("neutral");
    }
    return QStringLiteral("error");
}

QString ApplicationViewModel::errorStage() const
{
    return snapshot_.error ? snapshot_.error->stage : QString{};
}

QString ApplicationViewModel::errorDiagnosticId() const
{
    return snapshot_.error ? snapshot_.error->diagnostic_id : QString{};
}

bool ApplicationViewModel::projectSafe() const noexcept
{
    return !snapshot_.error || snapshot_.error->project_safe;
}

bool ApplicationViewModel::dirty() const noexcept
{
    return snapshot_.dirty;
}

bool ApplicationViewModel::readOnly() const noexcept
{
    return snapshot_.read_only;
}

bool ApplicationViewModel::recoveryAvailable() const noexcept
{
    return snapshot_.recovery_available;
}

bool ApplicationViewModel::canAnalyze() const noexcept
{
    return snapshot_.route == UiRoute::workspace && snapshot_.state == WorkspaceState::idle &&
           !snapshot_.read_only && !snapshot_.assets.isEmpty();
}

bool ApplicationViewModel::canCancel() const noexcept
{
    return snapshot_.state == WorkspaceState::running;
}

bool ApplicationViewModel::canSave() const noexcept
{
    return snapshot_.route == UiRoute::workspace && snapshot_.dirty && !snapshot_.read_only &&
           snapshot_.state != WorkspaceState::loading &&
           snapshot_.state != WorkspaceState::cancelling;
}

bool ApplicationViewModel::canExport() const noexcept
{
    const auto stable_state = snapshot_.state == WorkspaceState::idle ||
                              snapshot_.state == WorkspaceState::read_only;
    return snapshot_.route == UiRoute::workspace && stable_state &&
           !snapshot_.assets.isEmpty();
}

bool ApplicationViewModel::canPreview() const noexcept
{
    return snapshot_.route == UiRoute::workspace && !snapshot_.assets.isEmpty() &&
           snapshot_.state != WorkspaceState::loading &&
           snapshot_.state != WorkspaceState::cancelling;
}

bool ApplicationViewModel::canRecover() const noexcept
{
    return snapshot_.recovery_available && snapshot_.state == WorkspaceState::recovery;
}

bool ApplicationViewModel::canUndo() const noexcept
{
    return canEditTimeline() && snapshot_.can_undo;
}

bool ApplicationViewModel::canRedo() const noexcept
{
    return canEditTimeline() && snapshot_.can_redo;
}

bool ApplicationViewModel::canEditTimeline() const noexcept
{
    return snapshot_.route == UiRoute::workspace && !snapshot_.read_only &&
           snapshot_.state == WorkspaceState::idle;
}

bool ApplicationViewModel::workerConnected() const noexcept
{
    return snapshot_.worker_connected;
}

QString ApplicationViewModel::selectedEventText() const
{
    return snapshot_.selected_event_text;
}

bool ApplicationViewModel::selectedEventLocked() const noexcept
{
    return snapshot_.selected_event_locked;
}

QString ApplicationViewModel::viewportText() const
{
    return QStringLiteral("%1 – %2")
        .arg(format_time_ns(snapshot_.viewport_range.start_ns),
             format_time_ns(snapshot_.viewport_range.end_ns));
}

QString ApplicationViewModel::lastSavedPath() const
{
    return snapshot_.last_saved_path;
}

QString ApplicationViewModel::lastExportPath() const
{
    return snapshot_.last_export_path;
}

QAbstractItemModel* ApplicationViewModel::assets() noexcept
{
    return &assets_;
}

QAbstractItemModel* ApplicationViewModel::tasks() noexcept
{
    return &tasks_;
}

QAbstractItemModel* ApplicationViewModel::templateParameters() noexcept
{
    return &template_parameters_;
}

void ApplicationViewModel::createProject()
{
    dispatch(UiCommandKind::create_project);
}

void ApplicationViewModel::openProject()
{
    dispatch(UiCommandKind::open_project);
}

void ApplicationViewModel::returnToStart()
{
    dispatch(UiCommandKind::return_to_start);
}

void ApplicationViewModel::activateStage(const QString& stage)
{
    dispatch(UiCommandKind::activate_stage, stage);
}

void ApplicationViewModel::startAnalysis()
{
    if (canAnalyze()) {
        dispatch(UiCommandKind::start_analysis);
    }
}

void ApplicationViewModel::cancelActiveTask()
{
    if (canCancel()) {
        dispatch(UiCommandKind::cancel_active_task);
    }
}

void ApplicationViewModel::retryFailedOperation()
{
    dispatch(UiCommandKind::retry_failed_operation);
}

void ApplicationViewModel::recoverAutosave()
{
    if (canRecover()) {
        dispatch(UiCommandKind::recover_autosave);
    }
}

void ApplicationViewModel::openPrimary()
{
    if (canRecover()) {
        dispatch(UiCommandKind::open_primary);
    }
}

void ApplicationViewModel::saveProject()
{
    if (canSave()) {
        dispatch(UiCommandKind::save_project);
    }
}

void ApplicationViewModel::exportProject()
{
    if (canExport()) {
        dispatch(UiCommandKind::export_project);
    }
}

void ApplicationViewModel::togglePreview()
{
    if (canPreview()) {
        dispatch(UiCommandKind::toggle_preview);
    }
}

void ApplicationViewModel::stopPreview()
{
    if (snapshot_.preview_state != PreviewState::stopped) {
        dispatch(UiCommandKind::stop_preview);
    }
}

void ApplicationViewModel::openProjectFrom(const QString& path)
{
    if (!path.isEmpty()) {
        dispatch(UiCommandKind::open_project_from, path);
    }
}

void ApplicationViewModel::importAsset(const QString& path)
{
    if (!path.isEmpty() && !snapshot_.read_only) {
        dispatch(UiCommandKind::import_asset, path);
    }
}

void ApplicationViewModel::saveProjectTo(const QString& path)
{
    if (!path.isEmpty() && !snapshot_.read_only) {
        dispatch(UiCommandKind::save_project_to, path);
    }
}

void ApplicationViewModel::exportProjectTo(const QString& path)
{
    if (!path.isEmpty() && canExport()) {
        dispatch(UiCommandKind::export_project_to, path);
    }
}

namespace {

QString pointer_argument(double x_pixels, double width_pixels)
{
    return QStringLiteral("%1|%2")
        .arg(QString::number(x_pixels, 'g', 17),
             QString::number(width_pixels, 'g', 17));
}

} // namespace

void ApplicationViewModel::zoomTimeline(int steps)
{
    dispatch(UiCommandKind::timeline_zoom, QString::number(steps));
}

void ApplicationViewModel::panTimeline(double delta_pixels, double width_pixels)
{
    dispatch(UiCommandKind::timeline_pan, pointer_argument(delta_pixels, width_pixels));
}

void ApplicationViewModel::seekTimeline(double x_pixels, double width_pixels)
{
    dispatch(UiCommandKind::timeline_seek, pointer_argument(x_pixels, width_pixels));
}

void ApplicationViewModel::selectTimelineEvent(double x_pixels, double width_pixels)
{
    dispatch(UiCommandKind::timeline_select, pointer_argument(x_pixels, width_pixels));
}

void ApplicationViewModel::addTimelineEvent(double x_pixels, double width_pixels)
{
    if (canEditTimeline()) {
        dispatch(UiCommandKind::timeline_add_manual_event,
                 pointer_argument(x_pixels, width_pixels));
    }
}

void ApplicationViewModel::toggleTimelineEventSelection(double x_pixels,
                                                        double width_pixels)
{
    dispatch(UiCommandKind::timeline_toggle_selection,
             pointer_argument(x_pixels, width_pixels));
}

void ApplicationViewModel::beginTimelineDrag(double x_pixels, double width_pixels)
{
    if (canEditTimeline()) {
        dispatch(UiCommandKind::timeline_drag_begin,
                 pointer_argument(x_pixels, width_pixels));
    }
}

void ApplicationViewModel::updateTimelineDrag(double x_pixels, double width_pixels)
{
    if (canEditTimeline()) {
        dispatch(UiCommandKind::timeline_drag_update,
                 pointer_argument(x_pixels, width_pixels));
    }
}

void ApplicationViewModel::endTimelineDrag()
{
    dispatch(UiCommandKind::timeline_drag_end);
}

void ApplicationViewModel::toggleSelectedEventLock()
{
    if (canEditTimeline() && !snapshot_.selected_event_id.isEmpty()) {
        dispatch(UiCommandKind::toggle_selected_event_lock);
    }
}

void ApplicationViewModel::batchOffsetSelected(const QString& milliseconds)
{
    if (canEditTimeline() && !snapshot_.selected_event_id.isEmpty()) {
        dispatch(UiCommandKind::batch_offset_selected, milliseconds);
    }
}

void ApplicationViewModel::undo()
{
    if (canUndo()) {
        dispatch(UiCommandKind::undo);
    }
}

void ApplicationViewModel::redo()
{
    if (canRedo()) {
        dispatch(UiCommandKind::redo);
    }
}

void ApplicationViewModel::reconnectWorker()
{
    if (!snapshot_.worker_connected) {
        dispatch(UiCommandKind::reconnect_worker);
    }
}

std::shared_ptr<const rendering::RenderSnapshot> ApplicationViewModel::renderSnapshot() const
{
    return snapshot_.render_snapshot;
}

core::TimeRange ApplicationViewModel::viewportTimeRange() const noexcept
{
    return snapshot_.viewport_range;
}

rendering::FrameIndex ApplicationViewModel::authoritativeFrameIndex() const noexcept
{
    return snapshot_.frame_index;
}

void ApplicationViewModel::dispatch(UiCommandKind kind, QString argument)
{
    UiCommand command;
    command.request_id = QStringLiteral("ui-%1").arg(next_request_id_++);
    command.kind = kind;
    command.argument = std::move(argument);
    service_->post(std::move(command));
}

void ApplicationViewModel::receive_snapshot(WorkspaceSnapshotDto snapshot)
{
    if (QThread::currentThread() == thread()) {
        apply_snapshot(std::move(snapshot));
        return;
    }
    QMetaObject::invokeMethod(
        this,
        [this, snapshot = std::move(snapshot)]() mutable {
            apply_snapshot(std::move(snapshot));
        },
        Qt::QueuedConnection);
}

void ApplicationViewModel::apply_snapshot(WorkspaceSnapshotDto snapshot)
{
    if (snapshot.schema_version != bridge_schema_version ||
        snapshot.contract_version != qt_string(bridge_contract_version)) {
        return;
    }
    snapshot_ = std::move(snapshot);
    assets_.replace(snapshot_.assets);
    tasks_.replace(snapshot_.tasks);
    template_parameters_.replace(snapshot_.template_parameters);
    emit viewStateChanged();
}

} // namespace space_rhythm::ui
