import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 960
    height: 540
    visible: true
    title: qsTr("Space Rhythm")
    color: "#111827"

    Component.onCompleted: {
        if (Qt.application.arguments.indexOf("--smoke") !== -1)
            Qt.callLater(Qt.quit)
    }

    Text {
        anchors.centerIn: parent
        text: qsTr("Space Rhythm engineering shell")
        color: "#f9fafb"
    }
}
