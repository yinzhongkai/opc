import QtQuick
import QtMultimedia

Item {
    id: root
    width: 64
    height: 64
    readonly property bool multimediaReady: mediaPlayer !== null

    MediaPlayer {
        id: mediaPlayer
    }

    Component.onCompleted: {
        console.log("SPACE_RHYTHM_QML_SMOKE", Qt.application.version,
                    mediaPlayer.playbackState)
    }
}
