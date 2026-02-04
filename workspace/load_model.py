import os
os.environ['CUDA_VISIBLE_DEVICES'] = ''

import torch
torch.cuda.is_available = lambda: False

from qwen_asr import Qwen3ASRModel

print('开始下载并加载 Qwen3-ASR-1.7B 模型...')
model = Qwen3ASRModel.from_pretrained(
   'Qwen/Qwen3-ASR-1.7B',
   device_map='cpu',
   torch_dtype=torch.float32,
   trust_remote_code=True
)
print('模型加载成功！')