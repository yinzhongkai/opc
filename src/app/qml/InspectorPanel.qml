import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

            Label {
                text: root.viewModel.readOnly
                      ? qsTr("只读：可检查选中事件，不能提交修改")
                      : qsTr("选择时间线事件后在此编辑精确属性")
                color: "#a8b3c0"
                wrapMode: Text.WordWrap
                padding: 12
                Accessible.name: text
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
                    onClicked: root.viewModel.exportProject()
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
