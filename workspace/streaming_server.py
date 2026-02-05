# coding=utf-8
"""
vLLM 流式实时转写服务（与官方 demo_streaming 协议兼容，增加 CORS 与 language 参数）。

需要安装: pip install qwen-asr[vllm]
运行: python streaming_server.py [--port 8001]
前端「实时字幕」标签可配置 STREAMING_API_BASE 指向本服务（如 http://localhost:8001）。
"""
import os
# 未设置时默认用国内镜像，避免连 huggingface.co 超时；已设置则保留
os.environ.setdefault('HF_ENDPOINT', 'https://hf-mirror.com')

import argparse
import time
import uuid
from dataclasses import dataclass
from typing import Dict, Optional

import numpy as np
from flask import Flask, request, jsonify

# 语言代码 -> 规范名（与 api_server 一致）
LANGUAGE_CODE_TO_CANONICAL = {
    "zh": "Chinese", "en": "English", "yue": "Cantonese", "ja": "Japanese",
    "ko": "Korean", "de": "German", "fr": "French", "es": "Spanish",
    "ar": "Arabic", "pt": "Portuguese", "id": "Indonesian", "it": "Italian",
    "ru": "Russian", "th": "Thai", "vi": "Vietnamese", "tr": "Turkish",
    "hi": "Hindi", "ms": "Malay", "nl": "Dutch", "sv": "Swedish",
    "da": "Danish", "fi": "Finnish", "pl": "Polish", "cs": "Czech",
    "fil": "Filipino", "fa": "Persian", "el": "Greek", "ro": "Romanian",
    "hu": "Hungarian", "mk": "Macedonian",
}


def parse_language(form_language: Optional[str]):
    if not form_language or str(form_language).strip().lower() in ("", "auto"):
        return None
    code = str(form_language).strip().lower()
    return LANGUAGE_CODE_TO_CANONICAL.get(code, code)


@dataclass
class Session:
    state: object
    created_at: float
    last_seen: float


app = Flask(__name__)
asr = None
SESSIONS: Dict[str, Session] = {}
SESSION_TTL_SEC = 10 * 60
UNFIXED_CHUNK_NUM = 4
UNFIXED_TOKEN_NUM = 5
CHUNK_SIZE_SEC = 1.0


@app.after_request
def _cors(resp):
    resp.headers["Access-Control-Allow-Origin"] = "*"
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    resp.headers["Access-Control-Allow-Headers"] = "Content-Type"
    return resp


@app.route("/api/start", methods=["OPTIONS"])
@app.route("/api/chunk", methods=["OPTIONS"])
@app.route("/api/finish", methods=["OPTIONS"])
def _cors_preflight():
    return "", 204


def _gc_sessions():
    now = time.time()
    dead = [sid for sid, s in SESSIONS.items() if now - s.last_seen > SESSION_TTL_SEC]
    for sid in dead:
        try:
            asr.finish_streaming_transcribe(SESSIONS[sid].state)
        except Exception:
            pass
        SESSIONS.pop(sid, None)


def _get_session(session_id: str) -> Optional[Session]:
    _gc_sessions()
    s = SESSIONS.get(session_id)
    if s:
        s.last_seen = time.time()
    return s


@app.post("/api/start")
def api_start():
    if asr is None:
        return jsonify({"error": "ASR model not loaded"}), 503
    # language: 前端传 auto/zh/en 等，转为 None 或 Chinese/English
    raw_lang = request.args.get("language") or request.form.get("language")
    if raw_lang is None:
        j = request.get_json(silent=True)
        raw_lang = j.get("language") if isinstance(j, dict) else None
    language = parse_language(raw_lang)

    session_id = uuid.uuid4().hex
    state = asr.init_streaming_state(
        language=language,
        unfixed_chunk_num=UNFIXED_CHUNK_NUM,
        unfixed_token_num=UNFIXED_TOKEN_NUM,
        chunk_size_sec=CHUNK_SIZE_SEC,
    )
    now = time.time()
    SESSIONS[session_id] = Session(state=state, created_at=now, last_seen=now)
    return jsonify({"session_id": session_id})


