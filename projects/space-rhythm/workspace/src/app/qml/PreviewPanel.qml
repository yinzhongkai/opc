import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    objectName: "previewPanel"
    required property var viewModel
    padding: 0
    Accessible.name: qsTr("视觉和音频预览")
    Accessible.description: qsTr("由 C++ 场景图渲染并使用格式化时间显示")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#070a0e"

            Item {
                id: sceneGraphHost
                objectName: "previewSceneGraphHost"
                anchors.fill: parent
                anchors.margins: 12
                Accessible.name: qsTr("场景图视觉预览")
                Accessible.description: qsTr("几何由 C++ SceneGraphRenderItem 生成；当前时间 %1")
                    .arg(root.viewModel.previewTimeText)
                Accessible.role: Accessible.Graphic
            }

            Label {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 8
                z: 2
                text: qsTr("C++ SceneGraph · %1").arg(root.viewModel.timelineRevisionText)
                color: "#a8b3c0"
                Accessible.name: text
            }
        }

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Item { Layout.fillWidth: true }
                ToolButton {
                    objectName: "previewPlayPauseButton"
                    text: root.viewModel.previewState === "playing" ? qsTr("暂停") : qsTr("播放")
                    enabled: root.viewModel.canPreview
                    Accessible.name: text
                    Accessible.description: enabled ? qsTr("播放或暂停同修订预览") : qsTr("需要已就绪素材")
                    onClicked: root.viewModel.togglePreview()
                }
                ToolButton {
                    objectName: "previewStopButton"
                    text: qsTr("停止")
                    enabled: root.viewModel.previewState !== "stopped"
                    Accessible.name: text
                    onClicked: root.viewModel.stopPreview()
                }
                Label {
                    objectName: "previewTimeLabel"
                    text: root.viewModel.previewTimeText
                    color: "#f0f6fc"
                    font.family: "Consolas"
                    Accessible.name: qsTr("预览时间 %1").arg(text)
                }
                Item { Layout.fillWidth: true }
            }
        }
    }
}
