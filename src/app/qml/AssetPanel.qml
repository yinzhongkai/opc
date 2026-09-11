pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    objectName: "assetPanel"
    required property var viewModel
    padding: 0
    Accessible.name: qsTr("素材面板")
    Accessible.description: qsTr("项目素材和媒体状态")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    text: qsTr("素材")
                    font.weight: Font.DemiBold
                    Accessible.name: text
                }
                ToolButton {
                    objectName: "assetImportButton"
                    text: qsTr("添加")
                    enabled: !root.viewModel.readOnly
                    Accessible.name: qsTr("添加素材")
                    Accessible.description: enabled ? qsTr("选择视频或音频素材") : qsTr("项目只读")
                    onClicked: root.viewModel.activateStage("import")
                }
            }
        }

        Label {
            objectName: "assetEmptyState"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: assetList.count === 0
            text: root.viewModel.workspaceState === "loading"
                  ? qsTr("正在读取项目信息…")
                  : qsTr("尚无素材\n使用“添加”导入视频或音频")
            color: "#a8b3c0"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Accessible.name: text
        }

        ListView {
            id: assetList
            objectName: "assetList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: count > 0
            clip: true
            model: root.viewModel.assets
            Accessible.name: qsTr("素材列表，共 %1 项").arg(count)
            Accessible.description: qsTr("使用方向键选择素材")

            delegate: ItemDelegate {
                id: assetDelegate
                required property string assetId
                required property string displayName
                required property string kind
                required property string assetState
                required property string durationText
                required property string detailText
                width: ListView.view.width
                objectName: "assetRow_" + assetDelegate.assetId
                text: assetDelegate.displayName
                Accessible.name: assetDelegate.displayName
                Accessible.description: assetDelegate.durationText + " · "
                                        + assetDelegate.detailText + " · "
                                        + assetDelegate.assetState

                contentItem: Column {
                    spacing: 2
                    Label { text: assetDelegate.displayName; color: "#f0f6fc" }
                    Label {
                        text: assetDelegate.durationText + " · " + assetDelegate.detailText
                        color: "#a8b3c0"
                    }
                }
            }
        }
    }
}
