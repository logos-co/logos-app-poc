import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    property string text: ""
    property string iconSource: ""
    property bool isActive: false
    
    signal clicked()
    
    width: 80
    height: 70
    
    // Active indicator bar
    Rectangle {
        id: activeIndicator
        width: 3
        height: parent.height
        anchors.left: parent.left
        color: "#4A90E2"
        visible: root.isActive
    }
    
    Rectangle {
        anchors.fill: parent
        color: mouseArea.containsMouse ? "#3D3D3D" : "transparent"
        radius: 0
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 4
            
            Image {
                id: iconImage
                Layout.alignment: Qt.AlignHCenter
                source: root.iconSource
                sourceSize.width: 28
                sourceSize.height: 28
                fillMode: Image.PreserveAspectFit
                opacity: root.isActive ? 1.0 : 0.7
            }
            
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.text
                font.pixelSize: 9
                font.bold: root.isActive
                color: root.isActive ? "#FFFFFF" : "#CCCCCC"
                horizontalAlignment: Text.AlignHCenter
            }
        }
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.clicked()
        }
    }
}



