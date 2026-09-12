#pragma once

#include <space_rhythm/ui/workspace_service.hpp>

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include <memory>

namespace space_rhythm::ui {

class AssetListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        AssetIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        KindRole,
        StateRole,
        DurationTextRole,
        DetailTextRole,
    };
    Q_ENUM(Role)

    explicit AssetListModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replace(QVector<AssetDto> assets);

private:
    QVector<AssetDto> assets_;
};

class TaskListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        RequestIdRole = Qt::UserRole + 1,
        OperationRole,
        SubjectRole,
        StageRole,
        StatusRole,
        ProgressRole,
        ProgressPpmRole,
        RetryableRole,
        DiagnosticIdRole,
    };
    Q_ENUM(Role)

    explicit TaskListModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replace(QVector<TaskDto> tasks);

private:
    QVector<TaskDto> tasks_;
};

class TemplateParameterListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        UnitRole,
        MinimumTextRole,
        MaximumTextRole,
        EngineeringDefaultTextRole,
        ValueTextRole,
        AdvancedRole,
    };
    Q_ENUM(Role)

    explicit TemplateParameterListModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void replace(QVector<TemplateParameterDto> parameters);

private:
    QVector<TemplateParameterDto> parameters_;
};

class ApplicationViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString bridgeContractVersion READ bridgeContractVersion CONSTANT)
    Q_PROPERTY(int bridgeSchemaVersion READ bridgeSchemaVersion CONSTANT)
    Q_PROPERTY(QString route READ route NOTIFY viewStateChanged)
    Q_PROPERTY(QString workspaceState READ workspaceState NOTIFY viewStateChanged)
    Q_PROPERTY(QString projectTitle READ projectTitle NOTIFY viewStateChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle NOTIFY viewStateChanged)
    Q_PROPERTY(QString activeStage READ activeStage NOTIFY viewStateChanged)
    Q_PROPERTY(QString timelineRevisionText READ timelineRevisionText NOTIFY viewStateChanged)
    Q_PROPERTY(QString previewTimeText READ previewTimeText NOTIFY viewStateChanged)
    Q_PROPERTY(QString previewState READ previewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY viewStateChanged)
    Q_PROPERTY(QString statusTone READ statusTone NOTIFY viewStateChanged)
    Q_PROPERTY(QString errorStage READ errorStage NOTIFY viewStateChanged)
    Q_PROPERTY(QString errorDiagnosticId READ errorDiagnosticId NOTIFY viewStateChanged)
    Q_PROPERTY(bool projectSafe READ projectSafe NOTIFY viewStateChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY viewStateChanged)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY viewStateChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY viewStateChanged)
    Q_PROPERTY(bool canAnalyze READ canAnalyze NOTIFY viewStateChanged)
    Q_PROPERTY(bool canCancel READ canCancel NOTIFY viewStateChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY viewStateChanged)
    Q_PROPERTY(bool canExport READ canExport NOTIFY viewStateChanged)
    Q_PROPERTY(bool canPreview READ canPreview NOTIFY viewStateChanged)
    Q_PROPERTY(bool canRecover READ canRecover NOTIFY viewStateChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY viewStateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY viewStateChanged)
    Q_PROPERTY(bool canEditTimeline READ canEditTimeline NOTIFY viewStateChanged)
    Q_PROPERTY(bool workerConnected READ workerConnected NOTIFY viewStateChanged)
    Q_PROPERTY(QString selectedEventText READ selectedEventText NOTIFY viewStateChanged)
    Q_PROPERTY(bool selectedEventLocked READ selectedEventLocked NOTIFY viewStateChanged)
    Q_PROPERTY(QString viewportText READ viewportText NOTIFY viewStateChanged)
    Q_PROPERTY(QString lastSavedPath READ lastSavedPath NOTIFY viewStateChanged)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY viewStateChanged)
    Q_PROPERTY(QAbstractItemModel* assets READ assets CONSTANT)
    Q_PROPERTY(QAbstractItemModel* tasks READ tasks CONSTANT)
    Q_PROPERTY(QAbstractItemModel* templateParameters READ templateParameters CONSTANT)

