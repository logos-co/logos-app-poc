import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property int count: 0

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16

        Text {
            text: root.count
            font.pixelSize: 48
            font.weight: Font.Bold
            color: "#333333"
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Increment"
            Layout.alignment: Qt.AlignHCenter

            contentItem: Text {
                text: parent.text
                font.pixelSize: 15
                font.weight: Font.Medium
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 140
                implicitHeight: 44
                color: parent.pressed ? "#1a7f37" : "#238636"
                radius: 8
                border.color: "#2ea043"
                border.width: 1
            }

            onClicked: root.count++
        }
    }
}
