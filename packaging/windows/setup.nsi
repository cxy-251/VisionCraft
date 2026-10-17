Unicode true
!include "LogicLib.nsh"
Name "VisionCraft ${PRODUCT_VERSION}"
OutFile "${OUTPUT_FILE}"
RequestExecutionLevel user
SilentInstall silent
SetCompressor /SOLID lzma
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "VisionCraft Setup"
VIAddVersionKey /LANG=1033 "FileDescription" "VisionCraft Windows Installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
Section
    InitPluginsDir
    SetOutPath "$PLUGINSDIR"
    File /oname=VisionCraft.msi "${MSI_FILE}"
    ClearErrors
    ExecWait '"$SYSDIR\msiexec.exe" /i "$PLUGINSDIR\VisionCraft.msi"' $0
    ${If} ${Errors}
        MessageBox MB_OK|MB_ICONSTOP "Unable to launch Windows Installer."
        SetErrorLevel 1
    ${Else}
        SetErrorLevel $0
    ${EndIf}
SectionEnd