public:
    explicit ApplicationViewModel(std::unique_ptr<WorkspaceService> service,
                                  QObject* parent = nullptr);
    ~ApplicationViewModel() override;

    ApplicationViewModel(const ApplicationViewModel&) = delete;
    ApplicationViewModel& operator=(const ApplicationViewModel&) = delete;

    [[nodiscard]] QString bridgeContractVersion() const;
    [[nodiscard]] int bridgeSchemaVersion() const noexcept;
    [[nodiscard]] QString route() const;
    [[nodiscard]] QString workspaceState() const;
    [[nodiscard]] QString projectTitle() const;
    [[nodiscard]] QString windowTitle() const;
    [[nodiscard]] QString activeStage() const;
    [[nodiscard]] QString timelineRevisionText() const;
    [[nodiscard]] QString previewTimeText() const;
    [[nodiscard]] QString previewState() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString statusTone() const;
    [[nodiscard]] QString errorStage() const;
    [[nodiscard]] QString errorDiagnosticId() const;
    [[nodiscard]] bool projectSafe() const noexcept;
    [[nodiscard]] bool dirty() const noexcept;
    [[nodiscard]] bool readOnly() const noexcept;
    [[nodiscard]] bool recoveryAvailable() const noexcept;
    [[nodiscard]] bool canAnalyze() const noexcept;
    [[nodiscard]] bool canCancel() const noexcept;
    [[nodiscard]] bool canSave() const noexcept;
    [[nodiscard]] bool canExport() const noexcept;
    [[nodiscard]] bool canPreview() const noexcept;
    [[nodiscard]] bool canRecover() const noexcept;
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    [[nodiscard]] bool canEditTimeline() const noexcept;
    [[nodiscard]] bool workerConnected() const noexcept;
    [[nodiscard]] QString selectedEventText() const;
    [[nodiscard]] bool selectedEventLocked() const noexcept;
    [[nodiscard]] QString viewportText() const;
    [[nodiscard]] QString lastSavedPath() const;
    [[nodiscard]] QString lastExportPath() const;
    [[nodiscard]] QAbstractItemModel* assets() noexcept;
    [[nodiscard]] QAbstractItemModel* tasks() noexcept;
    [[nodiscard]] QAbstractItemModel* templateParameters() noexcept;

    Q_INVOKABLE void createProject();
    Q_INVOKABLE void openProject();
    Q_INVOKABLE void returnToStart();
    Q_INVOKABLE void activateStage(const QString& stage);
    Q_INVOKABLE void startAnalysis();
    Q_INVOKABLE void cancelActiveTask();
    Q_INVOKABLE void retryFailedOperation();
    Q_INVOKABLE void recoverAutosave();
    Q_INVOKABLE void openPrimary();
    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void exportProject();
    Q_INVOKABLE void togglePreview();
    Q_INVOKABLE void stopPreview();
    Q_INVOKABLE void openProjectFrom(const QString& path);
    Q_INVOKABLE void importAsset(const QString& path);
    Q_INVOKABLE void saveProjectTo(const QString& path);
    Q_INVOKABLE void exportProjectTo(const QString& path);
    Q_INVOKABLE void zoomTimeline(int steps);
    Q_INVOKABLE void panTimeline(double deltaPixels, double widthPixels);
    Q_INVOKABLE void seekTimeline(double xPixels, double widthPixels);
    Q_INVOKABLE void selectTimelineEvent(double xPixels, double widthPixels);
    Q_INVOKABLE void addTimelineEvent(double xPixels, double widthPixels);
    Q_INVOKABLE void toggleTimelineEventSelection(double xPixels, double widthPixels);
    Q_INVOKABLE void beginTimelineDrag(double xPixels, double widthPixels);
    Q_INVOKABLE void updateTimelineDrag(double xPixels, double widthPixels);
    Q_INVOKABLE void endTimelineDrag();
    Q_INVOKABLE void toggleSelectedEventLock();
    Q_INVOKABLE void batchOffsetSelected(const QString& milliseconds);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void reconnectWorker();

    // C++-only authority accessors. They are deliberately absent from the Qt
    // meta-object, so QML cannot store, round or reinterpret these values.
    [[nodiscard]] std::shared_ptr<const rendering::RenderSnapshot> renderSnapshot() const;
    [[nodiscard]] core::TimeRange viewportTimeRange() const noexcept;
    [[nodiscard]] rendering::FrameIndex authoritativeFrameIndex() const noexcept;

signals:
    void viewStateChanged();

private:
    void dispatch(UiCommandKind kind, QString argument = {});
    void receive_snapshot(WorkspaceSnapshotDto snapshot);
    void apply_snapshot(WorkspaceSnapshotDto snapshot);

    std::unique_ptr<WorkspaceService> service_;
    WorkspaceSnapshotDto snapshot_;
    AssetListModel assets_;
    TaskListModel tasks_;
    TemplateParameterListModel template_parameters_;
    std::uint64_t next_request_id_{1};
};

} // namespace space_rhythm::ui
