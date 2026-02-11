# 模型文件说明

本目录用于存放 Sherpa-ONNX 语音识别与标点模型，**不随 Git 提交**（体积过大）。请按下面说明自行下载并放到对应路径。

---

## 一、语音识别模型（必选）

用于上传转写、录音转写、实时转写。

### 需要哪些文件

在 **本目录（models）** 下需同时存在：

| 文件 | 说明 |
|------|------|
| `encoder*.onnx` | 编码器，文件名需包含 `encoder` 且以 `.onnx` 结尾（如 `encoder.int8.onnx`） |
| `decoder*.onnx` | 解码器，文件名需包含 `decoder`（如 `decoder.onnx`） |
| `joiner*.onnx` | Joiner，文件名需包含 `joiner`（如 `joiner.int8.onnx`） |
| `tokens.txt` | 词表，必须为此文件名 |

四个文件缺一不可，且需来自**同一套**流式模型。

### 从哪里下载

任选一套 **流式** 中文/中英模型，例如：

- **Zipformer 中文流式（推荐）**  
  - https://huggingface.co/csukuangfj/sherpa-onnx-streaming-zipformer-zh-int8-2025-06-30  
  - 或 https://huggingface.co/csukuangfj/sherpa-onnx-streaming-zipformer-zh-14M-2023-02-23  
- **更多模型**  
  - https://k2-fsa.github.io/sherpa/onnx/pretrained_models/index.html（选 streaming 类别）

下载后解压，将解压目录中的 `encoder-xxx.onnx`、`decoder-xxx.onnx`、`joiner-xxx.onnx`、`tokens.txt` **全部拷贝到本目录 `models/`**，或把 config 里的 `model_dir` 指到解压目录。

### 本地路径怎么配

在可执行文件同目录的 **config.ini** 里：

```ini
[asr]
backend=sherpa_onnx

[sherpa_onnx]
model_dir=../models
```

- 若 exe 在 `Qt_ASR_Demo/build_release/`，则 `../models` 即本目录。  
- 也可填绝对路径，如：`model_dir=D:/Code/Qt_ASR_Demo/models`。

---

## 二、标点模型（可选）

用于在转写结果中自动加标点。不配置则无标点。

### 需要哪个文件

只需一个文件：

| 文件 | 建议路径 |
|------|----------|
| `model.int8.onnx` | 放在 **models/punct/model.int8.onnx**（即本目录下新建子目录 `punct`，把文件放进去） |

### 从哪里下载

- 发布页：https://github.com/k2-fsa/sherpa-onnx/releases/tag/punctuation-models  
- 下载 **sherpa-onnx-punct-ct-transformer-zh-en-vocab272727-2024-04-12-int8**，解压后将其中的 **model.int8.onnx** 放到 `models/punct/`。

### 本地路径怎么配

在 config.ini 的 `[sherpa_onnx]` 下增加：

```ini
punct_model=../models/punct/model.int8.onnx
```

（若 exe 在 `build_release/`，该相对路径即指向本目录下的 `punct/model.int8.onnx`。）

---

## 三、目录结构示例

按上述方式放置后，本目录大致为：

```
models/
├── README.md           （本说明）
├── tokens.txt          （必选，来自 ASR 模型包）
├── encoder.int8.onnx   （必选）
├── decoder.onnx        （必选）
├── joiner.int8.onnx    （必选）
└── punct/              （可选）
    └── model.int8.onnx
```

更多细节见项目根目录下的 **doc/集成说明-Sherpa-ONNX.md**。
