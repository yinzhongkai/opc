#include <space_rhythm/ui/view_models.hpp>
#include <space_rhythm/ui/workspace_service.hpp>
#include <space_rhythm/worker/ui_worker_protocol.hpp>

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>

namespace {

namespace ui = space_rhythm::ui;

ui::IntegratedWorkspaceOptions integrated_options(std::uint32_t worker_delay_ms = 0)
{
    ui::IntegratedWorkspaceOptions options;
    options.headless = true;
    options.worker_executable_path = QStringLiteral(SPACE_RHYTHM_T026_WORKER_EXE);
    options.worker_test_delay_ms = worker_delay_ms;
    return options;
}

bool exported_audio_has_signal(const QString& path_value)
{
    const auto path = std::filesystem::path{path_value.toStdWString()};
    const auto source = space_rhythm::media::MediaSource::open(path);
    if (!source) {
        return false;
    }
    space_rhythm::media::StreamSelectionRequest video;
    video.mode = space_rhythm::media::SelectionMode::none;
    space_rhythm::media::StreamSelectionRequest audio;
    audio.mode = space_rhythm::media::SelectionMode::required_default_then_lowest_index;
    const auto selection = source.value()->select(video, audio);
    if (!selection || selection.value().audio.selected.empty()) {
        return false;
    }
    space_rhythm::audio::PcmNarrowAdapter adapter;
    bool has_signal = false;
    space_rhythm::media::AudioCallbacks callbacks;
    callbacks.on_format_changed = [](const space_rhythm::media::FormatChanged&) {
        return space_rhythm::media::PublishResult::accepted;
    };
    callbacks.on_pcm = [&](space_rhythm::media::PcmBuffer pcm) {
        const auto adapted = adapter.adapt(pcm);
        if (!adapted) {
            return space_rhythm::media::PublishResult::closed;
        }
        for (std::uint64_t frame = 0; frame < adapted.value().valid_frame_count;
             ++frame) {
            const auto sample = adapted.value().sample(frame, 0);
            if (sample && std::abs(sample.value()) > 0.0001F) {
                has_signal = true;
                break;
            }
        }
        return space_rhythm::media::PublishResult::accepted;
    };
    space_rhythm::media::AudioOutputSpec output;
    output.sample_rate = 48'000;
    output.channels = 1;
    const auto decoded = source.value()->decode_audio(selection.value(),
                                                      selection.value().audio.selected.front(),
                                                      output,
                                                      {},
                                                      callbacks);
    return decoded.has_value() && has_signal;
}

class UiIntegrationTest final : public QObject {
    Q_OBJECT

private slots:
    void workerResultReferencesAreVersionedAndVerifiedOutOfBand()
    {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        const auto import_path = std::filesystem::path{
            work.filePath(QStringLiteral("import-result.bin")).toStdWString()};
        space_rhythm::worker::ImportResultDto input;
        input.source_path = std::filesystem::path{L"C:/media/input.mkv"};
        input.source_fingerprint_sha256 = std::string(64, 'a');
        input.source_size_bytes = 123'456;
        input.duration_ns = 9'007'199'254'740'993LL;
        input.has_video = true;
        input.has_audio = true;
        input.ffmpeg_version = "8.1.2";
        input.stream_count = 2;
        input.selection.presentation_origin.timestamp.time_base = {1, 1'000};
        const auto written = space_rhythm::worker::write_import_result(import_path, input);
        QVERIFY(written.has_value());
        const auto reference = space_rhythm::worker::file_reference(import_path);
        QVERIFY(reference.has_value());
        QCOMPARE(reference.value().kind, space_rhythm::system::DataReferenceKind::file);
        QCOMPARE(reference.value().sha256.size(), 64U);
        const auto decoded = space_rhythm::worker::read_import_result(import_path);
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded.value().duration_ns, input.duration_ns);
        QCOMPARE(decoded.value().source_size_bytes, input.source_size_bytes);
        QCOMPARE(decoded.value().source_fingerprint_sha256,
                 input.source_fingerprint_sha256);

        const auto corrupt_path = work.filePath(QStringLiteral("corrupt.bin"));
        QFile corrupt(corrupt_path);
        QVERIFY(corrupt.open(QIODevice::WriteOnly));
        QCOMPARE(corrupt.write("bad", 3), 3);
        corrupt.close();
        QVERIFY(!space_rhythm::worker::read_import_result(
                     std::filesystem::path{corrupt_path.toStdWString()})
                     .has_value());
    }

