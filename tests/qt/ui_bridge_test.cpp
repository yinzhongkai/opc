#include <space_rhythm/rendering/scene_graph_render_item.hpp>
#include <space_rhythm/ui/scene_graph_attachment.hpp>
#include <space_rhythm/ui/view_models.hpp>
#include <space_rhythm/ui/workspace_service.hpp>

#include <QAbstractItemModel>
#include <QMetaObject>
#include <QQuickItem>
#include <QSignalSpy>
#include <QTest>

#include <memory>

namespace {

namespace ui = space_rhythm::ui;

class UiBridgeTest final : public QObject {
    Q_OBJECT

private slots:
    void versionedDescriptorMatchesPublishedContracts()
    {
        const auto service = ui::make_mock_workspace_service();
        const auto descriptor = service->descriptor();
        QCOMPARE(descriptor.schema_version, ui::bridge_schema_version);
        QCOMPARE(descriptor.contract_version, ui::qt_string(ui::bridge_contract_version));
        QCOMPARE(descriptor.core_contract_version,
                 ui::qt_string(space_rhythm::core::contract_version));
        QCOMPARE(descriptor.system_ipc_schema_version,
                 space_rhythm::system::ipc_schema_version);
        QCOMPARE(descriptor.project_schema_version,
                 space_rhythm::system::project_schema_version);
        QCOMPARE(descriptor.rendering_contract_version,
                 ui::qt_string(space_rhythm::rendering::contract_version));
    }

    void qmlSurfaceDoesNotExposeRawAuthorityValues()
    {
        ui::ApplicationViewModel view_model(ui::make_mock_workspace_service(
            ui::MockScenario::idle));
        const auto* meta = view_model.metaObject();
        QVERIFY(meta->indexOfProperty("timelineRevisionText") >= 0);
        QVERIFY(meta->indexOfProperty("previewTimeText") >= 0);
        QCOMPARE(meta->indexOfProperty("timelineRevision"), -1);
        QCOMPARE(meta->indexOfProperty("timeNs"), -1);
        QCOMPARE(meta->indexOfProperty("previewTimeNs"), -1);
        QCOMPARE(meta->indexOfProperty("frameIndex"), -1);
        QCOMPARE(view_model.timelineRevisionText(), QStringLiteral("r42"));
        QCOMPARE(view_model.previewTimeText(), QStringLiteral("00:12.340"));
    }

    void openProjectPublishesModelsWithoutBlocking()
    {
        ui::ApplicationViewModel view_model(ui::make_mock_workspace_service());
        QCOMPARE(view_model.route(), QStringLiteral("start"));
        QCOMPARE(view_model.assets()->rowCount(), 0);

        QSignalSpy changed(&view_model, &ui::ApplicationViewModel::viewStateChanged);
        view_model.openProject();
        QCOMPARE(view_model.route(), QStringLiteral("workspace"));
        QCOMPARE(view_model.workspaceState(), QStringLiteral("loading"));
        QTRY_COMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QCOMPARE(view_model.assets()->rowCount(), 2);
        QVERIFY(changed.count() >= 2);

        const auto roles = view_model.assets()->roleNames();
        QVERIFY(roles.values().contains(QByteArrayLiteral("assetId")));
        QVERIFY(roles.values().contains(QByteArrayLiteral("durationText")));
    }

    void cancellationWaitsForAcknowledgedTerminalSnapshot()
    {
        ui::ApplicationViewModel view_model(ui::make_mock_workspace_service(
            ui::MockScenario::running));
        QCOMPARE(view_model.workspaceState(), QStringLiteral("running"));
        QVERIFY(view_model.canCancel());

        view_model.cancelActiveTask();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("cancelling"));
        QCOMPARE(view_model.tasks()->index(0, 0).data(ui::TaskListModel::StatusRole),
                 QStringLiteral("cancelling"));
        QVERIFY(!view_model.canCancel());

        QTRY_COMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QCOMPARE(view_model.tasks()->index(0, 0).data(ui::TaskListModel::StatusRole),
                 QStringLiteral("cancelled"));
    }

    void failureRecoveryReadOnlyAndFatalScenariosRemainDistinct()
    {
        ui::ApplicationViewModel failed(ui::make_mock_workspace_service(
            ui::MockScenario::failed));
        QCOMPARE(failed.workspaceState(), QStringLiteral("failed"));
        QCOMPARE(failed.errorStage(), QStringLiteral("decode_audio"));
        QVERIFY(failed.projectSafe());

        ui::ApplicationViewModel recovery(ui::make_mock_workspace_service(
            ui::MockScenario::recovery));
        QCOMPARE(recovery.route(), QStringLiteral("start"));
        QCOMPARE(recovery.workspaceState(), QStringLiteral("recovery"));
        QVERIFY(recovery.canRecover());
        recovery.recoverAutosave();
        QCOMPARE(recovery.route(), QStringLiteral("workspace"));
        QCOMPARE(recovery.workspaceState(), QStringLiteral("loading"));
        QTRY_COMPARE(recovery.workspaceState(), QStringLiteral("idle"));
        QVERIFY(recovery.dirty());

        ui::ApplicationViewModel read_only(ui::make_mock_workspace_service(
            ui::MockScenario::read_only));
        QCOMPARE(read_only.workspaceState(), QStringLiteral("readOnly"));
        QVERIFY(read_only.readOnly());
        QVERIFY(!read_only.canAnalyze());
        QVERIFY(!read_only.canSave());
        QVERIFY(read_only.canPreview());
        QVERIFY(read_only.canExport());

        ui::ApplicationViewModel fatal(ui::make_mock_workspace_service(
            ui::MockScenario::fatal));
        QCOMPARE(fatal.route(), QStringLiteral("fatal"));
        QVERIFY(!fatal.errorDiagnosticId().isEmpty());
    }

    void templateModelKeepsSixtyFourBitValuesAsText()
    {
        ui::ApplicationViewModel view_model(ui::make_mock_workspace_service(
            ui::MockScenario::idle));
        auto* model = view_model.templateParameters();
        QVERIFY(model->rowCount() > 0);
        const auto roles = model->roleNames();
        QVERIFY(roles.values().contains(QByteArrayLiteral("valueText")));
        QVERIFY(!roles.values().contains(QByteArrayLiteral("value")));
        const auto value = model->index(0, 0).data(
            ui::TemplateParameterListModel::ValueTextRole);
        QCOMPARE(value.metaType().id(), QMetaType::QString);
    }

    void sceneGraphItemIsAttachedBelowTheQmlHost()
    {
        QQuickItem host;
        host.setObjectName(QStringLiteral("previewSceneGraphHost"));
        host.setSize({640.0, 360.0});

        auto* item = ui::attach_scene_graph_render_item(host);
        QVERIFY(item != nullptr);
        QCOMPARE(item->objectName(), QStringLiteral("sceneGraphRenderItem"));
        QCOMPARE(item->parentItem(), &host);
        QCOMPARE(item->size(), host.size());

        host.setSize({800.0, 450.0});
        QCOMPARE(item->size(), host.size());
        QCOMPARE(item->submitted_snapshot(), nullptr);
    }
};

} // namespace

QTEST_MAIN(UiBridgeTest)

#include "ui_bridge_test.moc"
