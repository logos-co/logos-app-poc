import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 20

        Text {
            text: "Package Manager"
            font.pixelSize: 24
            font.bold: true
            color: "#ffffff"
        }

        Text {
            text: "Manage plugins and packages"
            font.pixelSize: 14
            color: "#a0a0a0"
        }

        RowLayout {
            spacing: 10

            Button {
                text: "Reload"
                enabled: !backend.isInstalling
                onClicked: backend.reload()

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 13
                    color: parent.enabled ? "#ffffff" : "#808080"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: parent.enabled ? (parent.pressed ? "#3d3d3d" : "#4d4d4d") : "#2d2d2d"
                    radius: 4
                    border.color: parent.enabled ? "#5d5d5d" : "#3d3d3d"
                    border.width: 1
                }
            }

            Button {
                text: "Test Call"
                enabled: !backend.isInstalling
                onClicked: backend.testPluginCall()

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 13
                    color: parent.enabled ? "#ffffff" : "#808080"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: parent.enabled ? (parent.pressed ? "#3d3d3d" : "#4d4d4d") : "#2d2d2d"
                    radius: 4
                    border.color: parent.enabled ? "#5d5d5d" : "#3d3d3d"
                    border.width: 1
                }
            }

            Button {
                text: backend.isInstalling ? "Installing..." : "Install"
                enabled: backend.hasSelectedPackages && !backend.isInstalling
                onClicked: backend.install()

                contentItem: Row {
                    spacing: 6
                    anchors.centerIn: parent
                    
                    Rectangle {
                        id: spinner
                        width: 14
                        height: 14
                        radius: 7
                        color: "transparent"
                        border.color: "#ffffff"
                        border.width: 2
                        visible: backend.isInstalling
                        
                        Rectangle {
                            width: 4
                            height: 4
                            radius: 2
                            color: "#ffffff"
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 1
                        }
                        
                        RotationAnimation on rotation {
                            from: 0
                            to: 360
                            duration: 1000
                            loops: Animation.Infinite
                            running: backend.isInstalling
                        }
                    }
                    
                    Text {
                        text: backend.isInstalling ? "Installing..." : "Install"
                        font.pixelSize: 13
                        color: parent.parent.enabled ? "#ffffff" : "#808080"
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                background: Rectangle {
                    implicitWidth: backend.isInstalling ? 130 : 100
                    implicitHeight: 32
                    color: parent.enabled ? (parent.pressed ? "#1a7f37" : "#238636") : "#2d2d2d"
                    radius: 4
                    border.color: parent.enabled ? "#2ea043" : "#3d3d3d"
                    border.width: 1
                    
                    Behavior on implicitWidth {
                        NumberAnimation { duration: 150 }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                SplitView.preferredWidth: 180
                SplitView.minimumWidth: 150
                SplitView.maximumWidth: 250
                color: "#2d2d2d"

                ListView {
                    id: categoryList
                    anchors.fill: parent
                    model: backend.categories
                    currentIndex: backend.selectedCategoryIndex

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 40
                        color: ListView.isCurrentItem ? "#3d3d3d" : (mouseArea.containsMouse ? "#353535" : "transparent")
                        radius: 3

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            text: modelData
                            color: "#ffffff"
                            font.pixelSize: 14
                            verticalAlignment: Text.AlignVCenter
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: backend.selectedCategoryIndex = index
                        }
                    }
                }
            }

            ColumnLayout {
                SplitView.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: parent.height * 0.6
                    color: "#333333"
                    border.color: "#000000"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 35
                            color: "#333333"

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "S"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 150
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Package"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 120
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Installed Ver"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 120
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Latest Ver"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 80
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Type"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "transparent"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Description"
                                        color: "#a0a0a0"
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#444444"
                        }

                        ListView {
                            id: packageList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: backend.packages

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 35
                                color: index % 2 === 0 ? "#333333" : "#2a2a2a"

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    Rectangle {
                                        Layout.preferredWidth: 40
                                        Layout.fillHeight: true
                                        color: "transparent"

                                        CheckBox {
                                            anchors.centerIn: parent
                                            checked: modelData.isSelected
                                            onCheckedChanged: {
                                                if (checked !== modelData.isSelected) {
                                                    backend.togglePackage(index, checked)
                                                }
                                            }

                                            indicator: Rectangle {
                                                implicitWidth: 18
                                                implicitHeight: 18
                                                x: parent.leftPadding
                                                y: parent.height / 2 - height / 2
                                                radius: 3
                                                color: parent.checked ? "#4A90E2" : "#2d2d2d"
                                                border.color: parent.checked ? "#4A90E2" : "#5d5d5d"

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "✓"
                                                    color: "#ffffff"
                                                    font.pixelSize: 12
                                                    visible: parent.parent.checked
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 150
                                        Layout.fillHeight: true
                                        color: "transparent"
                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: modelData.name
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            width: parent.width - 15
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 120
                                        Layout.fillHeight: true
                                        color: "transparent"
                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: modelData.installedVersion
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            width: parent.width - 15
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 120
                                        Layout.fillHeight: true
                                        color: "transparent"
                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: modelData.latestVersion
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            width: parent.width - 15
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 80
                                        Layout.fillHeight: true
                                        color: "transparent"
                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: modelData.type
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            width: parent.width - 15
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "transparent"
                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.right: parent.right
                                            anchors.rightMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: modelData.description
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: backend.selectPackage(index)
                                    z: -1
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#333333"
                    border.color: "#444444"
                    border.width: 1

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10

                        TextArea {
                            id: detailsText
                            textFormat: Text.RichText
                            text: backend.detailsHtml
                            color: "#ffffff"
                            readOnly: true
                            wrapMode: Text.Wrap
                            background: Rectangle {
                                color: "transparent"
                            }
                            font.pixelSize: 13
                        }
                    }
                }
            }
        }
    }
}
