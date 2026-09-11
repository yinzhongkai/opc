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
        property string bridgeContractVersion: "0.1.0"
        property int bridgeSchemaVersion: 1
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
