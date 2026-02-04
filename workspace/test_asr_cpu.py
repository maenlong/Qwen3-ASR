import os
os.environ['CUDA_VISIBLE_DEVICES'] = ''

import torch
torch.cuda.is_available = lambda: False

from qwen_asr import Qwen3ASRModel

print('Attempting to load model on CPU...')
model = Qwen3ASRModel.from_pretrained(
   'Qwen/Qwen3-ASR-1.7B',
   device_map='cpu',
   torch_dtype=torch.float32
)
print('Model loaded successfully on CPU')

# 简单测试一下
results = model.transcribe(
   audio="https://qianwen-res.oss-cn-beijing.aliyuncs.com/Qwen3-ASR-Repo/asr_en.wav",
   language=None
)

print("Language detected:", results[0].language)
print("Transcribed text:", results[0].text)