Unicode true
!include "LogicLib.nsh"
Name "VisionCraft ${PRODUCT_VERSION} Portable"
OutFile "${OUTPUT_FILE}"
RequestExecutionLevel user
SilentInstall silent
SetCompressor /SOLID lzma
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "VisionCraft Portable"
VIAddVersionKey /LANG=1033 "FileDescription" "VisionCraft Portable Launcher"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
Section
    InitPluginsDir
    SetOutPath "$PLUGINSDIR\app"
    File /r "${PAYLOAD_DIR}\*"
    # QSettings is persistent beside the launcher, never inside the temporary payload.
    System::Call 'Kernel32::SetEnvironmentVariableW(w "VC_PORTABLE_HOME", w "$EXEDIR\VisionCraft-data") i.r0'
    ${If} $0 == 0
        MessageBox MB_OK|MB_ICONSTOP "Unable to initialize portable settings."
        SetErrorLevel 1
        Return
    ${EndIf}
    ClearErrors
    ExecWait '"$PLUGINSDIR\app\VisionCraft.exe"' $0
    ${If} ${Errors}
        MessageBox MB_OK|MB_ICONSTOP "Unable to start VisionCraft."
        SetErrorLevel 1
    ${Else}
        SetErrorLevel $0
    ${EndIf}
SectionEnd
