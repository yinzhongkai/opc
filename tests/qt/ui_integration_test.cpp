#include <space_rhythm/ui/view_models.hpp>
#include <space_rhythm/ui/workspace_service.hpp>

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

namespace {

namespace ui = space_rhythm::ui;

class UiIntegrationTest final : public QObject {
    Q_OBJECT

private slots:
    void realImportCompletesOffTheGuiThread()
    {
        ui::ApplicationViewModel view_model(ui::make_integrated_workspace_service(
            {{}, {}, {}, true}));
        view_model.createProject();
        view_model.importAsset(QUrl::fromLocalFile(
            QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE)).toString());
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workspaceState() == QStringLiteral("idle") ||
                                     view_model.workspaceState() == QStringLiteral("failed"),
                                 30'000);
        qInfo() << "T026 import terminal" << view_model.workspaceState()
                << view_model.errorStage() << view_model.errorDiagnosticId();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QCOMPARE(view_model.assets()->rowCount(), 1);
    }

    void cancellationWaitsForWorkerTerminalAcknowledgement()
    {
        ui::ApplicationViewModel view_model(ui::make_integrated_workspace_service(
            {{}, {}, {}, true}));
        view_model.createProject();
        view_model.importAsset(QUrl::fromLocalFile(
            QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE)).toString());
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 30'000);
        view_model.startAnalysis();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("running"));
        view_model.cancelActiveTask();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("cancelling"));
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 30'000);
        const auto last_row = view_model.tasks()->rowCount() - 1;
        QCOMPARE(view_model.tasks()->index(last_row, 0).data(ui::TaskListModel::StatusRole),
                 QStringLiteral("cancelled"));
    }

    void activeWorkerDisconnectIsRecoverableWithoutPublishingLateResults()
    {
        auto service = ui::make_integrated_workspace_service({{}, {}, {}, true});
        auto* service_control = service.get();
        ui::ApplicationViewModel view_model(std::move(service));
        view_model.createProject();
        view_model.importAsset(QUrl::fromLocalFile(
            QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE)).toString());
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 30'000);
        const auto revision_before = view_model.timelineRevisionText();
        view_model.startAnalysis();
        service_control->post({ui::bridge_schema_version,
                               ui::qt_string(ui::bridge_contract_version),
                               QStringLiteral("disconnect-active"),
                               ui::UiCommandKind::simulate_worker_disconnect,
                               {}});
        QCOMPARE(view_model.workspaceState(), QStringLiteral("failed"));
        QVERIFY(!view_model.workerConnected());
        QTest::qWait(100);
        QCOMPARE(view_model.workspaceState(), QStringLiteral("failed"));
        QCOMPARE(view_model.timelineRevisionText(), revision_before);
        view_model.reconnectWorker();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QVERIFY(view_model.workerConnected());
    }

    void realImportAnalysisEditPreviewSaveAndExportPath()
    {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        const auto media_path = QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE);
        const auto project_path = work.filePath(QStringLiteral("roundtrip.srp"));
        const auto export_path = work.filePath(QStringLiteral("frozen-export.nut"));

        auto service = ui::make_integrated_workspace_service({{}, {}, {}, true});
        auto* service_control = service.get();
        ui::ApplicationViewModel view_model(std::move(service));
        QSignalSpy changed(&view_model, &ui::ApplicationViewModel::viewStateChanged);

        view_model.createProject();
        QCOMPARE(view_model.route(), QStringLiteral("workspace"));
        view_model.importAsset(QUrl::fromLocalFile(media_path).toString());
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 30'000);
        QCOMPARE(view_model.assets()->rowCount(), 1);
        QVERIFY(view_model.canAnalyze());
        QVERIFY(view_model.renderSnapshot() != nullptr);

        view_model.startAnalysis();
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workspaceState() == QStringLiteral("idle") ||
                                     view_model.workspaceState() == QStringLiteral("failed"),
                                 60'000);
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QCOMPARE(view_model.activeStage(), QStringLiteral("edit"));

        const auto viewport_before = view_model.viewportText();
        view_model.zoomTimeline(1);
        QVERIFY(view_model.viewportText() != viewport_before);
        view_model.panTimeline(-80.0, 640.0);

        view_model.addTimelineEvent(320.0, 640.0);
        QVERIFY(view_model.selectedEventText() != QStringLiteral("未选择事件"));
        const auto revision_after_add = view_model.timelineRevisionText();
        view_model.beginTimelineDrag(320.0, 640.0);
        view_model.updateTimelineDrag(400.0, 640.0);
        view_model.endTimelineDrag();
        QVERIFY(view_model.timelineRevisionText() != revision_after_add);
        QVERIFY(view_model.canUndo());

        view_model.toggleSelectedEventLock();
        QVERIFY(view_model.selectedEventLocked());
        view_model.toggleSelectedEventLock();
        QVERIFY(!view_model.selectedEventLocked());
        view_model.addTimelineEvent(200.0, 640.0);
        view_model.toggleTimelineEventSelection(400.0, 640.0);
        QVERIFY(view_model.selectedEventText().startsWith(QStringLiteral("已选择 2 个事件")));
        view_model.batchOffsetSelected(QStringLiteral("25"));
        view_model.undo();
        QVERIFY(view_model.canRedo());
        view_model.redo();

        view_model.togglePreview();
        QCOMPARE(view_model.previewState(), QStringLiteral("playing"));
        const auto initial_preview_text = view_model.previewTimeText();
        QTRY_VERIFY_WITH_TIMEOUT(view_model.previewTimeText() != initial_preview_text, 5'000);
        QTRY_VERIFY_WITH_TIMEOUT(view_model.authoritativeFrameIndex() > 0, 5'000);
        view_model.seekTimeline(80.0, 640.0);
        QCOMPARE(view_model.previewState(), QStringLiteral("playing"));
        const auto seek_preview_text = view_model.previewTimeText();
        QTRY_VERIFY_WITH_TIMEOUT(view_model.previewTimeText() != seek_preview_text, 5'000);
        view_model.togglePreview();
        QCOMPARE(view_model.previewState(), QStringLiteral("paused"));
        const auto paused_preview_text = view_model.previewTimeText();
        QTest::qWait(50);
        QCOMPARE(view_model.previewTimeText(), paused_preview_text);
        view_model.togglePreview();
        QCOMPARE(view_model.previewState(), QStringLiteral("playing"));
        QTRY_VERIFY_WITH_TIMEOUT(view_model.previewTimeText() != paused_preview_text, 5'000);
        view_model.stopPreview();
        QCOMPARE(view_model.previewState(), QStringLiteral("stopped"));

        view_model.saveProjectTo(QUrl::fromLocalFile(project_path).toString());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(project_path), 5'000);
        QVERIFY(!view_model.dirty());

        qInfo() << "T026 pre-export revisions" << view_model.timelineRevisionText()
                << view_model.renderSnapshot()->recipe().timeline_revision;

        view_model.exportProjectTo(QUrl::fromLocalFile(export_path).toString());
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workspaceState() == QStringLiteral("idle") ||
                                     view_model.workspaceState() == QStringLiteral("failed"),
                                 120'000);
        qInfo() << "T026 export terminal" << view_model.workspaceState()
                << view_model.errorStage() << view_model.errorDiagnosticId();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QVERIFY(QFileInfo(export_path).size() > 0);
        QVERIFY(!QFileInfo::exists(export_path + QStringLiteral(".partial")));

        service_control->post({ui::bridge_schema_version,
                               ui::qt_string(ui::bridge_contract_version),
                               QStringLiteral("disconnect-test"),
                               ui::UiCommandKind::simulate_worker_disconnect,
                               {}});
        QVERIFY(!view_model.workerConnected());
        QCOMPARE(view_model.workspaceState(), QStringLiteral("failed"));
        view_model.reconnectWorker();
        QVERIFY(view_model.workerConnected());
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        QVERIFY(changed.count() >= 12);
    }

    void savedProjectReopensThroughTheSameVersionedBridge()
    {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        const auto project_path = work.filePath(QStringLiteral("reopen.srp"));
        {
            ui::ApplicationViewModel writer(ui::make_integrated_workspace_service(
                {{}, {}, {}, true}));
            writer.createProject();
            writer.addTimelineEvent(100.0, 500.0);
            writer.saveProjectTo(QUrl::fromLocalFile(project_path).toString());
            QVERIFY(QFileInfo::exists(project_path));
        }
        ui::ApplicationViewModel reader(ui::make_integrated_workspace_service(
            {{}, {}, {}, true}));
        reader.openProjectFrom(QUrl::fromLocalFile(project_path).toString());
        QTRY_COMPARE_WITH_TIMEOUT(reader.workspaceState(), QStringLiteral("idle"), 10'000);
        QCOMPARE(reader.route(), QStringLiteral("workspace"));
        QVERIFY(reader.renderSnapshot() != nullptr);
        QVERIFY(!reader.dirty());
    }
};

} // namespace

QTEST_MAIN(UiIntegrationTest)

#include "ui_integration_test.moc"
