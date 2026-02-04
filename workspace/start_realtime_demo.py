import os
os.environ['CUDA_VISIBLE_DEVICES'] = ''
import torch
torch.cuda.is_available = lambda: False

from qwen_asr import Qwen3ASRModel
import gradio as gr
import tempfile
import numpy as np
import soundfile as sf
import io
from scipy.signal import resample

# 使用较小的模型
model = Qwen3ASRModel.from_pretrained(
   'Qwen/Qwen3-ASR-1.7B',  # 使用较小的模型
   device_map='cpu',
   torch_dtype=torch.float32
)

def transcribe_audio(audio_file):
   """处理上传的音频文件"""
   if audio_file is None:
       return "请上传音频文件"

   results = model.transcribe(
       audio=audio_file,
       language=None
   )
   return results[0].text

def transcribe_microphone(audio_data):
   """处理麦克风输入的音频"""
   if audio_data is None:
       return "请录制音频"

   # 音频数据是 (采样率, 音频数组) 格式
   sample_rate, audio_array = audio_data

   # 转换为临时文件进行处理
   temp_filename = tempfile.mktemp(suffix='.wav')
   sf.write(temp_filename, audio_array, sample_rate)

   results = model.transcribe(
       audio=temp_filename,
       language=None
   )
   return results[0].text

def transcribe_streaming(audio_chunk):
   """流式处理音频块（模拟实时转写）"""
   if audio_chunk is None:
       return "无音频输入"

   sample_rate, audio_array = audio_chunk

   # 如果音频太长，截取前10秒以加快处理
   max_samples = sample_rate * 10  # 10秒
   if len(audio_array) > max_samples:
       audio_array = audio_array[:max_samples]

   # 转换为临时文件
   temp_filename = tempfile.mktemp(suffix='.wav')
   sf.write(temp_filename, audio_array, sample_rate)

   results = model.transcribe(
       audio=temp_filename,
       language=None
   )
   return results[0].text

# 创建Gradio界面
with gr.Blocks(title="Qwen3-ASR 实时转写") as demo:
   gr.Markdown("# Qwen3-ASR 实时语音识别")
   gr.Markdown("支持上传音频文件或使用麦克风进行语音识别")

   with gr.Tab("上传音频"):
       with gr.Row():
           with gr.Column():
               audio_upload = gr.Audio(type='filepath', label='上传音频文件')
               upload_language = gr.Dropdown(
                   choices=['None', 'Chinese', 'English', 'Cantonese', 'Japanese', 'Korean'],
                   value='None',
                   label='指定语言（设为None则自动检测）'
               )
               upload_btn = gr.Button('转录音频')
           with gr.Column():
               upload_output = gr.Textbox(label='转录结果', interactive=False)

       upload_btn.click(
           fn=transcribe_audio,
           inputs=[audio_upload],
           outputs=upload_output
       )

   with gr.Tab("麦克风录音"):
       with gr.Row():
           with gr.Column():
               mic_input = gr.Microphone(label='使用麦克风录音')
               mic_language = gr.Dropdown(
                   choices=['None', 'Chinese', 'English', 'Cantonese', 'Japanese', 'Korean'],
                   value='None',
                   label='指定语言（设为None则自动检测）'
               )
               mic_btn = gr.Button('转录音频')
           with gr.Column():
               mic_output = gr.Textbox(label='转录结果', interactive=False)

       mic_btn.click(
           fn=transcribe_microphone,
           inputs=[mic_input],
           outputs=mic_output
       )

   with gr.Tab("流式处理"):
       with gr.Row():
           with gr.Column():
               stream_input = gr.Microphone(
                   label='实时录音（仅录制前10秒以加快处理）'
               )
               stream_output = gr.Textbox(label='实时转写结果', interactive=False)

               # 简单的按钮触发流式处理
               stream_btn = gr.Button('处理录音')

       stream_btn.click(
           fn=transcribe_streaming,
           inputs=[stream_input],
           outputs=stream_output
       )

# 启动服务
demo.launch(server_name='0.0.0.0', server_port=80, share=False)