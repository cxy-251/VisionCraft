"""生成课程素材；只写本目录assets，不接触设备、录音或桌面。需要ffmpeg。"""
from pathlib import Path
import math
import struct
import subprocess
import wave

# [region pcm]
folder = Path(__file__).resolve().parent / "assets"
folder.mkdir(exist_ok=True)
rate, seconds, channels = 48000, 2, 2
samples = bytearray()
for i in range(rate * seconds):
    t = i / rate
    # 左440Hz、右660Hz，峰值为16位满量程的20%，首尾渐入/出20ms。
    envelope = min(1.0, t / .02, (seconds - t) / .02)
    left = round(32767 * .2 * envelope * math.sin(2 * math.pi * 440 * t))
    right = round(32767 * .2 * envelope * math.sin(2 * math.pi * 660 * t))
    samples.extend(struct.pack("<hh", left, right))
with wave.open(str(folder / "tone.wav"), "wb") as output:
    output.setnchannels(channels)
    output.setsampwidth(2)
    output.setframerate(rate)
    output.writeframes(samples)
# [endregion]
# [region video]
subprocess.run([
    "ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
    "-f", "lavfi", "-i", "testsrc2=size=320x180:rate=15:duration=2",
    "-i", str(folder / "tone.wav"), "-map", "0:v:0", "-map", "1:a:0",
    "-c:v", "ffv1", "-threads", "1", "-c:a", "pcm_s16le", "-shortest",
    str(folder / "lesson.mkv")
], check=True)
# [endregion]
with (folder.parent / "output.txt").open("w") as output:
    output.write(f"PCM: rate={rate} Hz, channels={channels}, sample=s16le\n")
    output.write(f"PCM: frames={rate * seconds}, payload={len(samples)} bytes, duration={seconds} s\n")
    output.write("Tone: left=440 Hz, right=660 Hz, peak=0.2 full scale, 20 ms fade\n")
    output.flush()
    subprocess.run(["ffprobe", "-v", "error", "-show_entries",
        "stream=codec_name,codec_type,width,height,r_frame_rate,sample_rate,channels:format=duration",
        "-of", "default=noprint_wrappers=1", str(folder / "lesson.mkv")], stdout=output, check=True)
