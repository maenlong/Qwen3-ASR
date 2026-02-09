# Sherpa-ONNX 32 位预编译集成说明

本 Demo 支持通过 config.ini 选用 `sherpa_onnx` 做本地实时语音识别。以下为 **32 位、预编译库** 的快速接入步骤。

---

## 步骤一：下载 Sherpa-ONNX 32 位预编译包（Windows x86）

1. 打开发布页：  
   https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.12.23  

2. 下载 **Shared Libraries**、**MD**、**Release**、**no-tts** 的 x86 包（与 Qt MSVC 的 MD 运行时一致）：  
   **sherpa-onnx-v1.12.23-win-x86-shared-MD-Release-no-tts.tar.bz2**  
   直链：  
   https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.12.23/sherpa-onnx-v1.12.23-win-x86-shared-MD-Release-no-tts.tar.bz2  

3. 解压后得到目录（例如 `sherpa-onnx-v1.12.23-win-x86-shared-MD-Release-no-tts`），其内应包含：
   - `include/` — 头文件
   - `lib/` — 导入库（.lib）
   - `bin/` — 运行时 DLL（如 `sherpa-onnx.dll`、`onnxruntime.dll` 等）

4. 将**上述整个目录**放到本工程下并命名为 `third_party/sherpa-onnx`，即最终结构为：
   ```
   Qt_ASR_Demo/
   ├── third_party/
   │   └── sherpa-onnx/     ← 解压后的目录内容放这里
   │       ├── include/
   │       ├── lib/
   │       └── bin/
   ├── src/
   ├── Qt_ASR_Demo.pro
   └── ...
   ```
   若你使用其它路径，可在 Qt_ASR_Demo.pro 中设置 `SHERPA_ONNX_ROOT`（见步骤二）。

5. **DLL 由构建自动拷贝**：使用 Qt Creator 进行 **Release** 或 **Debug** 构建时，`.pro` 会在链接完成后自动将 `third_party/sherpa-onnx/bin/*.dll` 与 `lib/sherpa-onnx-c-api.dll` 复制到可执行文件所在目录（**build_release/** 或 **build_debug/**）。无需再手动复制或配置 PATH。

---

## 步骤二：工程中启用 Sherpa-ONNX（可选路径）

工程已通过 `.pro` 支持**可选**链接 Sherpa-ONNX：

- **默认**：若存在 `third_party/sherpa-onnx/include`，则自动加入头文件与库路径、链接 `sherpa-onnx.lib`，并在**构建后**将运行所需 DLL 拷贝到 `build_release/` 或 `build_debug/`。
- **自定义路径**：在 Qt Creator 的“构建环境”中增加环境变量（或在本机/项目里设置）：  
  `SHERPA_ONNX_ROOT = D:\path\to\sherpa-onnx`  
  将 `D:\path\to\sherpa-onnx` 替换为你的解压目录（该目录下要有 `include`、`lib`、`bin`）。

未解压或未设置路径时，工程仍可正常编译运行，仅无法使用 `sherpa_onnx` 后端（可继续用 `qwen_server`）。

---

## 步骤三：下载流式识别模型并配置 model_dir

1. 任选一个**流式**中文（或中英）模型，例如：
   - **Zipformer 中文流式**（Hugging Face）：  
     https://huggingface.co/csukuangfj/sherpa-onnx-streaming-zipformer-zh-int8-2025-06-30  
     或  
     https://huggingface.co/csukuangfj/sherpa-onnx-streaming-zipformer-zh-14M-2023-02-23  
   - 官方预训练模型页（可选 streaming 类别）：  
     https://k2-fsa.github.io/sherpa/onnx/pretrained_models/index.html  

2. 下载并解压到本地，得到包含 `encoder-xxx.onnx`、`decoder-xxx.onnx`、`joiner-xxx.onnx`、`tokens.txt` 等文件的目录。

3. 在 **config.ini** 中配置（与 exe 同目录）：
   ```ini
   [asr]
   backend=sherpa_onnx

   [sherpa_onnx]
   model_dir=你的模型解压目录绝对路径
   ```
   例如：
   ```ini
   [sherpa_onnx]
   model_dir=D:/models/sherpa-onnx-streaming-zipformer-zh-14M-2023-02-23
   ```

4. 启动 Demo，在界面中选择“麦克风”并开始实时转写时，将使用 Sherpa-ONNX 本地识别（若已按步骤一、二完成 Sherpa 放置并成功构建，DLL 已自动在 exe 同目录）。

---

## 步骤四（可选）：标点恢复

转写结果默认无标点。若需自动加标点、便于断句，可配置 Sherpa-ONNX 的**离线标点模型**：

1. 下载中英标点模型（CT-Transformer）：  
   https://github.com/k2-fsa/sherpa-onnx/releases/tag/punctuation-models  
   选择 **sherpa-onnx-punct-ct-transformer-zh-en-vocab272727-2024-04-12-int8**，解压后使用其中的 **model.int8.onnx**。

2. 在 config.ini 的 `[sherpa_onnx]` 下增加（路径可为相对 exe 的路径）：
   ```ini
   punct_model=../models/punct/model.int8.onnx
   ```
   将路径改为你实际放置 `model.int8.onnx` 的路径。

3. 配置后，**上传转写**、**录音转写**和**实时转写**的结果都会先经标点模型后处理再显示；不配置则保持无标点。

---

## 常见问题

- **缺少 DLL**：先确认已用 Release/Debug 完整构建一次（构建会自动把 `bin/*.dll` 与 `lib/sherpa-onnx-c-api.dll` 拷到 `build_release/` 或 `build_debug/`）；若仍报错，检查 `third_party/sherpa-onnx/bin/` 下是否有 DLL。  
- **模型加载失败**：检查 `model_dir` 路径是否正确、是否包含该模型所需的 encoder/decoder/joiner 与 `tokens.txt`。  
- **32 位与 VS 版本**：官方 x86 包用 Visual Studio 2022 编译，与 Qt 5.15.2 + VS2017 的 32 位 exe 一起使用通常需安装 **VC++ 2015–2022 Redistributable (x86)**。

完成以上步骤后，即可在 config 中通过 `backend=sherpa_onnx` 使用本地 Sherpa-ONNX 32 位预编译做实时字幕。
