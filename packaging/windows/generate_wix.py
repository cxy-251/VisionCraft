"""Generate a deterministic WiX 4 payload from windeployqt's deployment directory."""
import argparse
import hashlib
from pathlib import Path
import uuid
import xml.etree.ElementTree as ET

parser = argparse.ArgumentParser()
parser.add_argument('--payload', type=Path, required=True)
parser.add_argument('--version', required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--license', type=Path, required=True)
args = parser.parse_args()
ns = 'http://wixtoolset.org/schemas/v4/wxs'
ui_ns = ns + '/ui'
ET.register_namespace('', ns)
ET.register_namespace('ui', ui_ns)

def node(parent, tag, **attrs):
    return ET.SubElement(parent, '{' + ns + '}' + tag, attrs)

def ident(prefix, path):
    return prefix + hashlib.sha256(path.encode('utf-8')).hexdigest()[:24]

root = ET.Element('{' + ns + '}Wix')
package = node(root, 'Package', Name='VisionCraft', Manufacturer='VisionCraftStudio',
               Version=args.version, UpgradeCode='84BCCBEA-3765-45B6-85E3-995D0494691A',
               Scope='perMachine', Language='1033')
node(package, 'MajorUpgrade', DowngradeErrorMessage='A newer version of VisionCraft is already installed.')
node(package, 'MediaTemplate', EmbedCab='yes')
program_files = node(package, 'StandardDirectory', Id='ProgramFiles64Folder')
install = node(program_files, 'Directory', Id='INSTALLFOLDER', Name='VisionCraft')
feature = node(package, 'Feature', Id='MainFeature', Title='VisionCraft', Level='1')
directories = {'': install}
files = sorted(p for p in args.payload.rglob('*') if p.is_file())
if not files or not (args.payload / 'VisionCraft.exe').is_file():
    raise SystemExit('Deployment payload is missing VisionCraft.exe')
for path in files:
    rel = path.relative_to(args.payload)
    parent = install
    for i, part in enumerate(rel.parts[:-1]):
        key = '/'.join(rel.parts[:i + 1])
        if key not in directories:
            directories[key] = node(parent, 'Directory', Id=ident('D', key), Name=part)
        parent = directories[key]
    key = rel.as_posix()
    component_id = ident('C', key)
    component = node(parent, 'Component', Id=component_id,
                     Guid=str(uuid.uuid5(uuid.UUID('84bccbea-3765-45b6-85e3-995d0494691a'), key)))
    file_node = node(component, 'File', Id=ident('F', key), Source=str(path.resolve()), KeyPath='yes')
    if key == 'VisionCraft.exe':
        node(file_node, 'Shortcut', Id='LaunchVisionCraft', Name='VisionCraft',
             Directory='ApplicationProgramsFolder', WorkingDirectory='INSTALLFOLDER', Advertise='yes')
        node(component, 'RemoveFolder', Id='RemoveStartMenuFolder',
             Directory='ApplicationProgramsFolder', On='uninstall')
    node(feature, 'ComponentRef', Id=component_id)
menu = node(package, 'StandardDirectory', Id='ProgramMenuFolder')
menu = node(menu, 'Directory', Id='ApplicationProgramsFolder', Name='VisionCraft')
ET.SubElement(package, '{' + ui_ns + '}WixUI', {'Id': 'WixUI_InstallDir', 'InstallDirectory': 'INSTALLFOLDER'})
node(package, 'WixVariable', Id='WixUILicenseRtf', Value=str(args.license.resolve()))
ET.indent(root)
ET.ElementTree(root).write(args.output, encoding='utf-8', xml_declaration=True)
