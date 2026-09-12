pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    objectName: "applicationShell"
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    visible: true
    title: effectiveViewModel.windowTitle
    color: "#0d1117"

    property var appViewModel: null
    readonly property var effectiveViewModel: appViewModel ? appViewModel : standaloneViewModel

    ListModel { id: standaloneAssets }
    ListModel { id: standaloneTasks }
    ListModel {
        id: standaloneTemplateParameters
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
        id: standaloneViewModel
        property string bridgeContractVersion: "0.2.0"
        property int bridgeSchemaVersion: 2
        property string route: "start"
        property string workspaceState: "idle"
        property string projectTitle: ""
        property string windowTitle: projectTitle.length > 0 ? projectTitle + " — Space Rhythm" : "Space Rhythm"
        property string activeStage: "import"
        property string timelineRevisionText: "r0"
        property string previewTimeText: "00:00.000"
        property string previewState: "stopped"
        property string statusMessage: "请选择新建或打开项目"
        property string statusTone: "neutral"
        property string errorStage: ""
        property string errorDiagnosticId: ""
        property bool projectSafe: true
        property bool dirty: false
        property bool readOnly: false
        property bool recoveryAvailable: false
        property bool canAnalyze: false
        property bool canCancel: false
        property bool canSave: false
        property bool canExport: false
        property bool canPreview: false
        property bool canRecover: false
        property bool canUndo: false
        property bool canRedo: false
        property bool canEditTimeline: route === "workspace" && !readOnly
        property bool workerConnected: true
        property string selectedEventText: "未选择事件"
        property bool selectedEventLocked: false
        property string viewportText: "00:00.000 – 00:10.000"
        property string lastSavedPath: ""
        property string lastExportPath: ""
        property string lastCommand: ""
        property var assets: standaloneAssets
        property var tasks: standaloneTasks
        property var templateParameters: standaloneTemplateParameters

        function createProject() {
            projectTitle = "未命名节奏"
            route = "workspace"
            statusMessage = "就绪"
        }
        function openProject() { createProject() }
        function returnToStart() { route = "start" }
        function activateStage(stage) { activeStage = stage }
        function startAnalysis() {}
        function cancelActiveTask() {}
        function retryFailedOperation() {}
        function recoverAutosave() {}
        function openPrimary() {}
        function saveProject() {}
        function exportProject() {}
        function togglePreview() {}
        function stopPreview() {}
        function openProjectFrom(path) { openProject() }
        function importAsset(path) { lastCommand = "import:" + path }
        function saveProjectTo(path) { lastSavedPath = path; lastCommand = "saveAs" }
        function exportProjectTo(path) { lastExportPath = path; lastCommand = "exportAs" }
        function zoomTimeline(steps) { lastCommand = "zoom" }
        function panTimeline(deltaPixels, widthPixels) { lastCommand = "pan" }
        function seekTimeline(xPixels, widthPixels) { lastCommand = "seek" }
        function selectTimelineEvent(xPixels, widthPixels) { lastCommand = "select" }
        function addTimelineEvent(xPixels, widthPixels) { lastCommand = "addEvent" }
        function toggleTimelineEventSelection(xPixels, widthPixels) { lastCommand = "toggleSelection" }
        function beginTimelineDrag(xPixels, widthPixels) { lastCommand = "dragBegin" }
        function updateTimelineDrag(xPixels, widthPixels) { lastCommand = "drag" }
        function endTimelineDrag() { lastCommand = "dragEnd" }
        function toggleSelectedEventLock() { selectedEventLocked = !selectedEventLocked }
        function batchOffsetSelected(milliseconds) { lastCommand = "offset" }
        function undo() { lastCommand = "undo" }
        function redo() { lastCommand = "redo" }
        function reconnectWorker() { workerConnected = true }
    }

    Loader {
        id: pageLoader
        objectName: "applicationPageLoader"
        anchors.fill: parent
        focus: true
        sourceComponent: root.effectiveViewModel.route === "workspace"
                         ? workspacePage
                         : root.effectiveViewModel.route === "fatal" ? fatalPage : startPage
    }

    Component {
        id: startPage
        StartPage { viewModel: root.effectiveViewModel }
    }
    Component {
        id: workspacePage
        WorkspacePage { viewModel: root.effectiveViewModel }
    }
    Component {
        id: fatalPage
        FatalErrorPage { viewModel: root.effectiveViewModel }
    }

    Component.onCompleted: {
        if (Qt.application.arguments.indexOf("--smoke") !== -1)
            Qt.callLater(Qt.quit)
    }
}
