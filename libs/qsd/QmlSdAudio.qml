import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    property QSdAudio audioParams: null

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true

        Label {
            text: qsTr("Audio Settings")
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        // Q_PROPERTY(quint32 sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged FINAL)
        RowLayout {
            Label {
                text: qsTr("Sample Rate:")
            }

            SpinBox {
                from: 0
                to: 384000
                value: audioParams.sampleRate

                onValueModified: {
                    audioParams.sampleRate = value
                }
            }

            Label {
                text: qsTr("Hz")
            }
        }

        // Q_PROPERTY(quint32 channels READ channels WRITE setChannels NOTIFY channelsChanged FINAL)
        RowLayout {
            Label {
                text: qsTr("Channels:")
            }

            SpinBox {
                from: 0
                to: 64
                value: audioParams.channels

                onValueModified: {
                    audioParams.channels = value
                }
            }
        }

        // Q_PROPERTY(quint64 sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged FINAL)
        RowLayout {
            Label {
                text: qsTr("Sample Count:")
            }

            TextField {
                Layout.fillWidth: true
                text: audioParams.sampleCount.toString()

                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]+$/
                }

                onEditingFinished: {
                    audioParams.sampleCount = Number(text)
                }
            }
        }
    }
}