import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 390
    height: 844
    visible: true
    title: "Logos Demo"
    
    color: "#0a0a0f"
    
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0a0a0f" }
            GradientStop { position: 0.4; color: "#0d1117" }
            GradientStop { position: 1.0; color: "#161b22" }
        }
        
        // Decorative background circles
        Rectangle {
            width: 300
            height: 300
            radius: 150
            color: "transparent"
            border.color: "#1a2332"
            border.width: 1
            opacity: 0.5
            x: -100
            y: -50
        }
        
        Rectangle {
            width: 400
            height: 400
            radius: 200
            color: "transparent"
            border.color: "#1a2332"
            border.width: 1
            opacity: 0.3
            x: parent.width - 200
            y: parent.height - 250
        }
        
        Flickable {
            anchors.fill: parent
            contentHeight: mainColumn.height + 60
            clip: true
            
            ColumnLayout {
                id: mainColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 24
                spacing: 24
                
                // Header
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "⚡"
                        font.pixelSize: 48
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: "Logos Core"
                        font.pixelSize: 32
                        font.weight: Font.Bold
                        font.family: "SF Pro Display"
                        color: "#f0f6fc"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: "iOS Integration Demo"
                        font.pixelSize: 16
                        font.family: "SF Pro Text"
                        color: "#8b949e"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
                
                // Status Card
                Rectangle {
                    Layout.fillWidth: true
                    height: statusColumn.height + 32
                    color: "#21262d"
                    radius: 12
                    border.color: logosBridge.initialized ? "#238636" : "#30363d"
                    border.width: 1
                    
                    ColumnLayout {
                        id: statusColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 12
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: logosBridge.initialized ? "#238636" : "#f85149"
                                
                                SequentialAnimation on opacity {
                                    running: !logosBridge.initialized
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.3; duration: 800 }
                                    NumberAnimation { to: 1.0; duration: 800 }
                                }
                            }
                            
                            Text {
                                text: "Status"
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                font.family: "SF Pro Text"
                                color: "#8b949e"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: logosBridge.initialized ? "Connected" : "Disconnected"
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                font.family: "SF Pro Text"
                                color: logosBridge.initialized ? "#238636" : "#f85149"
                            }
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#30363d"
                        }
                        
                        Text {
                            text: logosBridge.status
                            font.pixelSize: 13
                            font.family: "SF Mono"
                            color: "#7ee787"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
                
                // Control Buttons
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    
                    Text {
                        text: "Controls"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.family: "SF Pro Text"
                        color: "#8b949e"
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        
                        Button {
                            Layout.fillWidth: true
                            text: "Initialize"
                            enabled: !logosBridge.initialized
                            
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 15
                                font.weight: Font.Medium
                                font.family: "SF Pro Text"
                                color: parent.enabled ? "#ffffff" : "#484f58"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            background: Rectangle {
                                implicitHeight: 44
                                color: parent.pressed ? "#1a7f37" : (parent.enabled ? "#238636" : "#21262d")
                                radius: 8
                                border.color: parent.enabled ? "#2ea043" : "#30363d"
                                border.width: 1
                            }
                            
                            onClicked: logosBridge.initialize()
                        }
                        
                        Button {
                            Layout.fillWidth: true
                            text: "Start"
                            enabled: logosBridge.initialized
                            
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 15
                                font.weight: Font.Medium
                                font.family: "SF Pro Text"
                                color: parent.enabled ? "#ffffff" : "#484f58"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            background: Rectangle {
                                implicitHeight: 44
                                color: parent.pressed ? "#1158c7" : (parent.enabled ? "#1f6feb" : "#21262d")
                                radius: 8
                                border.color: parent.enabled ? "#388bfd" : "#30363d"
                                border.width: 1
                            }
                            
                            onClicked: logosBridge.start()
                        }
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Test Async Operation"
                        enabled: logosBridge.initialized
                        
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 15
                            font.weight: Font.Medium
                            font.family: "SF Pro Text"
                            color: parent.enabled ? "#c9d1d9" : "#484f58"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        background: Rectangle {
                            implicitHeight: 44
                            color: parent.pressed ? "#30363d" : "#21262d"
                            radius: 8
                            border.color: "#30363d"
                            border.width: 1
                        }
                        
                        onClicked: logosBridge.testAsyncOperation()
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Call packageManager:testPluginCall"
                        enabled: logosBridge.initialized
                        
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 15
                            font.weight: Font.Medium
                            font.family: "SF Pro Text"
                            color: parent.enabled ? "#ffffff" : "#484f58"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        background: Rectangle {
                            implicitHeight: 44
                            color: parent.pressed ? "#6e40c9" : "#8957e5"
                            radius: 8
                            border.color: "#a371f7"
                            border.width: 1
                        }
                        
                        onClicked: logosBridge.callTestPluginCall()
                    }
                }
                
                // Async Result Card
                Rectangle {
                    Layout.fillWidth: true
                    height: asyncColumn.height + 32
                    color: "#21262d"
                    radius: 12
                    border.color: "#30363d"
                    border.width: 1
                    visible: logosBridge.lastAsyncResult.length > 0
                    
                    ColumnLayout {
                        id: asyncColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 8
                        
                        Text {
                            text: "Async Result"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: "SF Pro Text"
                            color: "#8b949e"
                        }
                        
                        Text {
                            text: logosBridge.lastAsyncResult
                            font.pixelSize: 13
                            font.family: "SF Mono"
                            color: "#a5d6ff"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
                
                // Test Plugin Call Result Card
                Rectangle {
                    Layout.fillWidth: true
                    height: pluginCallColumn.height + 32
                    color: "#21262d"
                    radius: 12
                    border.color: logosBridge.testPluginCallResult.startsWith("Success") ? "#238636" : (logosBridge.testPluginCallResult.startsWith("Error") ? "#f85149" : "#30363d")
                    border.width: 1
                    visible: logosBridge.testPluginCallResult.length > 0
                    
                    ColumnLayout {
                        id: pluginCallColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 8
                        
                        Text {
                            text: "packageManager:testPluginCall Result"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: "SF Pro Text"
                            color: "#8b949e"
                        }
                        
                        Text {
                            text: logosBridge.testPluginCallResult
                            font.pixelSize: 13
                            font.family: "SF Mono"
                            color: logosBridge.testPluginCallResult.startsWith("Success") ? "#7ee787" : (logosBridge.testPluginCallResult.startsWith("Error") ? "#f85149" : "#a5d6ff")
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
                
                // Static Modules Card
                Rectangle {
                    Layout.fillWidth: true
                    height: staticModulesColumn.height + 32
                    color: "#21262d"
                    radius: 12
                    border.color: "#30363d"
                    border.width: 1
                    
                    ColumnLayout {
                        id: staticModulesColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "Linked Modules (Static)"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: "SF Pro Text"
                            color: "#8b949e"
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#30363d"
                        }
                        
                        // Package Manager
                        Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            color: "#161b22"
                            radius: 6
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: logosBridge.hasPackageManager ? "#238636" : "#f85149"
                                }
                                
                                Text {
                                    text: "package_manager"
                                    font.pixelSize: 13
                                    font.family: "SF Mono"
                                    color: "#c9d1d9"
                                    Layout.fillWidth: true
                                }
                                
                                Text {
                                    text: logosBridge.hasPackageManager ? "linked" : "not linked"
                                    font.pixelSize: 11
                                    font.family: "SF Mono"
                                    color: logosBridge.hasPackageManager ? "#238636" : "#f85149"
                                }
                            }
                        }
                        
                        // Capability Module
                        Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            color: "#161b22"
                            radius: 6
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: logosBridge.hasCapabilityModule ? "#238636" : "#f85149"
                                }
                                
                                Text {
                                    text: "capability_module"
                                    font.pixelSize: 13
                                    font.family: "SF Mono"
                                    color: "#c9d1d9"
                                    Layout.fillWidth: true
                                }
                                
                                Text {
                                    text: logosBridge.hasCapabilityModule ? "linked" : "not linked"
                                    font.pixelSize: 11
                                    font.family: "SF Mono"
                                    color: logosBridge.hasCapabilityModule ? "#238636" : "#f85149"
                                }
                            }
                        }
                    }
                }
                
                // Plugins List
                Rectangle {
                    Layout.fillWidth: true
                    height: pluginsColumn.height + 32
                    color: "#21262d"
                    radius: 12
                    border.color: "#30363d"
                    border.width: 1
                    
                    ColumnLayout {
                        id: pluginsColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 12
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "Loaded Plugins"
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                font.family: "SF Pro Text"
                                color: "#8b949e"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: logosBridge.loadedPlugins.length.toString()
                                font.pixelSize: 12
                                font.weight: Font.Medium
                                font.family: "SF Mono"
                                color: "#8b949e"
                                
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: -6
                                    color: "#30363d"
                                    radius: 10
                                    z: -1
                                }
                            }
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#30363d"
                        }
                        
                        // Plugin list or empty state
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Repeater {
                                model: logosBridge.loadedPlugins
                                
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 36
                                    color: "#161b22"
                                    radius: 6
                                    
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 8
                                        
                                        Rectangle {
                                            width: 8
                                            height: 8
                                            radius: 4
                                            color: "#238636"
                                        }
                                        
                                        Text {
                                            text: modelData
                                            font.pixelSize: 13
                                            font.family: "SF Mono"
                                            color: "#c9d1d9"
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                            
                            // Empty state
                            Text {
                                visible: logosBridge.loadedPlugins.length === 0
                                text: logosBridge.initialized ? "No plugins loaded" : "Initialize to see plugins"
                                font.pixelSize: 13
                                font.family: "SF Pro Text"
                                color: "#484f58"
                                font.italic: true
                                Layout.alignment: Qt.AlignHCenter
                                Layout.topMargin: 8
                            }
                        }
                        
                        // Refresh button
                        Button {
                            Layout.fillWidth: true
                            text: "Refresh List"
                            enabled: logosBridge.initialized
                            
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 14
                                font.family: "SF Pro Text"
                                color: parent.enabled ? "#58a6ff" : "#484f58"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            background: Rectangle {
                                implicitHeight: 36
                                color: "transparent"
                                radius: 6
                                border.color: parent.enabled ? "#30363d" : "#21262d"
                                border.width: 1
                            }
                            
                            onClicked: logosBridge.refreshPluginList()
                        }
                    }
                }
                
                // Footer
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    spacing: 4
                    
                    Text {
                        text: "liblogos • Qt 6 • iOS"
                        font.pixelSize: 12
                        font.family: "SF Pro Text"
                        color: "#484f58"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: "Build: " + logosBridge.buildTimestamp
                        font.pixelSize: 10
                        font.family: "SF Mono"
                        color: "#6e7681"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }
}
