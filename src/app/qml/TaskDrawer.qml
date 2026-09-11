pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    objectName: "taskDrawer"
    required property var viewModel
    padding: 0
    implicitHeight: Math.max(72, Math.min(180, taskList.contentHeight + 42))
    Accessible.name: qsTr("后台任务抽屉")
    Accessible.description: qsTr("显示排队、运行、取消中、成功、失败和已取消任务")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    text: qsTr("后台任务")
                    font.weight: Font.DemiBold
                    Accessible.name: text
                }
                Label {
                    text: qsTr("%1 项").arg(taskList.count)
                    color: "#a8b3c0"
                    Accessible.name: text
                }
            }
        }

        Label {
            objectName: "taskEmptyState"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: taskList.count === 0
            text: qsTr("当前没有后台任务")
            color: "#a8b3c0"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Accessible.name: text
        }

        ListView {
            id: taskList
            objectName: "taskList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: count > 0
            clip: true
            model: root.viewModel.tasks
            Accessible.name: qsTr("任务列表，共 %1 项").arg(count)

            delegate: ItemDelegate {
                id: taskDelegate
                required property string requestId
                required property string operation
                required property string subject
                required property string stage
                required property string status
                required property real progress
                required property int progressPpm
                required property bool retryable
                required property string diagnosticId
                width: ListView.view.width
                objectName: "taskRow_" + taskDelegate.requestId
                Accessible.name: taskDelegate.operation + " · " + taskDelegate.status
                Accessible.description: taskDelegate.stage + " · "
                                        + Math.round(taskDelegate.progress * 100) + "%"

                contentItem: RowLayout {
                    spacing: 10
                    ColumnLayout {
                        Layout.preferredWidth: 180
                        Label { text: taskDelegate.operation; color: "#f0f6fc" }
                        Label {
                            text: taskDelegate.subject + " · " + taskDelegate.stage
                            color: "#a8b3c0"
                        }
                    }
                    ProgressBar {
                        objectName: "taskProgress_" + taskDelegate.requestId
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        value: taskDelegate.progress
                        indeterminate: taskDelegate.status === "queued"
                        Accessible.name: qsTr("%1 进度 %2%")
                                         .arg(taskDelegate.operation)
                                         .arg(Math.round(taskDelegate.progress * 100))
                    }
                    Label {
                        text: taskDelegate.status
                        color: taskDelegate.status === "failed" ? "#ff7b72" : "#a8b3c0"
                    }
                    Button {
                        objectName: "taskCancel_" + taskDelegate.requestId
                        visible: taskDelegate.status === "running"
                                 || taskDelegate.status === "cancelling"
                        enabled: taskDelegate.status === "running" && root.viewModel.canCancel
                        text: taskDelegate.status === "cancelling" ? qsTr("正在取消") : qsTr("取消")
                        Accessible.name: text
                        Accessible.description: qsTr("请求取消；等待 worker 确认后才进入已取消")
                        onClicked: root.viewModel.cancelActiveTask()
                    }
                    Button {
                        objectName: "taskRetry_" + taskDelegate.requestId
                        visible: taskDelegate.retryable
                        text: qsTr("重试")
                        Accessible.name: text
                        Accessible.description: qsTr("诊断 ID %1").arg(taskDelegate.diagnosticId)
                        onClicked: root.viewModel.retryFailedOperation()
                    }
                }
            }
        }
    }
}
