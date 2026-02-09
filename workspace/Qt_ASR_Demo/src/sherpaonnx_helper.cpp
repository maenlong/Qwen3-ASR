#ifdef HAVE_SHERPA_ONNX

#include "sherpaonnx_helper.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <cstring>
#include <vector>

#include "sherpa-onnx/c-api/c-api.h"

namespace {

// 简单解析 WAV：取采样率、声道数、位深，并将 data 块中的 PCM 转为 float 单声道 [-1,1]
bool readWavToFloatMono(const QString &path, int *outSampleRate, std::vector<float> *outSamples, QString *outError)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outError) *outError = QString::fromUtf8("无法打开文件");
        return false;
    }
    QByteArray raw = f.readAll();
    f.close();
    const size_t size = static_cast<size_t>(raw.size());
    if (size < 44) {
        if (outError) *outError = QString::fromUtf8("文件过短，不是有效 WAV");
        return false;
    }
    const char *p = raw.constData();
    if (std::memcmp(p, "RIFF", 4) != 0 || std::memcmp(p + 8, "WAVE", 4) != 0) {
        if (outError) *outError = QString::fromUtf8("不是 WAV 格式");
        return false;
    }
    int sampleRate = 16000;
    int channels = 1;
    int bitsPerSample = 16;
    const char *dataPtr = nullptr;
    size_t dataBytes = 0;

    size_t pos = 12;
    while (pos + 8 <= size) {
        const char *chunkId = p + pos;
        uint32_t chunkLen;
        std::memcpy(&chunkLen, p + pos + 4, 4);
        pos += 8;
        if (pos + chunkLen > size) break;
        if (std::memcmp(chunkId, "fmt ", 4) == 0 && chunkLen >= 16) {
            uint16_t fmt;
            std::memcpy(&fmt, p + pos, 2);
            if (fmt != 1) { // PCM
                if (outError) *outError = QString::fromUtf8("仅支持 PCM WAV");
                return false;
            }
            std::memcpy(&channels, p + pos + 2, 2);
            std::memcpy(&sampleRate, p + pos + 4, 4);
            std::memcpy(&bitsPerSample, p + pos + 14, 2);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataPtr = p + pos;
            dataBytes = chunkLen;
        }
        pos += chunkLen;
    }
    if (!dataPtr || dataBytes == 0) {
        if (outError) *outError = QString::fromUtf8("未找到 WAV data 块");
        return false;
    }
    if (bitsPerSample != 16) {
        if (outError) *outError = QString::fromUtf8("仅支持 16 位 WAV");
        return false;
    }

    const int numSamples = static_cast<int>(dataBytes / (channels * 2)); // 16-bit = 2 bytes
    outSamples->resize(numSamples);
    const int16_t *src = reinterpret_cast<const int16_t *>(dataPtr);
    float *dst = outSamples->data();
    if (channels == 1) {
        for (int i = 0; i < numSamples; ++i)
            dst[i] = src[i] / 32768.0f;
    } else {
        for (int i = 0; i < numSamples; ++i)
            dst[i] = src[i * channels] / 32768.0f; // 取左声道
    }
    *outSampleRate = sampleRate;
    return true;
}

// 在 modelDir 中查找 encoder/decoder/joiner/tokens 路径，返回是否找齐
bool findModelFiles(const QString &modelDir, QString *encoder, QString *decoder, QString *joiner, QString *tokens)
{
    QDir dir(modelDir);
    if (!dir.exists()) return false;
    QString enc, dec, join, tok;
    const QFileInfoList entries = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : entries) {
        QString name = fi.fileName().toLower();
        QString path = fi.absoluteFilePath();
        if (name.contains(QStringLiteral("encoder")) && name.endsWith(QStringLiteral(".onnx")))
            enc = path;
        else if (name.contains(QStringLiteral("decoder")) && name.endsWith(QStringLiteral(".onnx")))
            dec = path;
        else if (name.contains(QStringLiteral("joiner")) && name.endsWith(QStringLiteral(".onnx")))
            join = path;
        else if (name == QStringLiteral("tokens.txt"))
            tok = path;
    }
    if (enc.isEmpty() || dec.isEmpty() || join.isEmpty() || tok.isEmpty())
        return false;
    *encoder = enc;
    *decoder = dec;
    *joiner = join;
    *tokens = tok;
    return true;
}

} // namespace

