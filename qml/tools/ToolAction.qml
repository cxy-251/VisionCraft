import QtQuick
import QtQuick.Controls
import VisionCraft
Button {
    id: button
    implicitHeight: 36; leftPadding: 12; rightPadding: 12
    opacity: enabled ? 1 : 0.45
    contentItem: Text { text: button.text; color: Theme.text; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    background: Rectangle { color: button.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border; radius: 6 }
}
