import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Frame {
    id: root
    objectName: "inspectorPanel"
    required property var viewModel
    padding: 0
    Accessible.name: qsTr("检查器")
    Accessible.description: qsTr("事件、模板和导出参数")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabs
            objectName: "inspectorTabs"
            Layout.fillWidth: true
            Accessible.name: qsTr("检查器类别")
            TabButton { objectName: "eventInspectorTab"; text: qsTr("事件"); Accessible.name: text }
            TabButton { objectName: "templateInspectorTab"; text: qsTr("模板"); Accessible.name: text }
            TabButton { objectName: "exportInspectorTab"; text: qsTr("导出"); Accessible.name: text }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ColumnLayout {
                spacing: 10
                Label {
                    objectName: "audioMappingStatus"
                    Layout.fillWidth: true
                    Layout.margins: 12
                    text: root.viewModel.audioMappingStatusText
                    color: root.viewModel.developmentAudioMappingEnabled
                           ? "#7ee787" : "#e3b341"
                    wrapMode: Text.WordWrap
                    Accessible.name: text
                }
                Button {
                    objectName: "enableDevelopmentAudioMappingButton"
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    text: root.viewModel.developmentAudioMappingEnabled
                          ? qsTr("CC0 开发音色映射已启用")
                          : qsTr("启用 CC0 开发音色映射")
                    enabled: !root.viewModel.developmentAudioMappingEnabled
                    Accessible.name: text
                    Accessible.description: qsTr("显式使用 A-018 测试音色，仅用于开发试听与测试导出，不代表产品默认音色")
                    onClicked: root.viewModel.enableDevelopmentAudioMapping()
                }
                Label {
                    objectName: "selectedEventSummary"
                    Layout.fillWidth: true
                    Layout.margins: 12
                    text: root.viewModel.selectedEventText
                    color: "#f0f6fc"
                    wrapMode: Text.WordWrap
                    Accessible.name: qsTr("选中事件 %1").arg(text)
                }
                Button {
                    objectName: "eventLockButton"
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    text: root.viewModel.selectedEventLocked ? qsTr("解除锁定") : qsTr("锁定事件")
                    enabled: root.viewModel.canEditTimeline
                             && root.viewModel.selectedEventText !== qsTr("未选择事件")
                    Accessible.name: text
                    Accessible.description: enabled
                        ? qsTr("锁定事件后，拖动与批量偏移将由 C++ 核心拒绝")
                        : qsTr("请选择可编辑事件")
                    onClicked: root.viewModel.toggleSelectedEventLock()
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    TextField {
                        id: batchOffsetField
                        objectName: "batchOffsetMillisecondsField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("偏移毫秒，例如 120")
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: IntValidator { bottom: -3600000; top: 3600000 }
                        Accessible.name: qsTr("批量偏移毫秒")
                        Accessible.description: qsTr("输入由 C++ 转换为 TimeNs 并校验")
                    }
                    Button {
                        objectName: "batchOffsetButton"
                        text: qsTr("应用")
                        enabled: root.viewModel.canEditTimeline
                                 && batchOffsetField.acceptableInput
                        Accessible.name: qsTr("应用批量偏移")
                        Accessible.description: qsTr("通过核心 BatchOffsetEvents 事务提交")
                        onClicked: root.viewModel.batchOffsetSelected(batchOffsetField.text)
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    text: root.viewModel.readOnly
                          ? qsTr("只读：可检查事件，不能提交修改")
                          : qsTr("时间值仅以格式化文本显示；权威值保留在 C++。")
                    color: "#a8b3c0"
                    wrapMode: Text.WordWrap
                    Accessible.name: text
                }
                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                spacing: 8
                ComboBox {
                    objectName: "visualTemplateSelector"
                    Layout.fillWidth: true
                    Layout.margins: 12
                    model: [qsTr("波形/示波器 1.0.0"), qsTr("频谱几何 1.0.0"), qsTr("节奏线条脉冲 1.0.0")]
                    enabled: !root.viewModel.readOnly
                    Accessible.name: qsTr("视觉模板")
                    Accessible.description: qsTr("当前值为工程模板，不代表产品默认风格")
                }
                ListView {
                    id: parameterList
                    objectName: "templateParameterList"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 8
                    clip: true
                    model: root.viewModel.templateParameters
                    Accessible.name: qsTr("模板参数，共 %1 项").arg(count)
                    Accessible.description: qsTr("数值以字符串展示并由 C++ 验证")

                    delegate: ItemDelegate {
                        required property string name
                        required property string label
                        required property string unit
                        required property string minimumText
                        required property string maximumText
                        required property string engineeringDefaultText
                        required property string valueText
                        required property bool advanced
                        width: ListView.view.width
                        objectName: "templateParameter_" + name
                        text: label + ": " + valueText + (unit.length > 0 ? " " + unit : "")
                        Accessible.name: text
                        Accessible.description: qsTr("范围 %1 到 %2，工程默认 %3%4")
                            .arg(minimumText)
                            .arg(maximumText)
                            .arg(engineeringDefaultText)
                            .arg(advanced ? qsTr("，高级参数") : "")
                    }
                }
            }

            ColumnLayout {
                spacing: 10
                Label {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    text: qsTr("导出容器、编码器和默认扩展名待确认。")
                    color: "#e3b341"
                    wrapMode: Text.WordWrap
                    Accessible.name: text
                }
                Button {
                    objectName: "inspectorExportButton"
                    Layout.margins: 12
                    text: qsTr("创建导出任务")
                    enabled: root.viewModel.canExport
                    Accessible.name: text
                    Accessible.description: enabled ? qsTr("按当前快照创建后台任务") : qsTr("导出当前不可用")
                    onClicked: inspectorExportDialog.open()
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    FileDialog {
        id: inspectorExportDialog
        objectName: "inspectorExportDialog"
        title: qsTr("导出冻结快照（开发格式）")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "nut"
        nameFilters: [qsTr("T-019 测试导出 (*.nut)")]
        onAccepted: root.viewModel.exportProjectTo(selectedFile.toString())
    }
}
