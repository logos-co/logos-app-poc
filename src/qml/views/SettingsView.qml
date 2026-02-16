import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls

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

        LogosText {
            text: "Settings"
            font.pixelSize: 24
            font.weight: Font.Bold
            color: "#ffffff"
        }

        LogosText {
            text: "This is the Settings content area."
            color: "#a0a0a0"
        }

        Item { Layout.fillHeight: true }
    }
}



