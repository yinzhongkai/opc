import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    objectName: "timelinePanel"
    required property var viewModel
    padding: 0
    Accessible.name: qsTr("事件时间线")
    Accessible.description: qsTr("C++ 权威时间线；支持缩放、平移、拖动、锁定和历史操作")

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
                    id: undoButton
                    objectName: "timelineUndoButton"
                    text: qsTr("撤销")
                    enabled: root.viewModel.canUndo
                    Accessible.name: text
                    Accessible.description: qsTr("撤销最近一次 C++ 时间线事务")
                    onClicked: root.viewModel.undo()
                    KeyNavigation.tab: redoButton
                }
                ToolButton {
                    id: redoButton
                    objectName: "timelineRedoButton"
                    text: qsTr("重做")
                    enabled: root.viewModel.canRedo
                    Accessible.name: text
                    Accessible.description: qsTr("重做最近撤销的 C++ 时间线事务")
                    onClicked: root.viewModel.redo()
                }
                ToolButton {
                    objectName: "timelineZoomOutButton"
                    text: qsTr("缩小")
                    Accessible.name: text
                    Accessible.description: qsTr("缩小可见时间范围；精确映射由 C++ 执行")
                    onClicked: root.viewModel.zoomTimeline(-1)
                }
                ToolButton {
                    objectName: "timelineZoomInButton"
                    text: qsTr("放大")
                    Accessible.name: text
                    Accessible.description: qsTr("放大可见时间范围；精确映射由 C++ 执行")
                    onClicked: root.viewModel.zoomTimeline(1)
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
            KeyNavigation.tab: undoButton
            Accessible.name: qsTr("时间线画布，修订 %1").arg(root.viewModel.timelineRevisionText)
            Accessible.description: qsTr("高密度事件由 C++ 批量绘制；按 F6 可离开本区域")
            Accessible.role: Accessible.Canvas

            Item {
                id: timelineSceneGraphHost
                objectName: "timelineSceneGraphHost"
                anchors.fill: parent
                anchors.margins: 2
                Accessible.name: qsTr("C++ 批量时间线几何")
                Accessible.description: qsTr("可见范围 %1").arg(root.viewModel.viewportText)
                Accessible.role: Accessible.Graphic
            }

            Label {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 8
                z: 3
                text: root.viewModel.viewportText
                color: "#a8b3c0"
                font.family: "Consolas"
                Accessible.name: qsTr("可见范围 %1").arg(text)
            }

            MouseArea {
                id: timelinePointer
                objectName: "timelinePointerArea"
                anchors.fill: parent
                z: 4
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                hoverEnabled: true
                property real previousX: 0

                onPressed: function(mouse) {
                    previousX = mouse.x
                    if (mouse.button === Qt.LeftButton
                            && !(mouse.modifiers & Qt.ControlModifier))
                        root.viewModel.beginTimelineDrag(mouse.x, width)
                }
                onPositionChanged: function(mouse) {
                    if (pressedButtons & Qt.LeftButton)
                        root.viewModel.updateTimelineDrag(mouse.x, width)
                    else if (pressedButtons & (Qt.MiddleButton | Qt.RightButton)) {
                        root.viewModel.panTimeline(mouse.x - previousX, width)
                        previousX = mouse.x
                    }
                }
                onReleased: function(mouse) {
                    if (mouse.button === Qt.LeftButton)
                        root.viewModel.endTimelineDrag()
                }
                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        if (mouse.modifiers & Qt.ControlModifier)
                            root.viewModel.toggleTimelineEventSelection(mouse.x, width)
                        else
                            root.viewModel.selectTimelineEvent(mouse.x, width)
                    }
                }
                onDoubleClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        if (mouse.modifiers & Qt.ShiftModifier)
                            root.viewModel.addTimelineEvent(mouse.x, width)
                        else
                            root.viewModel.seekTimeline(mouse.x, width)
                    }
                }
                onWheel: function(wheel) {
                    root.viewModel.zoomTimeline(wheel.angleDelta.y >= 0 ? 1 : -1)
                    wheel.accepted = true
                }
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
                    root.viewModel.zoomTimeline(1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Minus) {
                    root.viewModel.zoomTimeline(-1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Left) {
                    root.viewModel.panTimeline(40, width)
                    event.accepted = true
                } else if (event.key === Qt.Key_Right) {
                    root.viewModel.panTimeline(-40, width)
                    event.accepted = true
                }
            }
        }
    }
}
