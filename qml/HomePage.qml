import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: page

    signal navigateTo(int index)

    readonly property int status: App.status
    readonly property bool connected: status === 2
    readonly property bool busy: status === 1 || status === 3

    // Qt 6.5-совместимая палитра
    readonly property color cardColor: "#2a2a2a"
    readonly property color cardBorder: "#3a3a3a"
    readonly property color textPrimary: "#eeeeee"
    readonly property color textSecondary: "#9e9e9e"
    readonly property color accent: "#5B7CFA"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("clgvpn")
                font.pixelSize: 24
                font.bold: true
                color: page.textPrimary
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Details")
                flat: true
                onClicked: detailsDialog.open()
            }
            Rectangle {
                color: page.connected ? Qt.rgba(0.36, 0.49, 0.98, 0.2)
                                      : page.cardColor
                border.color: page.cardBorder
                border.width: 1
                radius: 999
                implicitWidth: statusLabel.implicitWidth + 24
                implicitHeight: 28
                Label {
                    id: statusLabel
                    anchors.centerIn: parent
                    text: App.statusText
                    font.pixelSize: 12
                    color: page.textPrimary
                }
            }
        }

        Item { Layout.fillHeight: true }
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 208
            Layout.preferredHeight: 208

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: page.connected ? page.accent : "#333333"
                    }
                    GradientStop {
                        position: 1.0
                        color: page.connected ? "#7a5cfa" : "#252525"
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.color: page.accent
                    border.width: page.connected ? 4 : 0
                    opacity: page.connected ? 0.4 : 0
                    Behavior on opacity { NumberAnimation { duration: 300 } }
                }

                Label {
                    anchors.centerIn: parent
                    text: "⏻"
                    font.pixelSize: 72
                    color: page.connected ? "#ffffff" : page.textSecondary
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !page.busy
                    onClicked: App.toggleConnection()
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: page.connected ? qsTr("Disconnect") : qsTr("Connect")
            font.pixelSize: 18
            color: page.textPrimary
        }
        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            color: page.cardColor
            border.color: page.cardBorder
            border.width: 1
            radius: 20
            implicitHeight: serverRow.implicitHeight + 24

            RowLayout {
                id: serverRow
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Label { text: "🖥"; font.pixelSize: 22; color: page.textPrimary }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: App.selectedNodeName !== ""
                              ? App.selectedNodeName
                              : qsTr("No server selected")
                        font.bold: true
                        color: page.textPrimary
                    }
                    Label {
                        text: App.serverName !== ""
                              ? App.serverName
                              : qsTr("Pick a server in the Servers tab")
                        font.pixelSize: 12
                        color: page.textSecondary
                    }
                }

                Button {
                    text: "›"
                    flat: true
                    onClicked: page.navigateTo(1)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                color: page.cardColor
                border.color: page.cardBorder
                border.width: 1
                radius: 20
                implicitHeight: 60
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8
                    Label { text: "↓"; font.pixelSize: 18; color: page.textSecondary }
                    ColumnLayout {
                        spacing: 0
                        Label {
                            text: qsTr("Download")
                            font.pixelSize: 11
                            color: page.textSecondary
                        }
                        Label {
                            text: App.formatBytes(App.downloadedBytes)
                            font.bold: true
                            color: page.textPrimary
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                color: page.cardColor
                border.color: page.cardBorder
                border.width: 1
                radius: 20
                implicitHeight: 60
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8
                    Label { text: "↑"; font.pixelSize: 18; color: page.textSecondary }
                    ColumnLayout {
                        spacing: 0
                        Label {
                            text: qsTr("Upload")
                            font.pixelSize: 11
                            color: page.textSecondary
                        }
                        Label {
                            text: App.formatBytes(App.uploadedBytes)
                            font.bold: true
                            color: page.textPrimary
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: App.hint !== ""
            text: App.hint
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: page.textSecondary
        }
    }

    Dialog {
        id: detailsDialog
        title: qsTr("Connection details")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 480)
        standardButtons: Dialog.Close

        ColumnLayout {
            spacing: 8
            width: parent.width

            Label { text: qsTr("Node: %1").arg(App.serverName || "—"); color: page.textPrimary }
            Label { text: qsTr("Mode: %1").arg(App.modeText || "—"); color: page.textPrimary }
            Label { text: qsTr("Uptime: %1").arg(App.uptimeText || "—"); color: page.textPrimary }
            Label {
                visible: App.killSwitchActive
                text: qsTr("Kill switch: active")
                color: page.textPrimary
            }
            Label {
                text: qsTr("Traffic: ↓ %1  ↑ %2")
                    .arg(App.formatBytes(App.downloadedBytes))
                    .arg(App.formatBytes(App.uploadedBytes))
                color: page.textPrimary
            }
        }
    }
  }
