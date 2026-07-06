import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    id: aboutPage

    Item {
        width: parent.width * 0.85
        height: parent.height * 0.85
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 16

            Label {
                text: qsTr("About")
                font.pixelSize: 20
                font.bold: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    text: qsTr(
                        "job_zstd is a pure C++ compression library built on Zstandard, with no Qt dependency. It compresses single files and whole directory trees into a single self-describing archive, where every entry, a file, a directory, an empty directory, or a symlink, carries its own type tag, so an archive never needs outside context to know what it contains.\n\n" +

                        "Encryption and detached Ed25519 signing are built directly on job_crypto, and are treated as first-class operations rather than optional extras. Compression and decompression are implemented as std::streambuf based transports layered on top of one another, which is what allows encryption to sit as an outer wrapper around an otherwise ordinary compressed stream without either side needing to know the other exists. The library also includes hardening against path traversal and symlink based extraction attacks, since it may end up extracting archives whose contents were not authored by someone already trusted.\n\n" +

                        "qt-zstd and qml-zstd are thin wrappers over that same engine, exposing it to Qt and QML applications, including this one, without reimplementing any of the underlying logic. Every page in this app, blocking or asynchronous, plain or encrypted, is calling straight through to the same C++ library underneath."
                    )
                }
            }

            Label {
                text: qsTr("License")
                font.pixelSize: 16
                font.bold: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                clip: true

                TextArea {
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    font.family: "monospace"
                    font.pixelSize: 12
                    text: qsTr(
                        "MIT License\n\n" +
                        "Copyright (c) 2026 Joseph Mills\n\n" +
                        "Permission is hereby granted, free of charge, to any person obtaining a copy " +
                        "of this software and associated documentation files (the \"Software\"), to deal " +
                        "in the Software without restriction, including without limitation the rights " +
                        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell " +
                        "copies of the Software, and to permit persons to whom the Software is " +
                        "furnished to do so, subject to the following conditions:\n\n" +
                        "The above copyright notice and this permission notice shall be included in all " +
                        "copies or substantial portions of the Software.\n\n" +
                        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR " +
                        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, " +
                        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE " +
                        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER " +
                        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, " +
                        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE " +
                        "SOFTWARE."
                    )
                }
            }
        }
    }
}