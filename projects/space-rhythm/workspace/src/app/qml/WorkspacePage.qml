import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

FocusScope {
    id: root
    objectName: "workspacePage"
    required property var viewModel
    Accessible.name: qsTr("Space Rhythm 编辑工作区")
    Accessible.description: root.viewModel.statusMessage

    Rectangle {
        anchors.fill: parent
        color: "#0d1117"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StageToolbar {
            id: stageToolbar
            Layout.fillWidth: true
            viewModel: root.viewModel
        }

        StatusBanner {
            Layout.fillWidth: true
            viewModel: root.viewModel
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            AssetPanel {
                id: assetPanel
                Layout.preferredWidth: 240
                Layout.minimumWidth: 190
                Layout.fillHeight: true
                viewModel: root.viewModel
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 420
                spacing: 4

                PreviewPanel {
                    id: previewPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 240
                    viewModel: root.viewModel
                }

                TimelinePanel {
                    id: timelinePanel
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    Layout.minimumHeight: 180
                    viewModel: root.viewModel
                }
            }

            InspectorPanel {
                id: inspectorPanel
                Layout.preferredWidth: 300
                Layout.minimumWidth: 240
                Layout.fillHeight: true
                viewModel: root.viewModel
            }
        }

        TaskDrawer {
            id: taskDrawer
            Layout.fillWidth: true
            viewModel: root.viewModel
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 26
            color: "#161b22"
            Accessible.name: statusText.text
            Accessible.role: Accessible.StaticText

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Label {
                    id: statusText
                    objectName: "projectStatusText"
                    Layout.fillWidth: true
                    text: root.viewModel.dirty ? qsTr("未保存") : qsTr("已保存")
                    color: root.viewModel.dirty ? "#e3b341" : "#a8b3c0"
                    Accessible.name: text
                }
                Label {
                    text: root.viewModel.projectTitle + " · " + root.viewModel.timelineRevisionText
                    color: "#a8b3c0"
                    Accessible.name: text
                }
            }
        }
    }

    Shortcut {
        sequence: StandardKey.Save
        enabled: root.viewModel.canSave
        onActivated: {
            if (root.viewModel.lastSavedPath.length > 0)
                root.viewModel.saveProject()
            else
                shortcutSaveDialog.open()
        }
    }
    Shortcut {
        sequence: "Space"
        enabled: root.viewModel.canPreview
        onActivated: root.viewModel.togglePreview()
    }
    Shortcut {
        sequence: "Escape"
        enabled: root.viewModel.timelineGestureActive || root.viewModel.canCancel
        onActivated: {
            if (root.viewModel.timelineGestureActive)
                root.viewModel.cancelTimelineGesture()
            else
                taskDrawer.confirmCancel()
        }
    }
    Shortcut {
        sequence: StandardKey.Undo
        enabled: root.viewModel.canUndo
        onActivated: root.viewModel.undo()
    }
    Shortcut {
        sequence: StandardKey.Redo
        enabled: root.viewModel.canRedo
        onActivated: root.viewModel.redo()
    }
    Shortcut {
        sequence: "Ctrl+Shift+E"
        enabled: root.viewModel.canExport
        onActivated: shortcutExportDialog.open()
    }
    Shortcut {
        sequence: "F6"
        onActivated: timelinePanel.forceActiveFocus(Qt.TabFocusReason)
    }

    FileDialog {
        id: shortcutSaveDialog
        objectName: "shortcutSaveDialog"
        title: qsTr("保存 Space Rhythm 项目")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "srp"
        nameFilters: [qsTr("Space Rhythm 项目 (*.srp)")]
        onAccepted: root.viewModel.saveProjectTo(selectedFile.toString())
    }

    FileDialog {
        id: shortcutExportDialog
        objectName: "shortcutExportDialog"
        title: qsTr("导出冻结快照（开发格式）")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "nut"
        nameFilters: [qsTr("T-019 测试导出 (*.nut)")]
        onAccepted: root.viewModel.exportProjectTo(selectedFile.toString())
    }
}
