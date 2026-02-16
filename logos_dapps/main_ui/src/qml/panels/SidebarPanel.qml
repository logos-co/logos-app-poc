import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Theme
import controls

Control {
    id: root

    /** All sidebar sections (workspaces + views) **/
    property var sections: backend.sections
    /** Property to set the different ui modules discovered **/
    property var launcherApps: backend.launcherApps
    /** Current active section index **/
    property int currentActiveSectionIndex: backend.currentActiveSectionIndex

    signal launchUIModule(string name)
    signal updateLauncherIndex(int index)

    padding: 0
    topPadding: Theme.spacing.large

    QtObject {
        id: _d

        // Filter sections by type
        readonly property var workspaceSections: (root.sections || []).filter(function(item) {
            return item && item.type === "workspace"
        })

        readonly property var viewSections: (root.sections || []).filter(function(item) {
            return item && item.type === "view"
        })

        readonly property var loadedApps: (root.launcherApps || []).filter(function(item) {
            return item && item.isLoaded === true
        })

        readonly property var unloadedApps: (root.launcherApps || []).filter(function(item) {
            return item && item.isLoaded === false
        })
    }

    background: Rectangle {
        radius: Theme.spacing.radiusXlarge
        color: Theme.palette.backgroundSecondary
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacing.large

        Image {
            // As per design
            Layout.preferredWidth: 64
            Layout.preferredHeight: 34
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/icons/basecamp.png"
        }

        SeparatorLine {}

        // Workspaces
        Column {
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            Repeater {
                id: workspaceRepeater

                model: _d.workspaceSections

                SidebarIconButton {
                    required property int index
                    required property var modelData

                    width: parent.width
                    checked: root.currentActiveSectionIndex === index
                    icon.source: modelData.iconPath
                    onClicked: root.updateLauncherIndex(index)
                }
            }
        }

        SeparatorLine {}

        // Scrollable container for apps
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 3
            Layout.rightMargin: 3

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            contentItem: Flickable {
                clip: true
                contentWidth: width
                contentHeight: appsColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                interactive: contentHeight > height

                ColumnLayout {
                    id: appsColumn

                    width: parent.width
                    spacing: Theme.spacing.large
                    
                    // Loaded apps
                    SidebarCircleButtonContainer {
                        Layout.fillWidth: true
                        contentModel: _d.loadedApps
                        visible: _d.loadedApps && _d.loadedApps.length > 0
                        onModuleClicked: (name, index) => root.launchUIModule(name)
                    }

                    // Unloaded apps
                    SidebarCircleButtonContainer {
                        Layout.fillWidth: true
                        // no background on unloaded apps
                        background: null
                        contentModel: _d.unloadedApps
                        visible: _d.unloadedApps && _d.unloadedApps.length > 0
                        onModuleClicked: (name, index) => root.launchUIModule(name)
                    }
                }
            }
        }

        SeparatorLine {}

        // View sections (Dashboard, Modules, Settings)
        SidebarCircleButtonContainer {
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            Layout.leftMargin: 3
            Layout.rightMargin: 3
            Layout.bottomMargin: Theme.spacing.large
            contentModel: _d.viewSections
            // Calculate correct index: workspace sections count + view index
            onModuleClicked: (name, index) => root.updateLauncherIndex(_d.workspaceSections.length + index)
        }
    }

    // Reusable component for SeparatorLine
    component SeparatorLine: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        Layout.leftMargin: Theme.spacing.tiny
        Layout.rightMargin: Theme.spacing.tiny
        color: Theme.palette.borderTertiaryMuted
    }
}
