import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.0
import Firebird.Emu 1.0
import Firebird.UIComponents 1.0

ColumnLayout {
    id: root

    property bool busy: Emu.keypadMacroRecording || Emu.keypadMacroPlaying
    property bool selectedValid: macroList.currentIndex >= 0 && macroList.currentIndex < macroList.count
    property bool editingNew: true
    property bool loadingEditor: false
    property bool editorDirty: false
    property bool recordAttempted: false
    property bool programAttempted: false
    property int pendingSelection: -2

    property int editorExceptRow: editingNew ? -1 : macroList.currentIndex
    property bool editorNameAvailable: Emu.isKeypadMacroNameAvailable(editorName.text, editorExceptRow)
    property string editorNameError: editorName.text.trim() === ""
                                     ? qsTr("Enter a macro name.")
                                     : qsTr("A macro with this name already exists.")
    property string codeError: codeEditor.text.trim() === ""
                               ? qsTr("Enter macro code.")
                               : Emu.validateKeypadMacroCode(codeEditor.text)

    spacing: 5

    function durationText(durationMs) {
        return (durationMs / 1000).toFixed(2) + " s";
    }

    function selectedName() {
        return selectedValid
                ? Emu.keypadMacros.getDataRow(macroList.currentIndex, KeypadMacroModel.NameRole)
                : "";
    }

    function newCodeTemplate() {
        return "# " + qsTr("Calculate 1 + 2") + "\n"
             + "tap 1 80ms\n"
             + "wait 120ms\n"
             + "tap Plus 80ms\n"
             + "wait 120ms\n"
             + "tap 2 80ms\n"
             + "wait 120ms\n"
             + "tap Enter 80ms";
    }

    function loadSelectedMacro() {
        if(!selectedValid)
            return;
        loadingEditor = true;
        editingNew = false;
        editorName.text = selectedName();
        codeEditor.text = Emu.keypadMacroCode(macroList.currentIndex);
        loadingEditor = false;
        editorDirty = false;
        programAttempted = false;
    }

    function beginNewProgram() {
        loadingEditor = true;
        editingNew = true;
        macroList.currentIndex = -1;
        editorName.text = "";
        codeEditor.text = newCodeTemplate();
        loadingEditor = false;
        editorDirty = false;
        programAttempted = false;
        editorTabs.currentIndex = 1;
        editorName.forceActiveFocus();
    }

    function selectMacro(index) {
        editingNew = false;
        macroList.currentIndex = index;
        loadSelectedMacro();
    }

    function requestSelection(index) {
        if(editorDirty) {
            pendingSelection = index;
            discardDialog.open();
        } else if(index < 0) {
            beginNewProgram();
        } else {
            selectMacro(index);
        }
    }

    function saveProgram() {
        programAttempted = true;
        if(!editorNameAvailable || codeError !== "")
            return;

        if(editingNew) {
            if(Emu.createKeypadMacroFromCode(editorName.text, codeEditor.text)) {
                editingNew = false;
                macroList.currentIndex = macroList.count - 1;
                loadSelectedMacro();
            }
        } else if(Emu.updateKeypadMacroFromCode(macroList.currentIndex,
                                                editorName.text,
                                                codeEditor.text)) {
            loadSelectedMacro();
        }
    }

    function deleteSelected() {
        if(!selectedValid)
            return;
        var oldIndex = macroList.currentIndex;
        if(Emu.deleteKeypadMacro(oldIndex)) {
            if(macroList.count === 0)
                beginNewProgram();
            else
                selectMacro(Math.min(oldIndex, macroList.count - 1));
        }
    }

    Component.onCompleted: {
        if(macroList.count > 0)
            selectMacro(0);
        else
            beginNewProgram();
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: statusText.implicitHeight + 12
        radius: 3
        color: Emu.keypadMacroRecording ? "#f4d39a"
              : Emu.keypadMacroPlaying ? "#a9d7ef" : "#e7e7e7"

        FBLabel {
            id: statusText
            anchors.fill: parent
            anchors.margins: 6
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            color: "#202020"
            text: Emu.keypadMacroRecording
                  ? qsTr("Recording '%1'. Return to the calculator and enter the sequence.").arg(Emu.activeKeypadMacroName)
                  : Emu.keypadMacroPlaying
                    ? qsTr("Playing '%1'. Manual calculator input will stop playback.").arg(Emu.activeKeypadMacroName)
                    : qsTr("Select a saved macro to run or edit, or create one below.")
        }
    }

    RowLayout {
        Layout.fillWidth: true

        FBLabel {
            text: qsTr("Saved Macros")
            font.bold: true
        }
        Item { Layout.fillWidth: true }
        FBLabel {
            text: qsTr("%1 saved").arg(macroList.count)
        }
    }

    ListView {
        id: macroList
        Layout.fillWidth: true
        Layout.minimumHeight: 84
        Layout.preferredHeight: 110
        clip: true
        model: Emu.keypadMacros
        enabled: !root.busy
        currentIndex: count > 0 ? 0 : -1

        delegate: Rectangle {
            width: macroList.width
            height: 42
            color: ListView.isCurrentItem ? "#40536b" : (index % 2 ? "#202830" : "#182028")

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    FBLabel {
                        Layout.fillWidth: true
                        text: name
                        elide: Text.ElideRight
                        font.bold: true
                    }
                    FBLabel {
                        Layout.fillWidth: true
                        text: qsTr("%1 events, %2").arg(eventCount).arg(root.durationText(durationMs))
                        elide: Text.ElideRight
                    }
                }

                FBLabel {
                    text: "›"
                    font.pixelSize: 20
                    visible: parent.parent.ListView.isCurrentItem
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: !root.busy
                onClicked: root.requestSelection(index)
            }
        }

        FBLabel {
            anchors.centerIn: parent
            visible: macroList.count === 0
            text: qsTr("No keypad macros saved")
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 3

        Button {
            Layout.fillWidth: true
            text: Emu.keypadMacroPlaying ? qsTr("Stop") : qsTr("Run")
            enabled: root.selectedValid &&
                     (Emu.keypadMacroPlaying || (Emu.isRunning && !root.busy))
            onClicked: {
                if(Emu.keypadMacroPlaying)
                    Emu.stopKeypadMacroPlayback();
                else
                    Emu.playKeypadMacro(macroList.currentIndex);
            }
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("Edit")
            enabled: root.selectedValid && !root.busy
            onClicked: {
                root.loadSelectedMacro();
                editorTabs.currentIndex = 1;
                codeEditor.forceActiveFocus();
            }
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("Delete")
            enabled: root.selectedValid && !root.busy
            onClicked: deleteDialog.open()
        }
    }

    TabView {
        id: editorTabs
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 190

        Tab {
            title: qsTr("Record")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 7
                spacing: 6

                FBLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Enter a name, start recording, then return to the calculator to perform the sequence.")
                }

                TextField {
                    id: recordName
                    Layout.fillWidth: true
                    enabled: !root.busy
                    placeholderText: qsTr("New macro name")
                    onTextChanged: root.recordAttempted = false
                }

                FBLabel {
                    Layout.fillWidth: true
                    color: "red"
                    visible: root.recordAttempted &&
                             !Emu.isKeypadMacroNameAvailable(recordName.text, -1)
                    text: recordName.text.trim() === ""
                          ? qsTr("Enter a macro name.")
                          : qsTr("A macro with this name already exists.")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Record")
                        enabled: Emu.isRunning && !root.busy
                        onClicked: {
                            root.recordAttempted = true;
                            if(Emu.isKeypadMacroNameAvailable(recordName.text, -1) &&
                               Emu.startKeypadMacroRecording(recordName.text)) {
                                root.recordAttempted = false;
                                recordName.text = "";
                            }
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Save")
                        enabled: Emu.keypadMacroRecording
                        onClicked: {
                            if(Emu.stopKeypadMacroRecording())
                                root.selectMacro(macroList.count - 1);
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Cancel")
                        enabled: Emu.keypadMacroRecording
                        onClicked: Emu.cancelKeypadMacroRecording()
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Tab {
            title: qsTr("Code")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 7
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: editorName
                        Layout.fillWidth: true
                        enabled: !root.busy
                        placeholderText: qsTr("Macro name")
                        onTextChanged: {
                            if(!root.loadingEditor)
                                root.editorDirty = true;
                            root.programAttempted = false;
                        }
                    }
                    FBLabel {
                        text: root.editorDirty ? qsTr("Unsaved") : ""
                        color: "#b06000"
                    }
                }

                FBLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                    text: qsTr("Use tap, press, release, wait and touch. Durations are milliseconds; # starts a comment.")
                }

                TextArea {
                    id: codeEditor
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    enabled: !root.busy
                    wrapMode: TextEdit.NoWrap
                    font.family: "monospace"
                    onTextChanged: {
                        if(!root.loadingEditor)
                            root.editorDirty = true;
                        root.programAttempted = false;
                    }
                }

                FBLabel {
                    Layout.fillWidth: true
                    color: "red"
                    wrapMode: Text.WordWrap
                    visible: root.programAttempted &&
                             (!root.editorNameAvailable || root.codeError !== "")
                    text: !root.editorNameAvailable ? root.editorNameError : root.codeError
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("New")
                        enabled: !root.busy
                        onClicked: root.requestSelection(-1)
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Revert")
                        enabled: root.selectedValid && root.editorDirty && !root.busy
                        onClicked: root.loadSelectedMacro()
                    }
                    Button {
                        Layout.fillWidth: true
                        text: root.editingNew ? qsTr("Create") : qsTr("Save Changes")
                        enabled: !root.busy
                        onClicked: root.saveProgram()
                    }
                }
            }
        }
    }

    MessageDialog {
        id: deleteDialog
        title: qsTr("Delete Macro")
        text: qsTr("Delete keypad macro '%1'?").arg(root.selectedName())
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: root.deleteSelected()
    }

    MessageDialog {
        id: discardDialog
        title: qsTr("Unsaved Changes")
        text: qsTr("Discard the unsaved macro code changes?")
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            root.editorDirty = false;
            if(root.pendingSelection < 0)
                root.beginNewProgram();
            else
                root.selectMacro(root.pendingSelection);
            root.pendingSelection = -2;
        }
        onNo: root.pendingSelection = -2
    }
}
