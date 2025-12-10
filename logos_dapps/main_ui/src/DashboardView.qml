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
            text: "Dashboard"
            font.pixelSize: 24
            font.bold: true
            color: "#ffffff"
        }

        Text {
            text: "Dashboard"
            font.pixelSize: 18
            color: "#a0a0a0"
        }

        Item { Layout.fillHeight: true }
    }
}



