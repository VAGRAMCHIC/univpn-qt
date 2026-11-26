import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: dialog
    title: qsTr("Diagnostics")
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width - 32 : 600, 720)
    height: Math.min(parent ? parent.height - 64 : 600, 640)
    standardButtons: Dialog.Close

    property var logLines: App.logs()

    onOpened: logLines = App.logs()

    Connections {
        target: App
        function onLogsChanged() { dialog.logLines = App.logs() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("sing-box output")
                font.bold: true
                color: "#eeeeee"
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Copy")
                flat: true
                onClicked: {
                    logView.selectAll()
                    logView.copy()
                }
            }
            Button {
                text: qsTr("Clear")
                flat: true
                onClicked: App.clearLogs()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextArea {
                id: logView
                readOnly: true
                selectByMouse: true
                wrapMode: TextArea.NoWrap
                font.family: "monospace"
                font.pixelSize: 11
                color: "#eeeeee"
                background: Rectangle { color: "#1e1e1e" }
                text: dialog.logLines.length === 0
                      ? qsTr("Log is empty. Connect to see core output.")
                      : dialog.logLines.join("\n")
            }
        }
    }
  }
