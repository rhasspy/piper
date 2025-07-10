# jawris_speak.py
import sys
from TTS.api import TTS
import subprocess

tts = TTS(model_name="tts_models/en/ljspeech/tacotron2-DDC", progress_bar=False, gpu=False)

def speak(text, style="neutral"):
    filename = f"jawris_{style}.wav"
    print(f"🎤 Stimme '{style}' wird generiert...")
    tts.tts_to_file(text=text, file_path=filename)
    subprocess.run(["powershell", "-c", f"start {filename}"])

if __name__ == "__main__":
    text = sys.argv[1] if len(sys.argv) > 1 else "Greetings, Major."
    style = sys.argv[2] if len(sys.argv) > 2 else "neutral"
    speak(text, style)
