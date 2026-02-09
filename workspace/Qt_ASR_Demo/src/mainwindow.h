#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QUrl>
#include <QByteArray>
#include <QComboBox>
#include <QDateTime>
#include <QPair>
#include <QString>
#include <QFutureWatcher>

class QAudioInput;
class QIODevice;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectFile();
    void onPlayUploadFile();
    void onUploadPlayPauseClicked();
    void onUploadPlayerStateChanged(QMediaPlayer::State state);
    void onUploadPositionChanged(qint64 position);
    void onUploadDurationChanged(qint64 duration);
    void onUploadSliderMoved(int value);

    void onMicPlayClicked();
    void onMicPlayPauseClicked();
    void onMicPlayerStateChanged(QMediaPlayer::State state);
    void onMicPositionChanged(qint64 position);
    void onMicDurationChanged(qint64 duration);
    void onMicSliderMoved(int value);

    void onMicDeviceChanged(int index);
    void onMicModeChanged(int index);
    void onMicRecordClicked();
    void onMicStartRecording();
    void onMicStopRecording();
    void onMicAudioData(const QByteArray &data);
    void onLiveChunkTimer();
    void onLiveChunkTranscribeFinished();

    void onTranscribeUploadClicked();
    void onUploadTranscribeFinished();
    void onMicTranscribeFinished();
    void onTranscribeProgressTick();
    void onSherpaTranscribeFinished();
    void onSherpaFeedTimeout();       // 流式实时：定时喂 PCM 并解码
    void onSherpaLiveSessionReady();  // 后台创建会话完成，再启动喂流定时器

public:
    enum AsrBackend { AsrBackendQwenServer, AsrBackendSherpaONNX, AsrBackendVosk };
    static AsrBackend asrBackendFromString(const QString &s);
    QString asrBackendDisplayName() const;
    bool isQwenServerBackend() const { return m_asrBackend == AsrBackendQwenServer; }

private:
    void loadAsrConfig();
    QNetworkReply *postTranscribe(const QString &filePath, const QString &language);
    void appendUploadLogRow(const QString &fileName, double durationSec, const QDateTime &startTime, qint64 durationMs);
    void appendMicLogRow(const QString &fileName, double durationSec, const QDateTime &startTime, qint64 durationMs);
    QString transcribeLanguageFromCombo(QComboBox *combo);
    void startMicTranscribe();
    void refreshMicDevices();
    void updateUploadTimeLabel();
    void updateMicTimeLabel();
    static QString formatMs(qint64 ms);
    bool writeWavFile(const QString &path, const QByteArray &pcm);
    bool writeWavFileWithFormat(const QString &path, const QByteArray &pcm16, int sampleRate, int channels);
    QString decodeMediaToWav(const QString &sourcePath, QString *outError);

    void startPreview();  // 语音激励：用当前选中设备推电平
    void stopPreview();
    void onPreviewReadyRead();
    int liveChunkIntervalSec() const;  // 从 combo 取间隔秒数
    void sendLiveChunk();              // 取当前 PCM 缓冲发一段转写（实时模式）
    void stopLiveTranscribe();         // 停止实时转写（定时器+采集，可选发最后一段）
    void onMicStartLiveTranscribe();   // 开始实时转写（开采集+定时器）
    /** 若已配置标点模型则对 Sherpa 转写文本加标点后返回，否则返回原文本。仅 HAVE_SHERPA_ONNX 时有效。 */
    QString applySherpaPunctuation(const QString &text);

    Ui::MainWindow *ui;
    QMediaPlayer *m_uploadPlayer = nullptr;
    QMediaPlayer *m_micPlayer = nullptr;
    QString m_uploadFilePath;
    qint64 m_uploadFileDurationMs = 0;   // 上传文件时长（选文件后预加载媒体得到），用于预估转写时间
    QString m_micRecordPath;  // 录音文件路径，有内容时启用播放
    bool m_uploadSliderPressed = false;
    bool m_micSliderPressed = false;

    QStringList m_micDeviceNames;
    QString m_selectedMicDeviceName;

    class AudioCollector *m_audioCollector = nullptr;
    QByteArray m_recordedPcm;
    bool m_isRecording = false;
    bool m_isLiveRunning = false;       // 实时转写运行中
    QTimer *m_liveChunkTimer = nullptr; // 按间隔发送音频块
    bool m_liveChunkSending = false;    // 正在发送本段，避免重叠
    int m_liveSegmentIndex = 0;
    QNetworkReply *m_liveChunkReply = nullptr;
    QString m_liveChunkTempPath;        // 当前段临时 WAV
    QDateTime m_liveChunkStartTime;
    double m_liveChunkDurationSec = 0;

    QAudioInput *m_previewInput = nullptr;
    QIODevice *m_previewDevice = nullptr;
    int m_previewPeak = 0;

    QNetworkAccessManager *m_networkManager = nullptr;
    QString m_apiBaseUrl;
    AsrBackend m_asrBackend = AsrBackendQwenServer;
    QString m_sherpaOnnxModelDir;
    QString m_sherpaOnnxPunctModel;  // 可选，标点模型 .onnx 路径，为空则不后处理标点
    QString m_voskModelDir;
    QNetworkReply *m_uploadTranscribeReply = nullptr;
    QNetworkReply *m_micTranscribeReply = nullptr;
    QTimer *m_transcribeProgressTimer = nullptr;
    QDateTime m_transcribeStartTime;
    QString m_transcribeFileName;
    double m_transcribeDurationSec = 0;

    QFutureWatcher<QPair<QString, QString>> *m_sherpaTranscribeWatcher = nullptr;
    bool m_sherpaTranscribeForUpload = false;  // true=上传转写, false=录音转写
    QString m_sherpaTempWavPath;  // 非 WAV 上传时解码得到的临时 WAV，用后删除
#ifdef HAVE_SHERPA_ONNX
    void *m_sherpaLiveSession = nullptr;  // SherpaLiveSession*，流式实时会话
    QTimer *m_sherpaFeedTimer = nullptr;  // 约 200ms 喂一次 PCM 并解码
    QFutureWatcher<QPair<void *, QString>> *m_sherpaLiveSessionWatcher = nullptr;  // 后台创建会话
    QString m_sherpaLiveCommittedText;   // 已确认的转写内容，中间结果显示为 committed + pending
    void *m_sherpaPunctuation = nullptr; // 标点模型实例，首次需要时创建
#endif
};

#endif // MAINWINDOW_H
