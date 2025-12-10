import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    property string pluginName: ""
    property var methods: []
    property string resultText: ""
    
    signal backClicked()
    
    Component.onCompleted: loadMethods()
    onPluginNameChanged: loadMethods()
    
    function loadMethods() {
        if (pluginName.length > 0) {
            let methodsJson = backend.getCoreModuleMethods(pluginName)
            try {
                methods = JSON.parse(methodsJson)
            } catch (e) {
                methods = []
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        // Header with back button
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                text: "← Back"
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 32
                    color: parent.pressed ? "#3d3d3d" : "#4b4b4b"
                    radius: 4
                }

                onClicked: root.backClicked()
            }

            Text {
                text: "Methods: " + root.pluginName
                font.pixelSize: 20
                font.bold: true
                color: "#ffffff"
            }

            Item { Layout.fillWidth: true }
        }

        // Methods list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#2d2d2d"
            radius: 8
            border.color: "#3d3d3d"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 8

                        Repeater {
                            model: root.methods

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: methodColumn.implicitHeight + 24
                                color: "#363636"
                                radius: 6

                                ColumnLayout {
                                    id: methodColumn
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 8

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 12

                                        Text {
                                            text: modelData.name || modelData
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#4A90E2"
                                        }

                                        Text {
                                            text: modelData.signature || ""
                                            font.pixelSize: 12
                                            color: "#808080"
                                            visible: modelData.signature !== undefined
                                        }

                                        Item { Layout.fillWidth: true }

                                        Button {
                                            text: "Call"
                                            
                                            contentItem: Text {
                                                text: parent.text
                                                font.pixelSize: 12
                                                color: "#ffffff"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }

                                            background: Rectangle {
                                                implicitWidth: 60
                                                implicitHeight: 26
                                                color: parent.pressed ? "#45a049" : "#4CAF50"
                                                radius: 4
                                            }

                                            onClicked: {
                                                let methodName = modelData.name || modelData
                                                root.resultText = backend.callCoreModuleMethod(root.pluginName, methodName, "[]")
                                            }
                                        }
                                    }

                                    Text {
                                        text: modelData.description || ""
                                        font.pixelSize: 12
                                        color: "#a0a0a0"
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                        visible: modelData.description !== undefined && modelData.description.length > 0
                                    }
                                }
                            }
                        }

                        // Empty state
                        Text {
                            text: "No methods available for this plugin."
                            font.pixelSize: 14
                            color: "#606060"
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 40
                            visible: root.methods.length === 0
                        }
                    }
                }

                // Result area
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    color: "#1e1e1e"
                    radius: 4
                    border.color: "#4d4d4d"
                    border.width: 1
                    visible: root.resultText.length > 0

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "Result:"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#a0a0a0"
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                text: root.resultText
                                font.pixelSize: 12
                                font.family: "monospace"
                                color: "#4CAF50"
                                readOnly: true
                                wrapMode: Text.Wrap
                                background: Rectangle {
                                    color: "transparent"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}



