import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "UI Modules"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#ffffff"
                }

                Text {
                    text: "Available UI plugins in the system"
                    font.pixelSize: 14
                    color: "#a0a0a0"
                }
            }

            Button {
                text: "Install from Filesystem"
                onClicked: backend.openInstallPluginDialog()

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 13
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 180
                    implicitHeight: 32
                    color: parent.pressed ? "#1a7f37" : "#238636"
                    radius: 4
                    border.color: "#2ea043"
                    border.width: 1
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    model: backend.uiModules

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        color: "#2d2d2d"
                        radius: 8
                        border.color: "#3d3d3d"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            // Icon
                            Rectangle {
                                width: 48
                                height: 48
                                radius: 8
                                color: "#3d3d3d"

                                Image {
                                    anchors.centerIn: parent
                                    source: modelData.iconPath || ""
                                    sourceSize.width: 32
                                    sourceSize.height: 32
                                    visible: modelData.iconPath && modelData.iconPath.length > 0
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.name.substring(0, 2).toUpperCase()
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#808080"
                                    visible: !modelData.iconPath || modelData.iconPath.length === 0
                                }
                            }

                            // Name
                            Text {
                                text: modelData.name
                                font.pixelSize: 16
                                font.bold: true
                                color: "#ffffff"
                                Layout.fillWidth: true
                            }

                            // Load/Unload buttons (hidden for main_ui)
                            Button {
                                text: "Load"
                                visible: !modelData.isMainUi && !modelData.isLoaded

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 13
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    implicitWidth: 80
                                    implicitHeight: 32
                                    color: parent.pressed ? "#45a049" : "#4CAF50"
                                    radius: 4
                                }

                                onClicked: backend.loadUiModule(modelData.name)
                            }

                            Button {
                                text: "Unload"
                                visible: !modelData.isMainUi && modelData.isLoaded

                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 13
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    implicitWidth: 80
                                    implicitHeight: 32
                                    color: parent.pressed ? "#da190b" : "#f44336"
                                    radius: 4
                                }

                                onClicked: backend.unloadUiModule(modelData.name)
                            }
                        }
                    }
                }

                // Empty state
                Text {
                    text: "No UI plugins found in the plugins directory."
                    font.pixelSize: 14
                    color: "#606060"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 40
                    visible: backend.uiModules.length === 0
                }
            }
        }
    }
}



