import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    objectName: "timelinePanel"
    required property var viewModel
    padding: 0
    Accessible.name: qsTr("事件时间线")
    Accessible.description: qsTr("时间和修订由 C++ 提供；本任务只包含页面骨架")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    text: qsTr("时间线")
                    font.weight: Font.DemiBold
                    Accessible.name: text
                }
                ToolButton {
                    objectName: "timelineZoomOutButton"
                    text: qsTr("缩小")
                    Accessible.name: text
                    Accessible.description: qsTr("缩小可见时间范围；精确映射由 C++ 执行")
                }
                ToolButton {
                    objectName: "timelineZoomInButton"
                    text: qsTr("放大")
                    Accessible.name: text
                    Accessible.description: qsTr("放大可见时间范围；精确映射由 C++ 执行")
                }
                ToolButton {
                    objectName: "timelineSnapButton"
                    text: qsTr("吸附：网格")
                    checkable: true
                    checked: true
                    enabled: !root.viewModel.readOnly
                    Accessible.name: text
                    Accessible.description: enabled ? qsTr("切换时间线吸附") : qsTr("项目只读")
                }
            }
        }

        Rectangle {
            id: timelineSurface
            objectName: "timelineSurface"
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#11161e"
            border.color: activeFocus ? "#6ed5ff" : "#30363d"
            border.width: activeFocus ? 2 : 1
            focus: true
            Accessible.name: qsTr("时间线画布，修订 %1").arg(root.viewModel.timelineRevisionText)
            Accessible.description: qsTr("高密度事件由 C++ 批量绘制；按 F6 可离开本区域")
            Accessible.role: Accessible.Canvas

            Column {
                anchors.centerIn: parent
                spacing: 6
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("时间线交互宿主")
                    color: "#f0f6fc"
                    Accessible.name: text
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("不在 QML 中保存 TimeNs、revision 或 frame index")
                    color: "#a8b3c0"
                    Accessible.name: text
                }
            }
        }
    }
}
