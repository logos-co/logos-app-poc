import QtQuick
import QtQuick.Controls
import Logos.DesignSystem

AbstractButton {
    id: root

    implicitHeight: 50
    implicitWidth: 50

    // Dark gray pill background extending to left edge when active/highlighted
    background:  Rectangle {
        radius: width / 2
        color: Theme.palette.backgroundTertiary
        border.width: root.hovered ? 1 : 0
        border.color: Theme.palette.borderSecondary
    }

    contentItem: Item {
        Image {
            id: appIcon
            anchors.centerIn: parent
            width: 24
            height: 24
            source: root.icon.source
            fillMode: Image.PreserveAspectFit
            visible: !!root.icon.source &&
                     !(appIcon.status === Image.Null ||
                       appIcon.status === Image.Error)
        }
        Text {
            anchors.centerIn: parent
            text: root.text.substring(0, 4)
            font.family: Theme.typography.publicSans
            font.pixelSize: Theme.typography.secondaryText
            font.weight: Theme.typography.weightBold
            color: Theme.palette.textSecondary
            visible: !appIcon.visible
        }
    }
}
