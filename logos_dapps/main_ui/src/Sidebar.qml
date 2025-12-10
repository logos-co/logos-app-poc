import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    width: 80
    color: "#2D2D2D"

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20
        anchors.bottomMargin: 20
        spacing: 15

        Repeater {
            model: backend.sidebarItems

            SidebarButton {
                required property int index
                required property string modelData
                Layout.alignment: Qt.AlignHCenter
                text: modelData
                iconSource: backend.sidebarIconAt(index)
                isActive: backend.currentViewIndex === index
                onClicked: backend.currentViewIndex = index
            }
        }

        Item { Layout.fillHeight: true }
    }
}



