import os
os.environ['CUDA_VISIBLE_DEVICES'] = ''
import torch
torch.cuda.is_available = lambda: False

from qwen_asr import Qwen3ASRModel
import gradio as gr
import tempfile
import soundfile as sf
import numpy as np

# 加载模型
model = Qwen3ASRModel.from_pretrained(
   'Qwen/Qwen3-ASR-1.7B',
   device_map='cpu',
   torch_dtype=torch.float32
)

def transcribe_audio(audio_file):
   results = model.transcribe(
       audio=audio_file,
       language=None
   )
   return results[0].text

# 创建Gradio界面
with gr.Blocks() as demo:
   gr.Markdown('# Qwen3-ASR 演示')
   gr.Markdown('上传音频文件进行语音识别')

   with gr.Row():
       with gr.Column():
           audio_input = gr.Audio(type='filepath', label='上传音频')
           language_dropdown = gr.Dropdown(
               choices=['None', 'Chinese', 'English', 'Cantonese', 'Japanese', 'Korean'],
               value='None',
               label='指定语言（设为None则自动检测）'
           )
           transcribe_btn = gr.Button('转录')

       with gr.Column():
           output_text = gr.Textbox(label='转录结果', interactive=False)

   transcribe_btn.click(
       fn=lambda audio, lang: transcribe_audio(audio),
       inputs=[audio_input, language_dropdown],
       outputs=output_text
   )

# 启动服务
demo.launch(server_name='0.0.0.0', server_port=80, share=False)