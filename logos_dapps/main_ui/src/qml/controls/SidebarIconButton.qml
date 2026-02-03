import QtQuick
import QtQuick.Controls
import Logos.DesignSystem

AbstractButton {
    id: root

    implicitHeight: 56
    checkable: true
    autoExclusive: true

    // Dark gray pill background extending to left edge when active/highlighted
    background: Item {
        // Allow pill to extend beyond button bounds
        clip: false  
        
        Rectangle {
            id: highlightPill
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacing.small
            width: parent.width + Theme.spacing.xlarge
            height: parent.height
            radius: height / 2
            visible: root.checked || root.hovered
            color: Theme.palette.backgroundTertiary
            border.color: Theme.palette.borderSecondary
            border.width: 1
        }
    }

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            width: 24
            height: 24
            source: root.icon.source
            fillMode: Image.PreserveAspectFit
        }
    }
}
