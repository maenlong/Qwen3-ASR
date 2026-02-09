#ifndef SHERPAONNX_HELPER_H
#define SHERPAONNX_HELPER_H

#include <QString>
#include <QPair>
#include <QByteArray>

#ifdef HAVE_SHERPA_ONNX
// 使用 Sherpa-ONNX 对 WAV 文件做本地转写。返回 (识别文本, 错误信息)；成功时 second 为空。
QPair<QString, QString> transcribeWavWithSherpaOnnx(const QString &wavPath, const QString &modelDir);
// 对 16bit 单声道 PCM 做本地转写（用于录音/上传等整段识别）。
QPair<QString, QString> transcribePcmWithSherpaOnnx(const QByteArray &pcm16, int sampleRate, const QString &modelDir);

// 流式实时：创建一次会话，持续喂 PCM 并解码，不再按固定间隔切段。
struct SherpaLiveDecodeResult { QString text; bool isFinal; QString error; };
struct SherpaLiveSession;
SherpaLiveSession *createSherpaLiveSession(const QString &modelDir, QString *outError);
void destroySherpaLiveSession(SherpaLiveSession *session);
// 喂入新 PCM 并解码，返回当前结果；isFinal 为 true 时表示一句结束，可追加到正文。
SherpaLiveDecodeResult feedPcmAndDecode(SherpaLiveSession *session, const QByteArray &pcm16, int sampleRate);
// 结束输入并取回最后一段结果，调用后需 destroySherpaLiveSession。
QPair<QString, QString> finishSherpaLiveSession(SherpaLiveSession *session);

// 可选：标点恢复。配置 [sherpa_onnx] punct_model 指向 ct-transformer 的 .onnx 后，对识别结果加标点。
void *createSherpaOfflinePunctuation(const QString &ctTransformerPath, QString *outError);
void destroySherpaOfflinePunctuation(void *punct);
QString addPunctuationToText(void *punct, const QString &text);
#else
inline QPair<QString, QString> transcribeWavWithSherpaOnnx(const QString &, const QString &) {
    return qMakePair(QString(), QString::fromUtf8("Sherpa-ONNX 未编译进本程序"));
}
inline QPair<QString, QString> transcribePcmWithSherpaOnnx(const QByteArray &, int, const QString &) {
    return qMakePair(QString(), QString::fromUtf8("Sherpa-ONNX 未编译进本程序"));
}
struct SherpaLiveDecodeResult { QString text; bool isFinal; QString error; };
struct SherpaLiveSession {};
inline SherpaLiveSession *createSherpaLiveSession(const QString &, QString *outError) {
    if (outError) *outError = QString::fromUtf8("Sherpa-ONNX 未编译进本程序");
    return nullptr;
}
inline void destroySherpaLiveSession(SherpaLiveSession *) {}
inline SherpaLiveDecodeResult feedPcmAndDecode(SherpaLiveSession *, const QByteArray &, int) {
    return SherpaLiveDecodeResult{QString(), false, QString::fromUtf8("Sherpa-ONNX 未编译进本程序")};
}
inline QPair<QString, QString> finishSherpaLiveSession(SherpaLiveSession *) {
    return qMakePair(QString(), QString::fromUtf8("Sherpa-ONNX 未编译进本程序"));
}
inline void *createSherpaOfflinePunctuation(const QString &, QString *outError) {
    if (outError) *outError = QString::fromUtf8("Sherpa-ONNX 未编译进本程序");
    return nullptr;
}
inline void destroySherpaOfflinePunctuation(void *) {}
inline QString addPunctuationToText(void *, const QString &text) { return text; }
#endif

#endif // SHERPAONNX_HELPER_H
