import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: root
    objectName: "stageToolbar"
    required property var viewModel
    Accessible.name: qsTr("项目阶段工具栏")
    Accessible.description: qsTr("导入、分析、编辑、试听、保存和导出")

    RowLayout {
        anchors.fill: parent
        spacing: 6

        ToolButton {
            objectName: "importStageButton"
            text: qsTr("导入")
            checkable: true
            checked: root.viewModel.activeStage === "import"
            enabled: !root.viewModel.readOnly
            Accessible.name: text
            Accessible.description: enabled ? qsTr("切换到素材导入") : qsTr("只读项目不能导入素材")
            onClicked: root.viewModel.activateStage("import")
        }
        ToolButton {
            objectName: "analyzeStageButton"
            text: qsTr("分析")
            checkable: true
            checked: root.viewModel.activeStage === "analyze"
            enabled: root.viewModel.canAnalyze
            Accessible.name: text
            Accessible.description: enabled ? qsTr("启动后台分析") : qsTr("需要已就绪素材和可写项目")
            onClicked: root.viewModel.startAnalysis()
        }
        ToolButton {
            objectName: "editStageButton"
            text: qsTr("编辑")
            checkable: true
            checked: root.viewModel.activeStage === "edit"
            enabled: !root.viewModel.readOnly
            Accessible.name: text
            Accessible.description: enabled ? qsTr("切换到时间线编辑") : qsTr("项目只读")
            onClicked: root.viewModel.activateStage("edit")
        }
        ToolButton {
            objectName: "previewStageButton"
            text: qsTr("试听")
            checkable: true
            checked: root.viewModel.activeStage === "preview"
            enabled: root.viewModel.canPreview
            Accessible.name: text
            Accessible.description: enabled ? qsTr("切换到音画预览") : qsTr("需要已就绪素材")
            onClicked: root.viewModel.activateStage("preview")
        }
        ToolButton {
            objectName: "saveStageButton"
            text: qsTr("保存")
            enabled: root.viewModel.canSave
            Accessible.name: text
            Accessible.description: enabled ? qsTr("原子保存当前项目") : qsTr("没有可保存的修改或项目只读")
            onClicked: root.viewModel.saveProject()
        }
        ToolButton {
            objectName: "exportStageButton"
            text: qsTr("导出")
            enabled: root.viewModel.canExport
            Accessible.name: text
            Accessible.description: enabled ? qsTr("创建后台导出任务") : qsTr("需要已就绪项目，导出格式待确认")
            onClicked: root.viewModel.exportProject()
        }

        Item { Layout.fillWidth: true }
        Label {
            objectName: "timelineRevisionLabel"
            text: qsTr("时间线 %1").arg(root.viewModel.timelineRevisionText)
            color: "#a8b3c0"
            Accessible.name: text
        }
        Label {
            objectName: "readOnlyIndicator"
            visible: root.viewModel.readOnly
            text: qsTr("只读")
            color: "#e3b341"
            font.weight: Font.DemiBold
            Accessible.name: text
            Accessible.description: qsTr("写入动作已禁用，可试听或导出当前快照")
        }
    }
}
