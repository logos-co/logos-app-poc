import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Theme

Control {
    id: root

    property alias contentModel: repeater.model
    signal moduleClicked(string name, int index)

    padding: Theme.spacing.medium

    background: Rectangle {
        id: bg

        property real bgRadius: width/2
        property real borderWidth: 2

        radius: bgRadius

        gradient: Gradient {
            orientation: Gradient.Vertical

            GradientStop { position: 0.00; color: Theme.colors.getColor(Theme.colors.white, 0.4) }
            GradientStop { position: 0.37; color: Theme.colors.getColor(Theme.colors.white, 0)}
            GradientStop { position: 0.57; color: Theme.colors.getColor(Theme.colors.white, 0) }
            GradientStop { position: 1.00; color: Theme.colors.getColor(Theme.colors.white, 0.1) }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: bg.borderWidth
            radius: bg.bgRadius - bg.borderWidth
            color: Theme.palette.backgroundSecondary

            Rectangle {
                anchors.fill: parent
                anchors.margins: bg.borderWidth
                radius: parent.radius
                color: Theme.palette.backgroundMuted
            }
        }
    }

    contentItem: Column {
        spacing: Theme.spacing.medium
        Repeater {
            id: repeater

            delegate: SidebarCircleButton {
                checked: backend.currentActiveSectionIndex -1 === index
                text: modelData.name
                icon.source: modelData.iconPath
                onClicked: root.moduleClicked(modelData.name, index)
            }
        }
    }
}
