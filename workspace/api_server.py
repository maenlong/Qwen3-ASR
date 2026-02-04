import os
os.environ['CUDA_VISIBLE_DEVICES'] = ''
import torch
torch.cuda.is_available = lambda: False

from qwen_asr import Qwen3ASRModel
from flask import Flask, request, jsonify
import librosa
import io
import numpy as np
import tempfile
import os
import traceback

app = Flask(__name__)


# 允许从 Windows 上打开的网页（如 http://localhost:8080）跨域请求本 API（CORS）
@app.after_request
def _cors(resp):
    resp.headers["Access-Control-Allow-Origin"] = "*"
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    resp.headers["Access-Control-Allow-Headers"] = "Content-Type"
    return resp


@app.route("/transcribe", methods=["OPTIONS"])
def transcribe_options():
    return "", 204

# 客户端传的是前端下拉框的 value（如 zh/en/auto），Qwen3-ASR 需要规范语言名（Chinese/English）
# 见 https://github.com/QwenLM/Qwen3-ASR 及 qwen_asr/inference/utils.py SUPPORTED_LANGUAGES
LANGUAGE_CODE_TO_CANONICAL = {
    "zh": "Chinese",
    "en": "English",
    "yue": "Cantonese",
    "ja": "Japanese",
    "ko": "Korean",
    "de": "German",
    "fr": "French",
    "es": "Spanish",
    "ar": "Arabic",
    "pt": "Portuguese",
    "id": "Indonesian",
    "it": "Italian",
    "ru": "Russian",
    "th": "Thai",
    "vi": "Vietnamese",
    "tr": "Turkish",
    "hi": "Hindi",
    "ms": "Malay",
    "nl": "Dutch",
    "sv": "Swedish",
    "da": "Danish",
    "fi": "Finnish",
    "pl": "Polish",
    "cs": "Czech",
    "fil": "Filipino",
    "fa": "Persian",
    "el": "Greek",
    "ro": "Romanian",
    "hu": "Hungarian",
    "mk": "Macedonian",
}

# 加载模型（transformers 后端，CPU）
model = Qwen3ASRModel.from_pretrained(
    "Qwen/Qwen3-ASR-1.7B",
    device_map="cpu",
    dtype=torch.float32,
)

def parse_language(form_language):
    """将前端 language（auto/zh/en/...）转为 Qwen3-ASR 的 language：None 或规范名。"""
    if not form_language or form_language.strip().lower() == "auto":
        return None
    code = form_language.strip().lower()
    return LANGUAGE_CODE_TO_CANONICAL.get(code, code)

def load_audio_to_waveform(audio_bytes, target_sr=16000, filename=None):
    """将上传的音频字节转为 (波形, 采样率)。支持 wav/mp3/webm/ogg 等，webm 等格式可能需落盘再加载。"""
    try:
        y, sr = librosa.load(io.BytesIO(audio_bytes), sr=target_sr, mono=True)
        return np.asarray(y, dtype=np.float32), sr
    except Exception:
        # 浏览器麦克风多为 webm/ogg，部分环境需从文件路径加载
        ext = ".webm"
        if filename:
            for e in (".webm", ".ogg", ".wav", ".mp3"):
                if filename.lower().endswith(e):
                    ext = e
                    break
        with tempfile.NamedTemporaryFile(suffix=ext, delete=False) as f:
            f.write(audio_bytes)
            path = f.name
        try:
            y, sr = librosa.load(path, sr=target_sr, mono=True)
            return np.asarray(y, dtype=np.float32), sr
        finally:
            try:
                os.unlink(path)
            except Exception:
                pass

@app.route("/transcribe", methods=["POST"])
def transcribe_audio():
    try:
        if "audio" not in request.files:
            return jsonify({"error": "No audio file provided"}), 400

        audio_file = request.files["audio"]
        form_language = request.form.get("language", None)

        audio_data = audio_file.read()
        if not audio_data:
            return jsonify({"error": "Audio file is empty"}), 400

        filename = audio_file.filename or ""

        # 转为 (波形, 采样率)，Qwen3-ASR 支持该格式，内部会归一化到 16k
        waveform, sr = load_audio_to_waveform(audio_data, filename=filename)
        language = parse_language(form_language)

        results = model.transcribe(
            audio=(waveform, sr),
            language=language,
        )

        return jsonify({
            "success": True,
            "text": results[0].text,
            "language": results[0].language,
        })

    except Exception as e:
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500

@app.route("/", methods=["GET"])
def health_check():
    return jsonify({"status": "running"})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=80, debug=False)