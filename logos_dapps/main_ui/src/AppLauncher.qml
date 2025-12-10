import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    height: 90
    color: "#2D2D2D"
    
    // Top border
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#3D3D3D"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 20

        Item { Layout.fillWidth: true }

        Repeater {
            model: backend.launcherApps

            Item {
                width: 64
                height: 74

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    // App icon button
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64
                        height: 64
                        radius: 12
                        color: iconMouseArea.containsMouse ? "#4D4D4D" : "#3D3D3D"

                        Image {
                            anchors.centerIn: parent
                            source: modelData.iconPath || ""
                            sourceSize.width: 48
                            sourceSize.height: 48
                            fillMode: Image.PreserveAspectFit
                            visible: modelData.iconPath && modelData.iconPath.length > 0
                        }

                        Text {
                            anchors.centerIn: parent
                            text: modelData.name.substring(0, 4)
                            font.pixelSize: 14
                            font.bold: true
                            color: "#808080"
                            visible: !modelData.iconPath || modelData.iconPath.length === 0
                        }

                        MouseArea {
                            id: iconMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.onAppLauncherClicked(modelData.name)
                        }
                    }

                    // Dot indicator (visible when app is loaded)
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 6
                        height: 6
                        radius: 3
                        color: modelData.isLoaded ? "#A0A0A0" : "transparent"
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }
    }

    // Empty state message
    Text {
        anchors.centerIn: parent
        text: "No apps available"
        font.pixelSize: 14
        color: "#606060"
        visible: backend.launcherApps.length === 0
    }
}



