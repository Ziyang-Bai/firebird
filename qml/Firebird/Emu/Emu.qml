pragma Singleton
import QtQuick 2.0

QtObject {
    property bool gdbEnabled: true
    property int gdbPort: 3333
    property bool rdbEnabled: true
    property int rdbPort: 3334
    property bool running: false
    property bool isRunning: false
    property bool leftHanded: false
    property string version: "1.7"
    property var keypadMacros: ListModel {
        function rowCount() { return count; }
        function getDataRow(row, role) { return undefined; }
    }
    property bool keypadMacroRecording: false
    property bool keypadMacroPlaying: false
    property string activeKeypadMacroName: ""

    function useDefaultKit() {}
    function isMobile() { return true; }
    function setPaused(paused) { }
    function resume() { toast.showMessage("Resume"); }
    function dir() { return "/"; }
    function registerToast(toastref) { toast = toastref; }
    function registerNButton(keymap_id, buttonref) {}
    function restart() { toastMessage("Restart"); }
    signal toastMessage(string msg)
    signal touchpadStateChanged(real x, real y, bool contact, bool down)
    function setTouchpadState(x, y, contact, down) { touchpadStateChanged(x, y, contact, down); }
    signal buttonStateChanged(int id, bool state)
    function setButtonState(keymap_id, down) {}
    function startKeypadMacroRecording(name) { return false; }
    function stopKeypadMacroRecording() { return false; }
    function cancelKeypadMacroRecording() {}
    function playKeypadMacro(row) { return false; }
    function stopKeypadMacroPlayback() {}
    function renameKeypadMacro(row, name) { return false; }
    function deleteKeypadMacro(row) { return false; }
    function isKeypadMacroNameAvailable(name, exceptRow) { return false; }
    function keypadMacroCode(row) { return ""; }
    function validateKeypadMacroCode(code) { return ""; }
    function createKeypadMacroFromCode(name, code) { return false; }
    function replaceKeypadMacroFromCode(row, code) { return false; }
    function updateKeypadMacroFromCode(row, name, code) { return false; }
    function toLocalFile(url) { return url; }
    function basename(path) { return path; }
}
