import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "startPage"
    required property var viewModel
    Accessible.name: qsTr("Space Rhythm 启动页")
    Accessible.description: qsTr("新建、打开或恢复节奏项目")

    Rectangle {
        anchors.fill: parent
        color: "#0d1117"
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(520, root.width - 48)
        spacing: 16

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Space Rhythm")
            color: "#f0f6fc"
            font.pixelSize: 30
            font.weight: Font.DemiBold
            Accessible.name: text
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("从视频或音频开始创建可编辑的节奏项目")
            color: "#a8b3c0"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Accessible.name: text
        }

        Button {
            id: createButton
            objectName: "createProjectButton"
            Layout.fillWidth: true
            text: qsTr("新建项目")
            focus: true
            Accessible.name: text
            Accessible.description: qsTr("创建空的节奏项目工作区")
            onClicked: root.viewModel.createProject()
        }
        Button {
            id: openButton
            objectName: "openProjectButton"
            Layout.fillWidth: true
            text: qsTr("打开项目")
            Accessible.name: text
            Accessible.description: qsTr("打开已有的 Space Rhythm 项目")
            onClicked: root.viewModel.openProject()
        }

        Frame {
            id: recoveryPanel
            objectName: "recoveryPanel"
            Layout.fillWidth: true
            visible: root.viewModel.workspaceState === "recovery"
            Accessible.name: qsTr("项目恢复")
            Accessible.description: root.viewModel.statusMessage

            ColumnLayout {
                anchors.fill: parent
                Label {
                    text: qsTr("检测到较新的自动保存")
                    color: "#f0f6fc"
                    font.weight: Font.DemiBold
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("恢复后以未保存状态打开；原文件不会被覆盖。")
                    color: "#a8b3c0"
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Button {
                        objectName: "recoverAutosaveButton"
                        text: qsTr("恢复自动保存")
                        enabled: root.viewModel.canRecover
                        Accessible.name: text
                        Accessible.description: qsTr("从自动保存恢复并保留原项目文件")
                        onClicked: root.viewModel.recoverAutosave()
                    }
                    Button {
                        objectName: "openPrimaryButton"
                        text: qsTr("打开主文件")
                        enabled: root.viewModel.canRecover
                        Accessible.name: text
                        onClicked: root.viewModel.openPrimary()
                    }
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("UI bridge %1 · schema %2")
                .arg(root.viewModel.bridgeContractVersion)
                .arg(root.viewModel.bridgeSchemaVersion)
            color: "#a8b3c0"
            Accessible.name: text
        }
    }
}
