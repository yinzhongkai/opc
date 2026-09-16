import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "fatalErrorPage"
    required property var viewModel
    Accessible.name: qsTr("致命错误页")
    Accessible.description: root.viewModel.statusMessage

    Rectangle {
        anchors.fill: parent
        color: "#0d1117"
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(560, root.width - 48)
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("无法继续打开此项目")
            color: "#ff7b72"
            font.pixelSize: 24
            font.weight: Font.DemiBold
            Accessible.name: text
        }
        Label {
            objectName: "fatalErrorMessage"
            Layout.fillWidth: true
            text: root.viewModel.statusMessage
            color: "#f0f6fc"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Accessible.name: text
        }
        Label {
            objectName: "fatalDiagnosticId"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("阶段：%1 · 诊断 ID：%2")
                .arg(root.viewModel.errorStage)
                .arg(root.viewModel.errorDiagnosticId)
            color: "#a8b3c0"
            Accessible.name: text
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.viewModel.projectSafe ? qsTr("项目文件和原素材未被修改")
                                             : qsTr("项目安全状态未知，请保留诊断信息")
            color: root.viewModel.projectSafe ? "#56d364" : "#e3b341"
            Accessible.name: text
        }
        Button {
            objectName: "fatalReturnButton"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("返回启动页")
            focus: true
            Accessible.name: text
            Accessible.description: qsTr("保留现有文件并返回启动页")
            onClicked: root.viewModel.returnToStart()
        }
    }
}
