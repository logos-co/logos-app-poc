import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property int count: 0
    property string apiResult: ""
    property string httpStatus: "Not run"
    property string fileStatus: "Not run"
    property string openUrlStatus: "Not run"
    property string remoteLoadStatus: "Not run"
    property string imageStatus: "Not run"
    property string qfileStatus: "Not run"

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

                Text {
                    text: "Sandbox probes (should fail)"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: "#663c00"
                }

                Text {
                    text: "Trigger actions that would normally touch network or file system."
                    color: "#825f2e"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Button {
                    text: "HTTP GET https://example.com"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        httpStatus = "Running..."
                        const xhr = new XMLHttpRequest()
                        xhr.onreadystatechange = function() {
                            if (xhr.readyState === XMLHttpRequest.DONE) {
                                if (xhr.status === 0) {
                                    httpStatus = "Blocked (status=0, length=" + xhr.responseText.length + ")"
                                } else {
                                    httpStatus = "Done: status=" + xhr.status + ", length=" + xhr.responseText.length
                                }
                            }
                        }
                        xhr.onerror = function() { httpStatus = "Error (blocked)" }
                        xhr.open("GET", "https://example.com", true)
                        xhr.send()
                    }
                }
                Text { text: "HTTP status: " + root.httpStatus; color: "#825f2e"; Layout.fillWidth: true }

                Button {
                    text: "Read file:///etc/hosts"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        fileStatus = "Running..."
                        const xhr = new XMLHttpRequest()
                        xhr.timeout = 2000
                        xhr.onreadystatechange = function() {
                            if (xhr.readyState === XMLHttpRequest.DONE) {
                                if (xhr.status === 0) {
                                    fileStatus = "Blocked (status=0, length=" + xhr.responseText.length + ")"
                                } else {
                                    fileStatus = "Done: status=" + xhr.status + ", length=" + xhr.responseText.length
                                }
                            }
                        }
                        xhr.onerror = function() { fileStatus = "Error (blocked)" }
                        xhr.ontimeout = function() { fileStatus = "Timeout (blocked)" }
                        xhr.open("GET", "file:///etc/hosts", true)
                        xhr.send()
                    }
                }
                Text { text: "File status: " + root.fileStatus; color: "#825f2e"; Layout.fillWidth: true }

                Button {
                    text: "openUrlExternally(file:///etc/hosts)"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        try {
                            const ok = Qt.openUrlExternally("file:///etc/hosts")
                            openUrlStatus = "Return: " + ok
                        } catch (e) {
                            openUrlStatus = "Exception: " + e
                        }
                    }
                }
                Text { text: "Open URL status: " + root.openUrlStatus; color: "#825f2e"; Layout.fillWidth: true }

                Button {
                    text: "Loader source from http://example.com/fake.qml"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        remoteLoadStatus = "Running..."
                        remoteLoader.source = "http://example.com/fake.qml"
                    }
                }
                Loader {
                    id: remoteLoader
                    source: ""
                    onStatusChanged: {
                        if (status === Loader.Error) {
                            remoteLoadStatus = "Error (blocked): " + remoteLoader.status
                        } else if (status === Loader.Ready) {
                            remoteLoadStatus = "Loaded unexpectedly"
                        }
                    }
                }
                Text { text: "Remote load status: " + root.remoteLoadStatus; color: "#825f2e"; Layout.fillWidth: true }

                Button {
                    text: "Image source file:///etc/hosts"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        imageStatus = "Loading..."
                        naughtyImage.source = "file:///etc/hosts"
                    }
                }
                Image {
                    id: naughtyImage
                    visible: false
                    onStatusChanged: {
                        if (status === Image.Error) {
                            imageStatus = "Error (blocked): " + errorString
                        } else if (status === Image.Ready) {
                            imageStatus = "Loaded unexpectedly"
                        }
                    }
                }
                Text { text: "Image load status: " + root.imageStatus; color: "#825f2e"; Layout.fillWidth: true }

                Button {
                    text: "Attempt QFile read /etc/hosts"
                    Layout.alignment: Qt.AlignLeft
                    onClicked: {
                        qfileStatus = "Attempting..."
                        try {
                            // This should fail because QFile is not in the import list
                            // and file access should be blocked.
                            var f = new QFile("/etc/hosts")
                            if (f.exists()) {
                                qfileStatus = "Exists unexpectedly"
                            } else {
                                qfileStatus = "QFile unavailable or blocked"
                            }
                        } catch (e) {
                            qfileStatus = "Exception: " + e
                        }
                    }
                }
                Text { text: "QFile status: " + root.qfileStatus; color: "#825f2e"; Layout.fillWidth: true }
    }
}
