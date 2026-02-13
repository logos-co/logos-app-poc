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
            text: "Modules"
            font.pixelSize: 24
            font.bold: true
            color: "#ffffff"
        }

        // Core Modules directly, no tabs
        CoreModulesView {
            id: coreModulesTab
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}



