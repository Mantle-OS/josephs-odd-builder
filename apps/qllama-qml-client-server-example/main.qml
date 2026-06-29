import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QLlama

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "QLlama | Unified Loopback Simulation Interface"

    QLlamaModelParams {
        id: modelParams
        nGpuLayers: 99
    }

    QLlamaModel {
        id: localModel
        modelPath: "/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"

        Component.onCompleted: {
            loadModel(modelParams)
        }

        onIsLoadedChanged: {
            if (isLoaded) {
                console.log("[QML Layout] Base weights initialized. Provisioning compute context sub-graphs...")
                localContext.initContext(localModel, contextParams)
            }
        }
    }

    QLlamaContextParams {
        id: contextParams
        nCtx: 2048
        nThreads: 8
    }

    QLlamaContext {
        id: localContext
        onContextCreated: {
            console.log("[QML Layout] Sub-graph allocation success. Engaging edge network stack...")
            serverEngine.startServer()
        }

        onContextDestroyed: {
            serverEngine.stopServer()
        }
    }
    QLlamaSampler {
        id: localSampler
        temperature: 0.7
        topP: 0.9
    }

    QLlamaServerEngine {
        id: serverEngine
        model: localModel
        context: localContext
        sampler: localSampler
        listenPort: 8080
    }
    QLlamaClientEngine {
        id: clientEngine
        endpointUrl: "http://127.0.0.1:8080/v1/chat/completions"
        modelName: "qwen3-4b"
        temperature: 0.7
        topP: 0.9
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.4
            Layout.fillHeight: true
            spacing: 10

            Label {
                text: qsTr("Local API Server")
                font.pixelSize: 18
                font.bold: true
            }

            Frame {
                Layout.fillWidth: true
                background: Rectangle { color: "#222"; radius: 4 }

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Label { text: "<b>Architecture:</b> " + localModel.architecture; color: "#fff" }
                    Label { text: "<b>Parameters:</b> " + (localModel.parameterCount / 1e9).toFixed(2) + " B"; color: "#fff" }
                    Label { text: "<b>Hardware Status:</b> Context Allocated"; color: "#aaa" }  // line 99
                    Label { text: "<b>Port Pin:</b> " + serverEngine.listenPort; color: "#fff" }
                }
            }

            Button {
                Layout.fillWidth: true
                text: serverEngine.isListening ? "Halt Server Instance" : "Launch Server Instance"
                highlighted: !serverEngine.isListening

                onClicked: {
                    if (serverEngine.isListening) {
                        serverEngine.stopServer()
                    } else {
                        serverEngine.startServer()
                    }
                }
            }

            Label {
                text: "Service Activity Logging"
                font.bold: true
            }

            TextArea {
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                text: serverEngine.isListening ? "[qllama-server] Listening for incoming requests on port 8080...\n"
                                               : "[qllama-server] Status: Offline\n"
            }
        }

        // Vertical divider block
        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: "#444"
        }

        // CLIENT
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Label {
                text: qsTr("Client Side")
                font.pixelSize: 18
                font.bold: true
            }

            // Message History View Screen
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: chatDisplay
                    text: clientEngine.streamingText
                    readOnly: true
                    placeholderText: qsTr("Response ...")
                    font.pixelSize: 14
                    wrapMode: TextArea.WordWrap
                    textFormat:  TextEdit.MarkdownText
                }

            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                TextField {
                    id: promptInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Ask away...")
                    text: "Write a high-performance C function that computes the inverse square root of a float."
                    enabled: !clientEngine.isProcessing

                    onAccepted: sendButton.clicked()
                }

                Button {
                    id: sendButton
                    text: clientEngine.isProcessing ? qsTr("Generating...") : qsTr("Send")
                    enabled: serverEngine.isListening && promptInput.text.length > 0

                    onClicked: {
                        // FIXME these templete handlers should be in c++ based on the token pre type's
                        // its super annyoing that it is different . documentation is sparse here. per model type
                        clientEngine.prompt = "<|im_start|>user\n" + promptInput.text + "<|im_end|>\n<|im_start|>assistant\n"
                        clientEngine.generate()
                    }
                }

                Button {
                    text: "Abort"
                    visible: clientEngine.isProcessing
                    onClicked: clientEngine.cancel()
                }
            }
        }
    }
}