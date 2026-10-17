param(
    [Parameter(Mandatory)][string]$Payload,
    [Parameter(Mandatory)][string]$Version,
    [string]$Output = 'release'
)
$ErrorActionPreference = 'Stop'
$Payload = (Resolve-Path $Payload).Path
New-Item -ItemType Directory -Force $Output | Out-Null
$Output = (Resolve-Path $Output).Path
$base = "VisionCraft-$Version-windows-x64"
$license = Join-Path $Output 'license.rtf'
$text = Get-Content (Join-Path $PSScriptRoot '../../LICENSE') -Raw
$text = $text.Replace('\', '\\').Replace('{', '\{').Replace('}', '\}').Replace("`r", '').Replace("`n", '\par ')
Set-Content -Encoding ascii $license ('{\rtf1\ansi\deff0 {\fonttbl {\f0 Arial;}}\f0\fs20 ' + $text + '}')
$source = Join-Path $Output 'VisionCraft.wxs'
python "$PSScriptRoot/generate_wix.py" --payload $Payload --version $Version --output $source --license $license
if ($LASTEXITCODE -ne 0) { throw 'WiX payload generation failed' }
$msi = Join-Path $Output "$base.msi"
wix build -arch x64 -ext WixToolset.UI.wixext $source -o $msi
if ($LASTEXITCODE -ne 0) { throw 'MSI packaging failed' }
$nsis = Join-Path ${env:ProgramFiles(x86)} 'NSIS/makensis.exe'
if (!(Test-Path $nsis)) { throw "NSIS not found: $nsis" }
& $nsis /V2 "/DPRODUCT_VERSION=$Version" "/DOUTPUT_FILE=$Output/$base-setup.exe" "/DMSI_FILE=$msi" "$PSScriptRoot/setup.nsi"
if ($LASTEXITCODE -ne 0) { throw 'Setup EXE packaging failed' }
& $nsis /V2 "/DPRODUCT_VERSION=$Version" "/DOUTPUT_FILE=$Output/$base-portable.exe" "/DPAYLOAD_DIR=$Payload" "$PSScriptRoot/portable.nsi"
if ($LASTEXITCODE -ne 0) { throw 'Portable EXE packaging failed' }
foreach ($name in @("$base.msi", "$base-setup.exe", "$base-portable.exe")) {
    $file = Get-Item (Join-Path $Output $name)
    if ($file.Length -eq 0) { throw "Empty package: $name" }
    Write-Host ("{0}: {1:N0} bytes" -f $file.Name, $file.Length)
}
