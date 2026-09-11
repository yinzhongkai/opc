import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "workspaceStatusBanner"
    required property var viewModel
    implicitHeight: visible ? bannerLayout.implicitHeight + 16 : 0
    visible: viewModel.workspaceState !== "idle"
    color: viewModel.statusTone === "error"
           ? "#3d1719"
           : viewModel.statusTone === "warning" ? "#332b12" : "#102b3a"
    Accessible.name: viewModel.statusMessage
    Accessible.description: detailLabel.text
    Accessible.role: Accessible.StaticText

    RowLayout {
        id: bannerLayout
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        Label {
            id: stateLabel
            objectName: "workspaceStatusText"
            Layout.fillWidth: true
            text: root.viewModel.statusMessage
            color: "#f0f6fc"
            wrapMode: Text.WordWrap
            Accessible.name: text
        }
        Label {
            id: detailLabel
            visible: root.viewModel.workspaceState === "failed"
            text: qsTr("阶段：%1 · 诊断 ID：%2")
                .arg(root.viewModel.errorStage)
                .arg(root.viewModel.errorDiagnosticId)
            color: "#ffb3ad"
            Accessible.name: text
        }
        Button {
            objectName: "statusRetryButton"
            visible: root.viewModel.workspaceState === "failed"
            text: qsTr("重试")
            Accessible.name: text
            onClicked: root.viewModel.retryFailedOperation()
        }
    }
}
