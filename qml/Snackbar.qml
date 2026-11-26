import QtQuick
import QtQuick.Controls

Dialog {
    id: snack
    property alias text: label.text
    property int duration: 4000

    modal: false
    closePolicy: Dialog.NoAutoClose
    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: parent.height - height - 24
    width: Math.min(parent.width - 32, 480)

    background: Rectangle {
        color: "#2a2a2a"
        radius: 8
        border.color: "#3a3a3a"
    }

    contentItem: Label {
        id: label
        padding: 12
        wrapMode: Text.WordWrap
        color: "#eeeeee"
    }

    Timer {
        id: autoClose
        interval: snack.duration
        onTriggered: snack.close()
    }

    onOpened: autoClose.restart()
  }
