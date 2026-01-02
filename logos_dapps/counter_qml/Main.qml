import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property int count: 0
    property string apiResult: ""

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 18
        width: Math.min(parent.width, 440)

        Text {
            text: "QML Counter"
            font.pixelSize: 16
            font.weight: Font.DemiBold
            color: "#1f2328"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: root.count
            font.pixelSize: 48
            font.weight: Font.Bold
            color: "#333333"
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Increment"
            Layout.alignment: Qt.AlignHCenter

            contentItem: Text {
                text: parent.text
                font.pixelSize: 15
                font.weight: Font.Medium
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 140
                implicitHeight: 44
                color: parent.pressed ? "#1a7f37" : "#238636"
                radius: 8
                border.color: "#2ea043"
                border.width: 1
            }

            onClicked: root.count++
        }

        Text {
            text: "Logos API bridge"
            font.pixelSize: 15
            font.weight: Font.DemiBold
            color: "#1f2328"
        }

        Text {
            text: "Send a value to package_manager.testPluginCall and view the response."
            color: "#57606a"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        TextField {
            id: inputField
            placeholderText: "Enter a value for testPluginCall"
            Layout.fillWidth: true
        }

        Button {
            id: sendButton
            text: "Send to package_manager"
            Layout.alignment: Qt.AlignLeft

            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 40
                color: parent.pressed ? "#0a58ca" : "#0969da"
                radius: 8
                border.color: "#0a66c2"
                border.width: 1
            }

            onClicked: {
                if (typeof logos === "undefined" || !logos.callModule) {
                    root.apiResult = "Logos bridge unavailable";
                    return;
                }

                root.apiResult = logos.callModule(
                            "package_manager",
                            "testPluginCall",
                            [inputField.text]
                        );
            }
        }

        Text {
            id: resultText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#1f2328"
            text: root.apiResult.length > 0 ? root.apiResult : "Result will appear here."
        }
    }
}
