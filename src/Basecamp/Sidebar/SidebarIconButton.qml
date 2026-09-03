import QtQuick
import QtQuick.Controls
import Logos.Theme
import Basecamp.Icons

AbstractButton {
    id: root

    implicitHeight: 46
    checkable: true
    autoExclusive: true

    signal tooltipRequested(string text, real y)

    onHoveredChanged: {
        if (hovered && text) {
            var pos = root.mapToItem(null, root.width, root.height / 2)
            root.tooltipRequested(text, pos.y)
        }
    }

    background: Image {
        width: 56
        height: 46
        anchors.centerIn: parent
        source: BasecampIcons.workspace
        fillMode: Image.PreserveAspectFit
    }

    contentItem: Item {
        Image {
            id: glyph
            anchors.centerIn: parent
            width: 24
            height: 24
            source: root.icon.source
            fillMode: Image.PreserveAspectFit
        }

        Rectangle {
            objectName: "sidebar.workspaceActiveDot"
            visible: root.checked
            width: 4
            height: 4
            radius: width / 2
            color: Theme.palette.accentOrange
            anchors.horizontalCenter: glyph.horizontalCenter
            anchors.top: glyph.bottom
            anchors.topMargin: Theme.spacing.tiny
        }
    }
}
