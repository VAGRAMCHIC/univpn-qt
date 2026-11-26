import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: page

    readonly property color cardColor: "#2a2a2a"
    readonly property color cardBorder: "#3a3a3a"
    readonly property color textPrimary: "#eeeeee"
    readonly property color textSecondary: "#9e9e9e"
    readonly property color accent: "#5B7CFA"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Servers")
                font.pixelSize: 24
                font.bold: true
                color: page.textPrimary
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Add")
                flat: true
                onClicked: addSubscriptionDialog.open()
            }
        }

        // --- Подписки ---
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 220)
            clip: true
            spacing: 8
            model: App.subscriptions

            delegate: Rectangle {
                width: ListView.view.width
                color: page.cardColor
                border.color: page.cardBorder
                border.width: 1
                radius: 16
                implicitHeight: subRow.implicitHeight + 20

                RowLayout {
                    id: subRow
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: modelData.title
                            font.bold: true
                            color: page.textPrimary
                        }
                        Label {
                            text: qsTr("Nodes: %1").arg(modelData.nodeCount)
                            font.pixelSize: 12
                            color: page.textSecondary
                        }
                    }

                    Button {
                        text: qsTr("Refresh")
                        flat: true
                        onClicked: App.refreshSubscription(modelData.id)
                    }
                    Button {
                        text: qsTr("Remove")
                        flat: true
                        onClicked: {
                            removeConfirm.subId = modelData.id
                            removeConfirm.subTitle = modelData.title
                            removeConfirm.open()
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("All nodes")
            font.bold: true
            color: page.textPrimary
            visible: App.nodes.length > 0
        }

        // --- Узлы ---
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: App.nodes

            delegate: Rectangle {
                width: ListView.view.width
                radius: 12
                color: modelData.selected
                       ? Qt.rgba(0.36, 0.49, 0.98, 0.2)
                       : "transparent"
                border.color: modelData.selected ? page.accent : "transparent"
                border.width: 1
                implicitHeight: nodeRow.implicitHeight + 16

                RowLayout {
                    id: nodeRow
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Label {
                        text: modelData.selected ? "◉" : "○"
                        color: modelData.selected
                               ? page.accent
                               : page.textSecondary
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label { text: modelData.name; color: page.textPrimary }
                        Label {
                            text: modelData.endpoint
                            font.pixelSize: 11
                            color: page.textSecondary
                        }
                    }
                    Label {
                        text: modelData.protocol
                        font.pixelSize: 11
                        color: page.textSecondary
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: App.selectNode(modelData.tag)
                }
            }
        }
    }

    AddSubscriptionDialog {
        id: addSubscriptionDialog
    }

    Dialog {
        id: removeConfirm
        property string subId: ""
        property string subTitle: ""
        title: qsTr("Remove subscription?")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 360)
        standardButtons: Dialog.Cancel | Dialog.Ok

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: page.textPrimary
            text: qsTr("Subscription \"%1\" will be removed. Its nodes will disappear from the list.")
                .arg(removeConfirm.subTitle)
        }

        onAccepted: App.removeSubscription(removeConfirm.subId)
    }
  }