@app.post("/api/chunk")
def api_chunk():
    if asr is None:
        return jsonify({"error": "ASR model not loaded"}), 503
    session_id = request.args.get("session_id", "")
    s = _get_session(session_id)
    if not s:
        return jsonify({"error": "invalid session_id"}), 400
    if request.mimetype and "application/octet-stream" not in request.mimetype:
        return jsonify({"error": "expect application/octet-stream"}), 400

    raw = request.get_data(cache=False)
    if len(raw) % 4 != 0:
        return jsonify({"error": "float32 bytes length not multiple of 4"}), 400
    wav = np.frombuffer(raw, dtype=np.float32).reshape(-1)
    asr.streaming_transcribe(wav, s.state)
    return jsonify({
        "language": getattr(s.state, "language", "") or "",
        "text": getattr(s.state, "text", "") or "",
    })


@app.post("/api/finish")
def api_finish():
    if asr is None:
        return jsonify({"error": "ASR model not loaded"}), 503
    session_id = request.args.get("session_id", "")
    s = _get_session(session_id)
    if not s:
        return jsonify({"error": "invalid session_id"}), 400
    asr.finish_streaming_transcribe(s.state)
    out = {
        "language": getattr(s.state, "language", "") or "",
        "text": getattr(s.state, "text", "") or "",
    }
    SESSIONS.pop(session_id, None)
    return jsonify(out)


@app.get("/")
def health():
    return jsonify({"status": "running", "streaming": asr is not None})


def main():
    global asr
    p = argparse.ArgumentParser(description="Qwen3-ASR vLLM Streaming API (requires GPU)")
    p.add_argument("--asr-model-path", default="Qwen/Qwen3-ASR-1.7B")
    p.add_argument("--host", default="0.0.0.0")
    p.add_argument("--port", type=int, default=8001)
    p.add_argument("--gpu-memory-utilization", type=float, default=0.8)
    p.add_argument("--unfixed-chunk-num", type=int, default=4)
    p.add_argument("--unfixed-token-num", type=int, default=5)
    p.add_argument("--chunk-size-sec", type=float, default=1.0)
    args = p.parse_args()

    import torch
    if not torch.cuda.is_available():
        print("ERROR: vLLM 流式服务需要 GPU，当前未检测到可用 CUDA。")
        print("  - 若在 Docker 中运行，请使用: docker run --gpus all ...")
        print("  - 无 GPU 时请使用「分段转写」：将 client.html 中 STREAMING_API_BASE 设为 ''，仅运行 api_server.py")
        raise SystemExit(1)

    global UNFIXED_CHUNK_NUM, UNFIXED_TOKEN_NUM, CHUNK_SIZE_SEC
    UNFIXED_CHUNK_NUM = args.unfixed_chunk_num
    UNFIXED_TOKEN_NUM = args.unfixed_token_num
    CHUNK_SIZE_SEC = args.chunk_size_sec

    from qwen_asr import Qwen3ASRModel
    try:
        asr = Qwen3ASRModel.LLM(
            model=args.asr_model_path,
            gpu_memory_utilization=args.gpu_memory_utilization,
            max_new_tokens=32,
        )
    except RuntimeError as e:
        if "cuda" in str(e).lower() or "device" in str(e).lower() or "empty" in str(e).lower():
            print("ERROR: vLLM 需要 GPU，加载失败:", e)
            print("  - 若在 Docker 中运行，请使用: docker run --gpus all ... 并确保宿主机已安装 nvidia-docker")
            print("  - 无 GPU 时请将 client.html 中 STREAMING_API_BASE 设为 ''，使用 api_server.py 做分段转写")
            raise SystemExit(1)
        raise
    print("vLLM streaming model loaded.")
    app.run(host=args.host, port=args.port, debug=False, use_reloader=False, threaded=True)


if __name__ == "__main__":
    main()
