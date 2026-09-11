pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtTest
import "../../../src/app/qml" as Ui

TestCase {
    id: testCase
    name: "WorkspaceShell"
    when: windowShown

    ListModel {
        id: assets
        ListElement {
            assetId: "asset-01"
            displayName: "sample.mp4"
            kind: "video"
            assetState: "ready"
            durationText: "00:12.000"
            detailText: "视频 + 音频"
        }
    }
    ListModel {
        id: tasks
        ListElement {
            requestId: "task-01"
            operation: "音频分析"
            subject: "sample.mp4"
            stage: "谱变化"
            status: "running"
            progress: 0.42
            progressPpm: 420000
            retryable: false
            diagnosticId: ""
        }
    }
    ListModel {
        id: parameters
        ListElement {
            name: "amplitude-ppm"
            label: "幅度"
            unit: "ppm"
            minimumText: "100000"
            maximumText: "1000000"
            engineeringDefaultText: "900000"
            valueText: "900000"
            advanced: false
        }
    }

    QtObject {
        id: viewModel
        property string bridgeContractVersion: "0.1.0"
        property int bridgeSchemaVersion: 1
        property string route: "workspace"
        property string workspaceState: "idle"
        property string projectTitle: "测试项目"
        property string windowTitle: "测试项目 — Space Rhythm"
        property string activeStage: "edit"
        property string timelineRevisionText: "r9007199254740993"
        property string previewTimeText: "00:12.340"
        property string previewState: "stopped"
        property string statusMessage: "就绪"
        property string statusTone: "neutral"
        property string errorStage: ""
        property string errorDiagnosticId: ""
        property bool projectSafe: true
        property bool dirty: true
        property bool readOnly: false
        property bool recoveryAvailable: false
        property bool canAnalyze: true
        property bool canCancel: true
        property bool canSave: true
        property bool canExport: true
        property bool canPreview: true
        property bool canRecover: false
        property var assets: assets
        property var tasks: tasks
        property var templateParameters: parameters
        property string lastCommand: ""

        function createProject() { lastCommand = "create" }
        function openProject() { lastCommand = "open" }
        function returnToStart() { lastCommand = "start" }
        function activateStage(stage) { activeStage = stage; lastCommand = stage }
        function startAnalysis() { lastCommand = "analyze" }
        function cancelActiveTask() { lastCommand = "cancel" }
        function retryFailedOperation() { lastCommand = "retry" }
        function recoverAutosave() { lastCommand = "recover" }
        function openPrimary() { lastCommand = "primary" }
        function saveProject() { lastCommand = "save" }
        function exportProject() { lastCommand = "export" }
        function togglePreview() { previewState = "playing"; lastCommand = "preview" }
        function stopPreview() { previewState = "stopped"; lastCommand = "stop" }
    }

    ApplicationWindow {
        id: window
        width: 1280
        height: 800
        visible: true

        Ui.WorkspacePage {
            id: workspace
            anchors.fill: parent
            viewModel: viewModel
        }
    }

    Component {
        id: startPageComponent
        Ui.StartPage {}
    }
    Component {
        id: fatalPageComponent
        Ui.FatalErrorPage {}
    }

    function test_workspaceComponentsAndAuthorityText() {
        verify(findChild(workspace, "assetPanel") !== null)
        verify(findChild(workspace, "previewPanel") !== null)
        verify(findChild(workspace, "previewSceneGraphHost") !== null)
        verify(findChild(workspace, "timelinePanel") !== null)
        verify(findChild(workspace, "inspectorPanel") !== null)
        verify(findChild(workspace, "taskDrawer") !== null)
        compare(findChild(workspace, "timelineRevisionLabel").text,
                "时间线 r9007199254740993")
        compare(findChild(workspace, "previewTimeLabel").text, "00:12.340")
    }

    function test_keyControlsHaveStableNamesAndAccessibility() {
        const analyzeButton = findChild(workspace, "analyzeStageButton")
        verify(analyzeButton !== null)
        compare(analyzeButton.Accessible.name, "分析")
        verify(analyzeButton.Accessible.description.length > 0)

        const timeline = findChild(workspace, "timelineSurface")
        verify(timeline !== null)
        verify(timeline.Accessible.name.indexOf("r9007199254740993") >= 0)
        verify(findChild(workspace, "taskCancel_task-01") !== null)
    }

    function test_loadingRunningCancellingFailedAndReadOnlyStates() {
        const banner = findChild(workspace, "workspaceStatusBanner")
        const bannerText = findChild(workspace, "workspaceStatusText")
        const importButton = findChild(workspace, "importStageButton")

        viewModel.workspaceState = "loading"
        viewModel.statusMessage = "正在加载项目"
        tryCompare(banner, "visible", true)
        tryCompare(bannerText, "text", "正在加载项目")

        viewModel.workspaceState = "running"
        viewModel.statusMessage = "任务正在后台运行"
        tryCompare(bannerText, "text", "任务正在后台运行")

        viewModel.workspaceState = "cancelling"
        viewModel.statusMessage = "正在取消任务，等待 worker 确认"
        tryCompare(bannerText, "text", "正在取消任务，等待 worker 确认")

        viewModel.workspaceState = "failed"
        viewModel.statusTone = "error"
        viewModel.statusMessage = "操作失败，项目与原素材安全"
        viewModel.errorStage = "decode_audio"
        viewModel.errorDiagnosticId = "UI-TEST-001"
        tryCompare(findChild(workspace, "statusRetryButton"), "visible", true)

        viewModel.workspaceState = "readOnly"
        viewModel.statusTone = "warning"
        viewModel.statusMessage = "项目以只读方式打开"
        viewModel.readOnly = true
        tryCompare(importButton, "enabled", false)
        tryCompare(findChild(workspace, "readOnlyIndicator"), "visible", true)
    }

    function test_startRecoveryAndFatalPages() {
        const startPage = createTemporaryObject(startPageComponent, window.contentItem,
                                                { "viewModel": viewModel })
        verify(startPage !== null)
        compare(findChild(startPage, "createProjectButton").Accessible.name, "新建项目")

        viewModel.workspaceState = "recovery"
        viewModel.recoveryAvailable = true
        viewModel.canRecover = true
        viewModel.statusMessage = "检测到较新的自动保存，可选择恢复"
        tryCompare(findChild(startPage, "recoveryPanel"), "visible", true)
        verify(findChild(startPage, "recoverAutosaveButton").Accessible.description.length > 0)

        viewModel.statusMessage = "操作失败，项目与原素材安全"
        viewModel.errorStage = "load_project"
        viewModel.errorDiagnosticId = "UI-FATAL-001"
        const fatalPage = createTemporaryObject(fatalPageComponent, window.contentItem,
                                                { "viewModel": viewModel })
        verify(fatalPage !== null)
        verify(findChild(fatalPage, "fatalDiagnosticId").text.indexOf("UI-FATAL-001") >= 0)
        compare(findChild(fatalPage, "fatalReturnButton").Accessible.name, "返回启动页")
    }
}