QPair<QString, QString> transcribeWavWithSherpaOnnx(const QString &wavPath, const QString &modelDir)
{
    if (modelDir.isEmpty())
        return qMakePair(QString(), QString::fromUtf8("未配置 [sherpa_onnx] model_dir"));
    QString encoderPath, decoderPath, joinerPath, tokensPath;
    if (!findModelFiles(modelDir, &encoderPath, &decoderPath, &joinerPath, &tokensPath))
        return qMakePair(QString(), QString::fromUtf8("在 model_dir 中未找到 encoder/decoder/joiner.onnx 与 tokens.txt"));

    int sampleRate = 16000;
    std::vector<float> samples;
    QString err;
    if (!readWavToFloatMono(wavPath, &sampleRate, &samples, &err))
        return qMakePair(QString(), err);
    if (samples.empty())
        return qMakePair(QString(), QString::fromUtf8("音频无有效数据"));

    QByteArray encUtf8 = encoderPath.toUtf8();
    QByteArray decUtf8 = decoderPath.toUtf8();
    QByteArray joinUtf8 = joinerPath.toUtf8();
    QByteArray tokUtf8 = tokensPath.toUtf8();

    SherpaOnnxFeatureConfig feat_config;
    feat_config.sample_rate = 16000;
    feat_config.feature_dim = 80;

    SherpaOnnxOnlineTransducerModelConfig transducer;
    transducer.encoder = encUtf8.constData();
    transducer.decoder = decUtf8.constData();
    transducer.joiner = joinUtf8.constData();

    SherpaOnnxOnlineModelConfig model_config;
    std::memset(&model_config, 0, sizeof(model_config));
    model_config.transducer = transducer;
    model_config.tokens = tokUtf8.constData();
    model_config.num_threads = 2;
    model_config.provider = "cpu";
    model_config.debug = 0;

    SherpaOnnxOnlineRecognizerConfig config;
    std::memset(&config, 0, sizeof(config));
    config.feat_config = feat_config;
    config.model_config = model_config;
    config.decoding_method = "greedy_search";
    config.max_active_paths = 4;
    config.enable_endpoint = 0;

    const SherpaOnnxOnlineRecognizer *recognizer = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!recognizer) {
        return qMakePair(QString(), QString::fromUtf8("创建 Sherpa-ONNX 识别器失败"));
    }

    const SherpaOnnxOnlineStream *stream = SherpaOnnxCreateOnlineStream(recognizer);
    if (!stream) {
        SherpaOnnxDestroyOnlineRecognizer(recognizer);
        return qMakePair(QString(), QString::fromUtf8("创建识别流失败"));
    }

    SherpaOnnxOnlineStreamAcceptWaveform(stream, sampleRate, samples.data(), static_cast<int32_t>(samples.size()));
    SherpaOnnxOnlineStreamInputFinished(stream);
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream))
        SherpaOnnxDecodeOnlineStream(recognizer, stream);

    const SherpaOnnxOnlineRecognizerResult *result = SherpaOnnxGetOnlineStreamResult(recognizer, stream);
    QString text;
    if (result && result->text)
        text = QString::fromUtf8(result->text);
    if (result)
        SherpaOnnxDestroyOnlineRecognizerResult(result);
    SherpaOnnxDestroyOnlineStream(stream);
    SherpaOnnxDestroyOnlineRecognizer(recognizer);

    return qMakePair(text, QString());
}

#endif // HAVE_SHERPA_ONNX
