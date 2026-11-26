import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 420
    height: 820
    minimumWidth: 360
    minimumHeight: 640
    visible: true
    title: qsTr("clgvpn")

    Material.theme: Material.Dark
    Material.primary: "#5B7CFA"
    Material.accent: "#5B7CFA"

    property int currentIndex: 0

    Connections {
        target: App
        function onErrorOccurred(message) {
            errorSnack.text = message
            errorSnack.open()
        }
        function onSubscriptionAdded(title, nodeCount) {
            errorSnack.text = qsTr("Added: %1 (%2 nodes)").arg(title).arg(nodeCount)
            errorSnack.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentIndex

            HomePage {
                onNavigateTo: (index) => root.currentIndex = index
            }
            ServersPage {}
            SettingsPage {}
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: root.currentIndex
            onCurrentIndexChanged: root.currentIndex = currentIndex

            TabButton {
                text: qsTr("Connect")
                icon.source: ""
            }
            TabButton {
                text: qsTr("Servers")
            }
            TabButton {
                text: qsTr("Settings")
            }
        }
    }

    Snackbar {
        id: errorSnack
    }
  }