    void realImportCompletesOffTheGuiThread()
    {
        ui::ApplicationViewModel view_model(ui::make_integrated_workspace_service(
            integrated_options()));
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
            integrated_options(2'000)));
        view_model.createProject();
        view_model.importAsset(QUrl::fromLocalFile(
            QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE)).toString());
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workspaceState() == QStringLiteral("idle") ||
                                     view_model.workspaceState() == QStringLiteral("failed"),
                                 30'000);
        qInfo() << "T026 cancellation terminal" << view_model.workspaceState()
                << view_model.errorStage() << view_model.errorDiagnosticId();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        view_model.startAnalysis();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("running"));
        view_model.cancelActiveTask();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("cancelling"));
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workspaceState() == QStringLiteral("idle") ||
                                     view_model.workspaceState() == QStringLiteral("failed"),
                                 30'000);
        qInfo() << "T026 cancellation acknowledgement" << view_model.workspaceState()
                << view_model.errorStage() << view_model.errorDiagnosticId();
        QCOMPARE(view_model.workspaceState(), QStringLiteral("idle"));
        const auto last_row = view_model.tasks()->rowCount() - 1;
        QCOMPARE(view_model.tasks()->index(last_row, 0).data(ui::TaskListModel::StatusRole),
                 QStringLiteral("cancelled"));
    }

    void activeWorkerDisconnectIsRecoverableWithoutPublishingLateResults()
    {
        auto service = ui::make_integrated_workspace_service(integrated_options(2'000));
        auto* service_control = service.get();
        ui::ApplicationViewModel view_model(std::move(service));
        view_model.createProject();
        view_model.importAsset(QUrl::fromLocalFile(
            QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE)).toString());
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 30'000);
        const auto revision_before = view_model.timelineRevisionText();
        view_model.startAnalysis();
        QTest::qWait(100);
        service_control->post({ui::bridge_schema_version,
                               ui::qt_string(ui::bridge_contract_version),
                               QStringLiteral("disconnect-active"),
                               ui::UiCommandKind::simulate_worker_disconnect,
                               {}});
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("failed"), 5'000);
        QTRY_VERIFY_WITH_TIMEOUT(!view_model.workerConnected(), 5'000);
        QTest::qWait(100);
        QCOMPARE(view_model.workspaceState(), QStringLiteral("failed"));
        QCOMPARE(view_model.timelineRevisionText(), revision_before);
        view_model.reconnectWorker();
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 5'000);
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workerConnected(), 5'000);
    }

    void dragAndSeekUsePendingTransactionsAndRejectStaleRevision()
    {
        ui::ApplicationViewModel view_model(
            ui::make_integrated_workspace_service(integrated_options()));
        view_model.createProject();
        view_model.addTimelineEvent(320.0, 640.0);
        const auto revision_after_add = view_model.timelineRevisionText();

        view_model.beginTimelineDrag(320.0, 640.0);
        QVERIFY(view_model.timelineGestureActive());
        view_model.updateTimelineDrag(360.0, 640.0);
        view_model.updateTimelineDrag(400.0, 640.0);
        QCOMPARE(view_model.timelineRevisionText(), revision_after_add);
        view_model.cancelTimelineGesture();
        QCOMPARE(view_model.timelineRevisionText(), revision_after_add);
        QVERIFY(!view_model.timelineGestureActive());

        view_model.beginTimelineDrag(320.0, 640.0);
        view_model.updateTimelineDrag(400.0, 640.0);
        QCOMPARE(view_model.timelineRevisionText(), revision_after_add);
        view_model.endTimelineDrag();
        const auto revision_after_commit = view_model.timelineRevisionText();
        QVERIFY(revision_after_commit != revision_after_add);

        view_model.beginTimelineDrag(400.0, 640.0);
        view_model.updateTimelineDrag(450.0, 640.0);
        view_model.addTimelineEvent(100.0, 640.0);
        const auto revision_after_external_edit = view_model.timelineRevisionText();
        view_model.endTimelineDrag();
        QCOMPARE(view_model.timelineRevisionText(), revision_after_external_edit);
        QVERIFY(view_model.interactionStatusText().contains(QStringLiteral("已更新")));

        const auto time_before_seek = view_model.previewTimeText();
        view_model.beginTimelineSeek(500.0, 640.0);
        view_model.updateTimelineSeek(540.0, 640.0);
        QVERIFY(view_model.timelineSeekPending());
        QCOMPARE(view_model.previewTimeText(), time_before_seek);
        view_model.cancelTimelineGesture();
        QCOMPARE(view_model.previewTimeText(), time_before_seek);
        view_model.beginTimelineSeek(500.0, 640.0);
        view_model.endTimelineSeek();
        QVERIFY(view_model.previewTimeText() != time_before_seek);
    }

    void realImportAnalysisEditPreviewSaveAndExportPath()
    {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        const auto media_path = QStringLiteral(SPACE_RHYTHM_T026_MEDIA_FILE);
        const auto project_path = work.filePath(QStringLiteral("roundtrip.srp"));
        const auto export_path = work.filePath(QStringLiteral("frozen-export.nut"));

        auto service = ui::make_integrated_workspace_service(integrated_options());
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
        QVERIFY(!view_model.developmentAudioMappingEnabled());
        QVERIFY(!view_model.canPreview());
        QVERIFY(!view_model.canExport());
        view_model.enableDevelopmentAudioMapping();
        QVERIFY(view_model.developmentAudioMappingEnabled());
        QVERIFY(view_model.canPreview());
        QVERIFY(view_model.canExport());

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
        QTRY_COMPARE_WITH_TIMEOUT(view_model.previewState(), QStringLiteral("playing"), 10'000);
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
        QVERIFY(view_model.dirty());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(project_path), 5'000);
        QTRY_VERIFY_WITH_TIMEOUT(!view_model.dirty(), 5'000);

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
        QVERIFY(exported_audio_has_signal(export_path));

        service_control->post({ui::bridge_schema_version,
                               ui::qt_string(ui::bridge_contract_version),
                               QStringLiteral("disconnect-test"),
                               ui::UiCommandKind::simulate_worker_disconnect,
                               {}});
        QTRY_VERIFY_WITH_TIMEOUT(!view_model.workerConnected(), 5'000);
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("failed"), 5'000);
        view_model.reconnectWorker();
        QTRY_VERIFY_WITH_TIMEOUT(view_model.workerConnected(), 5'000);
        QTRY_COMPARE_WITH_TIMEOUT(view_model.workspaceState(), QStringLiteral("idle"), 5'000);
        QVERIFY(changed.count() >= 12);
    }

    void savedProjectReopensThroughTheSameVersionedBridge()
    {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        const auto project_path = work.filePath(QStringLiteral("reopen.srp"));
        {
            ui::ApplicationViewModel writer(ui::make_integrated_workspace_service(
                integrated_options()));
            writer.createProject();
            writer.addTimelineEvent(100.0, 500.0);
            writer.saveProjectTo(QUrl::fromLocalFile(project_path).toString());
            QVERIFY(writer.dirty());
            QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(project_path), 5'000);
            QTRY_VERIFY_WITH_TIMEOUT(!writer.dirty(), 5'000);
        }
        ui::ApplicationViewModel reader(ui::make_integrated_workspace_service(
            integrated_options()));
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
