#ifndef SHERPAONNX_HELPER_H
#define SHERPAONNX_HELPER_H

#include <QString>
#include <QPair>

#ifdef HAVE_SHERPA_ONNX
// 使用 Sherpa-ONNX 对 WAV 文件做本地转写。
// 返回 (识别文本, 错误信息)；成功时 second 为空。
QPair<QString, QString> transcribeWavWithSherpaOnnx(const QString &wavPath, const QString &modelDir);
#else
inline QPair<QString, QString> transcribeWavWithSherpaOnnx(const QString &, const QString &) {
    return qMakePair(QString(), QString::fromUtf8("Sherpa-ONNX 未编译进本程序"));
}
#endif

#endif // SHERPAONNX_HELPER_H
