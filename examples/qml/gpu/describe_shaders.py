"""提取qsb构建产物的接口信息；不创建窗口或运行GPU渲染。"""
import json
from pathlib import Path
import subprocess
import sys

# 用法：python3 describe_shaders.py /path/to/qsb /path/to/build/examples/qml/gpu/.qsb
qsb, folder = sys.argv[1:]
with (Path(__file__).parent / "output.txt").open("w") as output:
    for name in ["color.frag", "blur.frag"]:
        dump = subprocess.check_output([qsb, "-d", str(Path(folder) / (name + ".qsb"))], text=True)
        header, reflection = dump.split("Reflection info:", 1)
        info, _ = json.JSONDecoder().raw_decode(reflection.lstrip())
        output.write(f"=== {name}: compiled package, not a rendering measurement ===\n")
        output.write(header.strip() + "\n")
        for block in info["uniformBlocks"]:
            output.write(f"UBO binding={block['binding']} size={block['size']}\n")
            for member in block["members"]:
                output.write(f"  {member['name']}: {member['type']} offset={member['offset']} size={member['size']}\n")
        for sampler in info["combinedImageSamplers"]:
            output.write(f"sampler {sampler['name']}: binding={sampler['binding']} type={sampler['type']}\n")
