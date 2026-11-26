import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: page

    // Локальная палитра (Qt 6.5-совместимая)
    readonly property color cardColor: "#2a2a2a"
    readonly property color cardBorder: "#3a3a3a"
    readonly property color textPrimary: "#eeeeee"
    readonly property color textSecondary: "#9e9e9e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Label {
            text: qsTr("Settings")
            font.pixelSize: 24
            font.bold: true
            color: page.textPrimary
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: page.width - 40
                spacing: 12

                // --- Routing mode ---
                Rectangle {
                    Layout.fillWidth: true
                    color: page.cardColor
                    border.color: page.cardBorder
                    border.width: 1
                    radius: 16
                    implicitHeight: routingRow.implicitHeight + 24

                    ColumnLayout {
                        id: routingRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: qsTr("Routing")
                            font.bold: true
                            color: page.textPrimary
                        }
                        Label {
                            text: qsTr("What goes through the VPN")
                            font.pixelSize: 12
                            color: page.textSecondary
                        }

                        ComboBox {
                            Layout.fillWidth: true
                            model: [
                                qsTr("Smart (RU direct)"),
                                qsTr("All traffic"),
                                qsTr("Whitelist only")
                            ]
                            currentIndex: App.routingMode
                            onActivated: App.setRoutingMode(index)
                        }
                    }
                }

                // --- Language ---
                Rectangle {
                    Layout.fillWidth: true
                    color: page.cardColor
                    border.color: page.cardBorder
                    border.width: 1
                    radius: 16
                    implicitHeight: langRow.implicitHeight + 24

                    ColumnLayout {
                        id: langRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: qsTr("Language")
                            font.bold: true
                            color: page.textPrimary
                        }

                        ComboBox {
                            Layout.fillWidth: true
                            model: [qsTr("System"), "English", "Русский"]
                            currentIndex: {
                                if (App.localeCode === "ru") return 2
                                if (App.localeCode === "en") return 1
                                return 0
                            }
                            onActivated: {
                                if (index === 0) App.setLocaleCode("")
                                else if (index === 1) App.setLocaleCode("en")
                                else App.setLocaleCode("ru")
                            }
                        }
                    }
                }

                // --- Auto-connect ---
                Rectangle {
                    Layout.fillWidth: true
                    color: page.cardColor
                    border.color: page.cardBorder
                    border.width: 1
                    radius: 16
                    implicitHeight: autoConnectRow.implicitHeight + 24

                    RowLayout {
                        id: autoConnectRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: qsTr("Connect on launch")
                                font.bold: true
                                color: page.textPrimary
                            }
                            Label {
                                text: qsTr("When the app opens")
                                font.pixelSize: 12
                                color: page.textSecondary
                            }
                        }

                        Switch {
                            checked: App.autoConnect
                            onToggled: App.setAutoConnect(checked)
                        }
                    }
                }

                // --- Kill switch ---
                Rectangle {
                    Layout.fillWidth: true
                    color: page.cardColor
                    border.color: page.cardBorder
                    border.width: 1
                    radius: 16
                    implicitHeight: killSwitchRow.implicitHeight + 24

                    RowLayout {
                        id: killSwitchRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: qsTr("Kill switch")
                                font.bold: true
                                color: page.textPrimary
                            }
                            Label {
                                text: qsTr("Block traffic outside the VPN (TUN mode only)")
                                font.pixelSize: 12
                                color: page.textSecondary
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Switch {
                            checked: App.killSwitch
                            onToggled: App.setKillSwitch(checked)
                        }
                    }
                }

                // --- Logs ---
                Rectangle {
                    Layout.fillWidth: true
                    color: page.cardColor
                    border.color: page.cardBorder
                    border.width: 1
                    radius: 16
                    implicitHeight: logsRow.implicitHeight + 24

                    RowLayout {
                        id: logsRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: qsTr("Core log")
                                font.bold: true
                                color: page.textPrimary
                            }
                            Label {
                                text: qsTr("sing-box output for diagnostics")
                                font.pixelSize: 12
                                color: page.textSecondary
                            }
                        }

                        Button {
                            text: "›"
                            flat: true
                            onClicked: logsDialog.open()
                        }
                    }
                }
            }
        }
    }

    LogsDialog {
        id: logsDialog
    }
  }
