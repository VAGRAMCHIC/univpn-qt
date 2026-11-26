import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: dialog
    title: qsTr("New subscription")
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width - 32 : 360, 480)
    standardButtons: Dialog.Cancel | Dialog.Ok

    property bool busy: false

    onAccepted: {
        if (urlField.text.trim() === "") return
        App.addSubscription(urlField.text.trim())
        urlField.text = ""
    }
    onOpened: urlField.forceActiveFocus()

    ColumnLayout {
        width: parent.width
        spacing: 8

        TextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "https://panel.example.com/sub/…"
            selectByMouse: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Paste a subscription URL. Supports vless, trojan, shadowsocks.")
            wrapMode: Text.WordWrap
            font.pixelSize: 11
            color: "#9e9e9e"
        }
    }
  }
