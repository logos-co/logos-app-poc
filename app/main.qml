import QtQuick
import QtQuick.Controls
import Logos.DesignSystem

ApplicationWindow {
    id: root

    visible: true
    minimumWidth: 800
    minimumHeight: 600
    color: Theme.palette.background
    flags: Qt.platform.os === "windows" ? Qt.Window
            : Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint
              

    WindowContainer {
        anchors.fill: parent
        /* for macos we want system titlebar to integrate
        with app content and hence this negative padding is needed */
        anchors.topMargin: Qt.platform.os === "osx" ? -60 : 0
        window: mainContentWindow
    }
}

