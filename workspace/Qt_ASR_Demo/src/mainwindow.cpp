#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audiocollector.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioInput>
#include <QButtonGroup>
#include <QAudioDeviceInfo>
#include <QFileDialog>
#include <QFileInfo>
#include <QMediaContent>
#include <QListView>
#include <QSlider>
#include <QTimer>
#include <QtMath>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QCoreApplication>
#include <QUrl>
#include <QDebug>

// 与 client.html 一致的配色（Tailwind 风格）
static const char* kStyleSheet = R"(
/* 主窗口背景 */
QMainWindow, #centralwidget { background-color: #f1f5f9; }

/* 顶部标题栏：渐变 primary-600 -> primary-700，白字，上圆角 */
QFrame#headerFrame {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2563eb, stop:1 #1d4ed8);
    border: none;
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
}
QFrame#headerFrame QLabel { color: white; }
QFrame#headerFrame #labelTitle { font-size: 20px; font-weight: bold; }
QFrame#headerFrame #labelSubtitle { color: rgba(255,255,255,0.9); font-size: 13px; }

/* Tab 栏：与网页 .tabs / .tab-button 一致（底边线 + 选中白底蓝字蓝下划线） */
QWidget#tabBarWidget {
    background-color: #f8fafc;
    border: none;
    border-bottom: 1px solid #e2e8f0;
    padding: 0;
}
QPushButton#btnTabUpload, QPushButton#btnTabMic {
    background-color: transparent;
    color: #475569;
    border: none;
    border-bottom: 2px solid transparent;
    border-radius: 0;
    padding: 12px 16px;
    font-size: 15px;
    font-weight: 500;
    min-width: 100px;
}
QPushButton#btnTabUpload:checked, QPushButton#btnTabMic:checked {
    background-color: white;
    color: #2563eb;
    border-bottom: 2px solid #2563eb;
}
QPushButton#btnTabUpload:hover:!checked, QPushButton#btnTabMic:hover:!checked {
    color: #1e293b;
}
/* 内容区（原 Tab 页） */
QStackedWidget#stackedWidget {
    background-color: white;
    border: 1px solid #e2e8f0;
    border-top: none;
    border-bottom-left-radius: 8px;
    border-bottom-right-radius: 8px;
    padding: 16px;
}

/* 主按钮：primary-600，白字，圆角 */
QPushButton {
    background-color: #2563eb;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 10px 14px;
    font-size: 15px;
    font-weight: 500;
}
QPushButton:hover { background-color: #1d4ed8; }
QPushButton:pressed { background-color: #1e40af; }
QPushButton:disabled { background-color: #94a3b8; color: #e2e8f0; }

/* 录音按钮在“停止”状态时红色（与网页 btn-stop 一致） */
QPushButton#btnMicRecord:checked {
    background-color: #dc2626;
}
QPushButton#btnMicRecord:checked:hover { background-color: #b91c1c; }
QPushButton#btnMicRecord:checked:pressed { background-color: #991b1b; }

/* 下拉框：边框 neutral-300，圆角，与弹出项同高，字重正常不加粗 */
QComboBox {
    background-color: white;
    border: 1px solid #cbd5e1;
    border-radius: 4px;
    padding: 10px 12px;
    font-size: 15px;
    font-weight: normal;
    min-height: 40px;
}
QComboBox:hover { border-color: #94a3b8; }
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView::item {
    min-height: 40px;
    padding: 8px 12px;
    font-size: 14px;
    font-weight: normal;
}
QComboBox QAbstractItemView {
    border: 1px solid #cbd5e1;
    border-radius: 4px;
    background-color: white;
    font-size: 14px;
    font-weight: normal;
    selection-background-color: #eff6ff;
    selection-color: #1e293b;
}

/* 文本框（转录结果）：边框、圆角、内边距 */
QTextEdit {
    background-color: white;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    padding: 12px;
    font-size: 15px;
    color: #1e293b;
}

/* 表单标签：neutral-700，字重 500 */
QLabel {
    color: #334155;
    font-weight: 500;
    font-size: 15px;
}

/* 状态栏样式：背景 neutral-100，字色 neutral-600 */
QLabel#labelUploadStatus, QLabel#labelMicStatus {
    background-color: #f1f5f9;
    color: #475569;
    padding: 10px 15px;
    border-radius: 4px;
    font-weight: normal;
    font-size: 15px;
}
QLabel#labelUploadProgressText, QLabel#labelMicProgressText {
    font-size: 13px;
    color: #475569;
    font-weight: normal;
}

/* 进度条：6px 高，轨道 neutral-200，块 primary-500 */
QProgressBar {
    border: none;
    background-color: #e2e8f0;
    border-radius: 3px;
    height: 6px;
    text-align: center;
}
QProgressBar::chunk {
    background-color: #3b82f6;
    border-radius: 3px;
}

/* 转写日志表格：表头 neutral-100 + 粗体，行线 neutral-200 */
QTableWidget {
    background-color: white;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    gridline-color: #e2e8f0;
    font-size: 14px;
}
QTableWidget::item { padding: 8px 10px; color: #1e293b; font-size: 14px; }
QHeaderView::section {
    background-color: #f1f5f9;
    color: #1e293b;
    font-weight: 600;
    font-size: 14px;
    padding: 10px 12px;
    min-height: 28px;
    border: none;
    border-bottom: 2px solid #94a3b8;
    border-right: 1px solid #e2e8f0;
}

/* 播放器进度条 */
QSlider::groove:horizontal {
    height: 6px;
    background: #e2e8f0;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 14px;
    margin: -4px 0;
    background: #2563eb;
    border-radius: 7px;
}
QSlider::sub-page:horizontal { background: #3b82f6; border-radius: 3px; }
)";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Qwen3-ASR 语音识别 Demo"));
    resize(1150, 780);

    setStyleSheet(kStyleSheet);

    // 标题栏文字水平居中（.ui 中 item 的 alignment 会导致解析错误，故在代码中设置）
    ui->verticalLayout_header->setAlignment(ui->labelTitle, Qt::AlignHCenter);
    ui->verticalLayout_header->setAlignment(ui->labelSubtitle, Qt::AlignHCenter);

    // 与网页一致的布局间距
    ui->verticalLayout_main->setSpacing(0);
    ui->verticalLayout_main->setContentsMargins(12, 12, 12, 12);

    // 上传/麦克风 Tab 内容区边距
    ui->horizontalLayout_upload->setSpacing(20);
    ui->horizontalLayout_upload->setContentsMargins(0, 0, 0, 0);
    ui->horizontalLayout_mic->setSpacing(20);
    ui->horizontalLayout_mic->setContentsMargins(0, 0, 0, 0);

    ui->verticalLayout_uploadLeft->setSpacing(12);
    ui->verticalLayout_uploadLeft->setContentsMargins(0, 8, 0, 0);
    ui->verticalLayout_micLeft->setSpacing(12);
    ui->verticalLayout_micLeft->setContentsMargins(0, 8, 0, 0);

    ui->verticalLayout_uploadRight->setSpacing(8);
    ui->verticalLayout_uploadRight->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_micRight->setSpacing(8);
    ui->verticalLayout_micRight->setContentsMargins(0, 0, 0, 0);

    // 两页左右栏统一：左侧固定 360px，右侧占满剩余空间，切换 Tab 不跳动
    ui->horizontalLayout_upload->setStretch(0, 0);
    ui->horizontalLayout_upload->setStretch(1, 1);
    ui->horizontalLayout_mic->setStretch(0, 0);
    ui->horizontalLayout_mic->setStretch(1, 1);

    // 转写日志表头（与 client.html 一致），并确保表头可见
    const QStringList headers { tr("序号"), tr("音频文件名"), tr("音频时长"),
                               tr("转写开始时间"), tr("耗时"), tr("转写结束时间") };
    // 两个日志表一致：显示顶部列名（序号、音频文件名…），隐藏左侧行号列（第一列已是序号）
    ui->tableUploadLog->setHorizontalHeaderLabels(headers);
    ui->tableUploadLog->verticalHeader()->setVisible(false);
    ui->tableUploadLog->horizontalHeader()->setVisible(true);
    ui->tableUploadLog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableMicLog->setHorizontalHeaderLabels(headers);
    ui->tableMicLog->verticalHeader()->setVisible(false);
    ui->tableMicLog->horizontalHeader()->setVisible(true);
    ui->tableMicLog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Tab 按钮互斥并切换 stackedWidget 页面（两按钮等分宽度，与网页一致）
    ui->horizontalLayout_tabBar->setStretch(0, 1);
    ui->horizontalLayout_tabBar->setStretch(1, 1);
    QButtonGroup *tabGroup = new QButtonGroup(this);
    tabGroup->setExclusive(true);
    tabGroup->addButton(ui->btnTabUpload, 0);
    tabGroup->addButton(ui->btnTabMic, 1);
    connect(tabGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
        ui->stackedWidget->setCurrentIndex(id);
    });
    ui->stackedWidget->setCurrentIndex(0);

    // 上传：选择文件、播放、播放器内暂停/进度
    connect(ui->btnSelectFile, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    connect(ui->btnPlayFile, &QPushButton::clicked, this, &MainWindow::onPlayUploadFile);
    connect(ui->btnUploadPlayPause, &QPushButton::clicked, this, &MainWindow::onUploadPlayPauseClicked);
    m_uploadPlayer = new QMediaPlayer(this);
    connect(m_uploadPlayer, &QMediaPlayer::stateChanged, this, &MainWindow::onUploadPlayerStateChanged);
    connect(m_uploadPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::onUploadPositionChanged);
    connect(m_uploadPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onUploadDurationChanged);
    connect(ui->sliderUploadPosition, &QSlider::sliderPressed, this, [this]() { m_uploadSliderPressed = true; });
    connect(ui->sliderUploadPosition, &QSlider::sliderReleased, this, [this]() { m_uploadSliderPressed = false; });
    connect(ui->sliderUploadPosition, &QSlider::sliderMoved, this, &MainWindow::onUploadSliderMoved);

    // 麦克风：播放录音、播放器内暂停/进度（录音未实现前仅显示容器）
    connect(ui->btnMicPlay, &QPushButton::clicked, this, &MainWindow::onMicPlayClicked);
    connect(ui->btnMicPlayPause, &QPushButton::clicked, this, &MainWindow::onMicPlayPauseClicked);
    m_micPlayer = new QMediaPlayer(this);
    connect(m_micPlayer, &QMediaPlayer::stateChanged, this, &MainWindow::onMicPlayerStateChanged);
    connect(m_micPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::onMicPositionChanged);
    connect(m_micPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onMicDurationChanged);
    connect(ui->sliderMicPosition, &QSlider::sliderPressed, this, [this]() { m_micSliderPressed = true; });
    connect(ui->sliderMicPosition, &QSlider::sliderReleased, this, [this]() { m_micSliderPressed = false; });
    connect(ui->sliderMicPosition, &QSlider::sliderMoved, this, &MainWindow::onMicSliderMoved);

    // 所有下拉框统一使用自定义 QListView，使弹出项高度与 combobox 一致
    auto setComboView = [this](QComboBox *combo) {
        QListView *v = new QListView(this);
        v->setUniformItemSizes(true);
        combo->setView(v);
    };
    setComboView(ui->comboUploadLanguage);
    setComboView(ui->comboMicDevice);
    setComboView(ui->comboMicLanguage);
    setComboView(ui->comboMicMode);
    setComboView(ui->comboLiveChunkSeconds);
    ui->comboLiveChunkSeconds->setCurrentIndex(1);  // 默认每 10 秒

    // 麦克风设备列表与选择变化；切换设备后自动开语音激励预览
    refreshMicDevices();
    connect(ui->comboMicDevice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onMicDeviceChanged);
    connect(ui->comboMicMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onMicModeChanged);
    onMicModeChanged(ui->comboMicMode->currentIndex());  // 初始按钮文案与转写间隔显隐
    startPreview();

    // 录音/实时：采集器与开始/停止
    m_audioCollector = new AudioCollector(this);
    connect(m_audioCollector, &AudioCollector::audioDataCaptured, this, &MainWindow::onMicAudioData);
    connect(ui->btnMicRecord, &QPushButton::clicked, this, &MainWindow::onMicRecordClicked);
    ui->btnMicRecord->setCheckable(true);
    m_liveChunkTimer = new QTimer(this);
    m_liveChunkTimer->setSingleShot(false);
    connect(m_liveChunkTimer, &QTimer::timeout, this, &MainWindow::onLiveChunkTimer);

    m_networkManager = new QNetworkAccessManager(this);
    loadApiConfig();
    m_transcribeProgressTimer = new QTimer(this);
    m_transcribeProgressTimer->setInterval(1000);
    connect(m_transcribeProgressTimer, &QTimer::timeout, this, &MainWindow::onTranscribeProgressTick);
    connect(ui->btnTranscribeUpload, &QPushButton::clicked, this, &MainWindow::onTranscribeUploadClicked);
}

MainWindow::~MainWindow()
{
    stopPreview();
    delete ui;
}

QString MainWindow::formatMs(qint64 ms)
{
    const qint64 s = ms / 1000;
    return QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QChar('0'));
}

void MainWindow::onSelectFile()
{
    qDebug() << "[UI] Button: Select file";
    const QString path = QFileDialog::getOpenFileName(this, tr("选择音频文件"), QString(),
                                                      tr("音频 (*.wav *.mp3 *.ogg *.m4a *.flac);;所有文件 (*)"));
    if (path.isEmpty()) {
        qDebug() << "[UI] No file selected";
        return;
    }
    m_uploadFilePath = path;
    m_uploadFileDurationMs = 0;
    qDebug() << "[UI] File selected:" << path;
    ui->labelFileName->setText(QFileInfo(path).fileName());
    ui->btnPlayFile->setEnabled(true);
    ui->uploadPlayerContainer->setVisible(false);
    if (m_uploadPlayer->state() != QMediaPlayer::StoppedState)
        m_uploadPlayer->stop();
    // 预加载媒体以获取时长（与网页端一致），便于转写时显示预估时间
    m_uploadPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(m_uploadFilePath)));
}

void MainWindow::onPlayUploadFile()
{
    qDebug() << "[UI] Button: Play upload file";
    if (m_uploadFilePath.isEmpty()) return;
    ui->uploadPlayerContainer->setVisible(true);
    m_uploadPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(m_uploadFilePath)));
    m_uploadPlayer->play();
}

void MainWindow::onUploadPlayPauseClicked()
{
    qDebug() << "[UI] Button: Upload play/pause, state=" << m_uploadPlayer->state();
    if (m_uploadPlayer->state() == QMediaPlayer::PlayingState)
        m_uploadPlayer->pause();
    else
        m_uploadPlayer->play();
}

void MainWindow::onUploadPlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState)
        ui->btnUploadPlayPause->setText(tr("暂停"));
    else
        ui->btnUploadPlayPause->setText(tr("播放"));
    updateUploadTimeLabel();
}

void MainWindow::onUploadPositionChanged(qint64 position)
{
    if (!m_uploadSliderPressed && m_uploadPlayer->duration() > 0) {
        ui->sliderUploadPosition->setMaximum(1000);
        ui->sliderUploadPosition->setValue(static_cast<int>(1000 * position / m_uploadPlayer->duration()));
    }
    updateUploadTimeLabel();
}

void MainWindow::onUploadDurationChanged(qint64 duration)
{
    if (duration > 0)
        m_uploadFileDurationMs = duration;
    updateUploadTimeLabel();
}

void MainWindow::onUploadSliderMoved(int value)
{
    if (m_uploadPlayer->duration() <= 0) return;
    m_uploadPlayer->setPosition(static_cast<qint64>(value) * m_uploadPlayer->duration() / 1000);
}

void MainWindow::updateUploadTimeLabel()
{
    const qint64 pos = m_uploadPlayer->position();
    const qint64 dur = m_uploadPlayer->duration();
    ui->labelUploadTime->setText(formatMs(pos) + " / " + formatMs(dur > 0 ? dur : 0));
}

void MainWindow::onMicPlayClicked()
{
    qDebug() << "[UI] Button: Play mic recording";
    if (m_micRecordPath.isEmpty()) return;
    ui->micPlayerContainer->setVisible(true);
    m_micPlayer->setMedia(QMediaContent(QUrl::fromLocalFile(m_micRecordPath)));
    m_micPlayer->play();
}

void MainWindow::onMicPlayPauseClicked()
{
    qDebug() << "[UI] Button: Mic play/pause, state=" << m_micPlayer->state();
    if (m_micPlayer->state() == QMediaPlayer::PlayingState)
        m_micPlayer->pause();
    else
        m_micPlayer->play();
}

void MainWindow::onMicPlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState)
        ui->btnMicPlayPause->setText(tr("暂停"));
    else
        ui->btnMicPlayPause->setText(tr("播放"));
    updateMicTimeLabel();
}

void MainWindow::onMicPositionChanged(qint64 position)
{
    if (!m_micSliderPressed && m_micPlayer->duration() > 0) {
        ui->sliderMicPosition->setMaximum(1000);
        ui->sliderMicPosition->setValue(static_cast<int>(1000 * position / m_micPlayer->duration()));
    }
    updateMicTimeLabel();
}

void MainWindow::onMicDurationChanged(qint64 duration)
{
    Q_UNUSED(duration);
    updateMicTimeLabel();
}

void MainWindow::onMicSliderMoved(int value)
{
    if (m_micPlayer->duration() <= 0) return;
    m_micPlayer->setPosition(static_cast<qint64>(value) * m_micPlayer->duration() / 1000);
}

void MainWindow::updateMicTimeLabel()
{
    const qint64 pos = m_micPlayer->position();
    const qint64 dur = m_micPlayer->duration();
    ui->labelMicTime->setText(formatMs(pos) + " / " + formatMs(dur > 0 ? dur : 0));
}

void MainWindow::refreshMicDevices()
{
    m_micDeviceNames.clear();
    ui->comboMicDevice->clear();

    const QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const QAudioDeviceInfo &info : devices) {
        const QString name = info.deviceName();
        m_micDeviceNames.append(name);
        ui->comboMicDevice->addItem(name);
    }

    if (m_micDeviceNames.isEmpty()) {
        ui->comboMicDevice->addItem(tr("未检测到麦克风"));
        m_selectedMicDeviceName.clear();
    } else {
        m_selectedMicDeviceName = m_micDeviceNames.first();
    }
}

void MainWindow::onMicDeviceChanged(int index)
{
    qDebug() << "[UI] Mic device changed, index=" << index << "name=" << (index >= 0 && index < m_micDeviceNames.size() ? m_micDeviceNames.at(index) : QString());
    if (index >= 0 && index < m_micDeviceNames.size()) {
        m_selectedMicDeviceName = m_micDeviceNames.at(index);
    }
    stopPreview();
    startPreview();
}

void MainWindow::onMicModeChanged(int index)
{
    qDebug() << "[UI] Mic mode changed, index=" << index << (index == 1 ? "live" : "record");
    const bool isLive = (index == 1);
    ui->liveChunkWrap->setVisible(isLive);
    ui->comboLiveChunkSeconds->setEnabled(!m_isLiveRunning);
    if (isLive) {
        ui->btnMicRecord->setText(m_isLiveRunning ? tr("停止实时转写") : tr("开始实时转写"));
        ui->btnMicPlay->setVisible(false);  // 实时模式不显示播放录音
    } else {
        ui->btnMicRecord->setText(m_isRecording ? tr("停止录音") : tr("开始录音"));
        ui->btnMicPlay->setVisible(true);
        if (m_isLiveRunning)
            stopLiveTranscribe();
    }
}

void MainWindow::startPreview()
{
    if (m_selectedMicDeviceName.isEmpty()) return;
    stopPreview();

    QAudioDeviceInfo deviceInfo;
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == m_selectedMicDeviceName) {
            deviceInfo = info;
            break;
        }
    }
    if (deviceInfo.deviceName().isEmpty()) return;

    QAudioFormat format;
    format.setSampleRate(AudioCollector::sampleRate());
    format.setChannelCount(AudioCollector::channelCount());
    format.setSampleSize(AudioCollector::sampleSize());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    m_previewInput = new QAudioInput(deviceInfo, format, this);
    m_previewDevice = m_previewInput->start();
    if (m_previewDevice)
        connect(m_previewDevice, &QIODevice::readyRead, this, &MainWindow::onPreviewReadyRead);
}

void MainWindow::stopPreview()
{
    if (m_previewInput) {
        if (m_previewDevice) {
            m_previewDevice->disconnect(this);
            m_previewDevice = nullptr;
        }
        m_previewInput->stop();
        delete m_previewInput;
        m_previewInput = nullptr;
    }
    m_previewDevice = nullptr;
    ui->progressMicLevel->setValue(0);
}

void MainWindow::onPreviewReadyRead()
{
    if (!m_previewDevice) return;
    QByteArray data = m_previewDevice->readAll();
    if (data.size() < 2) return;

    int maxAbs = 0;
    const char *p = data.constData();
    for (int i = 0; i + 1 < data.size(); i += 2) {
        qint16 s;
        std::memcpy(&s, p + i, 2);
        maxAbs = std::max(maxAbs, std::abs(static_cast<int>(s)));
    }
    int level = (maxAbs * 100) / 32768;
    level = std::min(100, level);
    m_previewPeak = std::max(m_previewPeak, level);
    ui->progressMicLevel->setValue(m_previewPeak);
    // 峰值缓慢回落
    QTimer::singleShot(80, this, [this]() {
        m_previewPeak = std::max(0, m_previewPeak - 3);
        if (m_previewInput)
            ui->progressMicLevel->setValue(m_previewPeak);
    });
}

void MainWindow::onMicAudioData(const QByteArray &data)
{
    m_recordedPcm.append(data);
    // 录音或实时转写时用同一路数据驱动语音激励条
    const bool drivingLevel = m_isRecording || m_isLiveRunning;
    if (!drivingLevel || data.size() < 2) return;
    int maxAbs = 0;
    const char *p = data.constData();
    for (int i = 0; i + 1 < data.size(); i += 2) {
        qint16 s;
        std::memcpy(&s, p + i, 2);
        maxAbs = std::max(maxAbs, std::abs(static_cast<int>(s)));
    }
    int level = (maxAbs * 100) / 32768;
    level = std::min(100, level);
    m_previewPeak = std::max(m_previewPeak, level);
    ui->progressMicLevel->setValue(m_previewPeak);
    QTimer::singleShot(80, this, [this]() {
        if (m_isRecording || m_isLiveRunning) {
            m_previewPeak = std::max(0, m_previewPeak - 3);
            ui->progressMicLevel->setValue(m_previewPeak);
        }
    });
}

bool MainWindow::writeWavFile(const QString &path, const QByteArray &pcm)
{
    const int sampleRate = AudioCollector::sampleRate();
    const int channels = AudioCollector::channelCount();
    const int bitsPerSample = AudioCollector::sampleSize();
    const int byteRate = sampleRate * channels * (bitsPerSample / 8);
    const int dataSize = pcm.size();
    const int fileSize = 36 + dataSize;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.writeRawData("RIFF", 4);
    ds << quint32(fileSize);
    ds.writeRawData("WAVE", 4);
    ds.writeRawData("fmt ", 4);
    ds << quint32(16u);   // fmt chunk size
    ds << quint16(1u);    // PCM
    ds << quint16(channels);
    ds << quint32(sampleRate);
    ds << quint32(byteRate);
    ds << quint16(channels * (bitsPerSample / 8));
    ds << quint16(bitsPerSample);
    ds.writeRawData("data", 4);
    ds << quint32(dataSize);
    f.write(pcm);
    f.close();
    return true;
}

void MainWindow::onMicRecordClicked()
{
    const int mode = ui->comboMicMode->currentIndex();
    qDebug() << "[UI] Button: Mic record/live, mode=" << (mode == 1 ? "live" : "record") << "isLiveRunning=" << m_isLiveRunning << "isRecording=" << m_isRecording;
    if (mode == 1) {
        if (m_isLiveRunning)
            stopLiveTranscribe();
        else
            onMicStartLiveTranscribe();
        return;
    }
    if (m_isRecording)
        onMicStopRecording();
    else
        onMicStartRecording();
}

void MainWindow::onMicStartRecording()
{
    qDebug() << "[UI] Start recording, device=" << m_selectedMicDeviceName;
    if (m_selectedMicDeviceName.isEmpty()) {
        ui->labelMicStatus->setText(tr("请先选择麦克风设备"));
        return;
    }
    stopPreview();
    m_audioCollector->selectDevice(m_selectedMicDeviceName);
    m_recordedPcm.clear();
    m_audioCollector->startCapture();
    m_isRecording = true;
    ui->btnMicRecord->setText(tr("停止录音"));
    ui->btnMicRecord->setChecked(true);
    ui->labelMicStatus->setText(tr("录音中…"));
}

void MainWindow::onMicStopRecording()
{
    qDebug() << "[UI] Stop recording, PCM size=" << m_recordedPcm.size();
    m_audioCollector->stopCapture();
    m_isRecording = false;
    ui->btnMicRecord->setText(tr("开始录音"));
    ui->btnMicRecord->setChecked(false);

    if (m_recordedPcm.isEmpty()) {
        ui->labelMicStatus->setText(tr("就绪（未录到数据）"));
        m_micRecordPath.clear();
        ui->btnMicPlay->setEnabled(false);
        return;
    }

    QString path = QDir::tempPath() + QString("/qt_asr_rec_%1.wav")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    if (writeWavFile(path, m_recordedPcm)) {
        m_micRecordPath = path;
        qDebug() << "[UI] Recording saved, starting transcribe:" << path;
        ui->btnMicPlay->setEnabled(true);
        ui->labelMicStatus->setText(tr("录音已保存，正在转写…"));
        startMicTranscribe();
    } else {
        ui->labelMicStatus->setText(tr("保存录音失败"));
        ui->btnMicPlay->setEnabled(false);
    }
    startPreview();
}

int MainWindow::liveChunkIntervalSec() const
{
    const int idx = ui->comboLiveChunkSeconds->currentIndex();
    switch (idx) {
    case 0: return 5;
    case 1: return 10;
    case 2: return 30;
    case 3: return 60;
    default: return 10;
    }
}

void MainWindow::onMicStartLiveTranscribe()
{
    const int intervalSec = liveChunkIntervalSec();
    qDebug() << "[UI] Start live transcribe, device=" << m_selectedMicDeviceName << "interval_sec=" << intervalSec;
    if (m_selectedMicDeviceName.isEmpty()) {
        ui->labelMicStatus->setText(tr("请先选择麦克风设备"));
        return;
    }
    stopPreview();
    m_audioCollector->selectDevice(m_selectedMicDeviceName);
    m_recordedPcm.clear();
    m_liveSegmentIndex = 0;
    m_audioCollector->startCapture();
    m_isLiveRunning = true;
    ui->btnMicRecord->setText(tr("停止实时转写"));
    ui->btnMicRecord->setChecked(true);
    ui->comboLiveChunkSeconds->setEnabled(false);
    const int sec = liveChunkIntervalSec();
    ui->labelMicStatus->setText(tr("分段转写中：每 %1 秒发送一段音频并追加转写结果。").arg(sec));
    m_liveChunkTimer->start(sec * 1000);
}

void MainWindow::sendLiveChunk()
{
    if (m_liveChunkSending) return;
    const int minBytes = 16000;
    if (m_recordedPcm.size() < minBytes) return;

    QByteArray chunk = m_recordedPcm;
    m_recordedPcm.clear();
    const double durationSec = chunk.size() / (16000.0 * 2.0);
    qDebug() << "[API] Live chunk: sending segment, duration_sec=" << durationSec << "bytes=" << chunk.size();
    QString path = QDir::tempPath() + QString("/qt_asr_live_%1.wav")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    if (!writeWavFile(path, chunk)) return;

    m_liveChunkSending = true;
    m_liveChunkTempPath = path;
    m_liveChunkStartTime = QDateTime::currentDateTime();
    m_liveChunkDurationSec = durationSec;
    m_liveSegmentIndex++;
    QString lang = transcribeLanguageFromCombo(ui->comboMicLanguage);
    m_liveChunkReply = postTranscribe(path, lang);
    if (m_liveChunkReply)
        connect(m_liveChunkReply, &QNetworkReply::finished, this, &MainWindow::onLiveChunkTranscribeFinished);
    else {
        m_liveChunkSending = false;
        QFile::remove(path);
    }
}

void MainWindow::onLiveChunkTimer()
{
    sendLiveChunk();
}

void MainWindow::onLiveChunkTranscribeFinished()
{
    QNetworkReply *reply = m_liveChunkReply;
    m_liveChunkReply = nullptr;
    m_liveChunkSending = false;
    if (!reply) return;

    const qint64 durationMs = m_liveChunkStartTime.msecsTo(QDateTime::currentDateTime());
    reply->deleteLater();

    const QString path = m_liveChunkTempPath;
    QFile::remove(path);

    QByteArray json = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(json, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            QString serverErr = errDoc.object().value(QStringLiteral("error")).toString();
            if (!serverErr.isEmpty()) errMsg = serverErr;
        }
        qDebug() << "[API] Live chunk response: error" << reply->error() << errMsg;
        ui->labelMicStatus->setText(tr("本段失败: %1").arg(errMsg));
        return;
    }
    qDebug() << "[API] Live chunk response: status=ok, body_size=" << json.size();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "[API] Live chunk response: JSON parse error" << err.errorString();
        ui->labelMicStatus->setText(tr("本段解析失败"));
        return;
    }
    QJsonObject obj = doc.object();
    const QString fileName = tr("实时转写 第%1段").arg(m_liveSegmentIndex);
    appendMicLogRow(fileName, m_liveChunkDurationSec, m_liveChunkStartTime, durationMs);
    if (obj.value(QStringLiteral("success")).toBool()) {
        QString text = obj.value(QStringLiteral("text")).toString().trimmed();
        qDebug() << "[API] Live chunk response: success=true, segment=" << m_liveSegmentIndex << "text_length=" << text.size();
        if (!text.isEmpty()) {
            QString existing = ui->textMicResult->toPlainText();
            ui->textMicResult->setPlainText(existing.isEmpty() ? text : existing + text);
            ui->labelMicStatus->setText(tr("已追加本段"));
        } else {
            ui->labelMicStatus->setText(tr("本段无识别结果"));
        }
    } else {
        QString errMsg = obj.value(QStringLiteral("error")).toString();
        qDebug() << "[API] Live chunk response: success=false, error=" << errMsg;
        ui->labelMicStatus->setText(tr("本段失败: %1").arg(errMsg));
    }
}

void MainWindow::stopLiveTranscribe()
{
    qDebug() << "[UI] Stop live transcribe, remaining PCM size=" << m_recordedPcm.size();
    m_liveChunkTimer->stop();
    ui->comboLiveChunkSeconds->setEnabled(true);
    if (m_recordedPcm.size() >= 16000)  // 发送最后一段
        sendLiveChunk();
    m_audioCollector->stopCapture();
    m_isLiveRunning = false;
    ui->btnMicRecord->setText(tr("开始实时转写"));
    ui->btnMicRecord->setChecked(false);
    ui->labelMicStatus->setText(tr("已停止。可再次点击「开始实时转写」。"));
    startPreview();
}

void MainWindow::loadApiConfig()
{
    m_apiBaseUrl = QStringLiteral("http://localhost:8000");
    QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/config.ini");
    QSettings ini(path, QSettings::IniFormat);
    ini.beginGroup(QStringLiteral("server"));
    if (ini.contains(QStringLiteral("api_base_url"))) {
        QString url = ini.value(QStringLiteral("api_base_url")).toString().trimmed();
        if (!url.isEmpty()) m_apiBaseUrl = url;
    }
    ini.endGroup();
    qDebug() << "[Config] API base URL:" << m_apiBaseUrl << "config_file=" << path;
}

QNetworkReply *MainWindow::postTranscribe(const QString &filePath, const QString &language)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[API] postTranscribe: failed to open file" << filePath;
        return nullptr;
    }
    QByteArray audioData = file.readAll();
    file.close();
    if (audioData.isEmpty()) {
        qDebug() << "[API] postTranscribe: file empty" << filePath;
        return nullptr;
    }

    QUrl url(m_apiBaseUrl + QStringLiteral("/transcribe"));
    qDebug() << "[API] POST transcribe: url=" << url.toString() << "file=" << QFileInfo(filePath).fileName() << "language=" << language << "body_size=" << audioData.size();
    QNetworkRequest request(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"audio\"; filename=\"%1\"")
            .arg(QFileInfo(filePath).fileName())));
    audioPart.setBody(audioData);
    multiPart->append(audioPart);

    QHttpPart langPart;
    langPart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"language\"")));
    langPart.setBody(language.toUtf8());
    multiPart->append(langPart);

    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    qDebug() << "[API] Request sent, reply=" << (reply ? "ok" : "null");
    return reply;
}

// 用 Unicode 码点避免源文件编码导致“秒”“毫秒”乱码
static QString formatDurationSec(double sec)
{
    if (sec <= 0 || qIsNaN(sec)) return QStringLiteral("-");
    const QChar secChar(0x79D2);  // 秒
    if (sec < 60) return QString::number(sec, 'f', 1) + secChar;
    int m = static_cast<int>(sec) / 60;
    int s = static_cast<int>(sec) % 60;
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

// 耗时：毫秒转成可读时长 MM:SS（如 02:34）
static QString formatDurationMs(qint64 ms)
{
    if (ms <= 0) return QStringLiteral("-");
    const qint64 totalSec = ms / 1000;
    const int m = static_cast<int>(totalSec / 60);
    const int s = static_cast<int>(totalSec % 60);
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

void MainWindow::appendUploadLogRow(const QString &fileName, double durationSec,
    const QDateTime &startTime, qint64 durationMs)
{
    const int row = 0;
    ui->tableUploadLog->insertRow(row);
    const int rows = ui->tableUploadLog->rowCount();
    ui->tableUploadLog->setItem(row, 0, new QTableWidgetItem(QString::number(rows)));
    ui->tableUploadLog->setItem(row, 1, new QTableWidgetItem(fileName));
    ui->tableUploadLog->setItem(row, 2, new QTableWidgetItem(formatDurationSec(durationSec)));
    ui->tableUploadLog->setItem(row, 3, new QTableWidgetItem(startTime.toString(QStringLiteral("hh:mm:ss"))));
    ui->tableUploadLog->setItem(row, 4, new QTableWidgetItem(formatDurationMs(durationMs)));
    ui->tableUploadLog->setItem(row, 5, new QTableWidgetItem(startTime.addMSecs(durationMs).toString(QStringLiteral("hh:mm:ss"))));
}

QString MainWindow::transcribeLanguageFromCombo(QComboBox *combo)
{
    QString t = combo->currentText().trimmed().toLower();
    if (t.isEmpty() || t == QStringLiteral("none")) return QStringLiteral("auto");
    return t;
}

void MainWindow::appendMicLogRow(const QString &fileName, double durationSec,
    const QDateTime &startTime, qint64 durationMs)
{
    const int row = 0;
    ui->tableMicLog->insertRow(row);
    const int rows = ui->tableMicLog->rowCount();
    ui->tableMicLog->setItem(row, 0, new QTableWidgetItem(QString::number(rows)));
    ui->tableMicLog->setItem(row, 1, new QTableWidgetItem(fileName));
    ui->tableMicLog->setItem(row, 2, new QTableWidgetItem(formatDurationSec(durationSec)));
    ui->tableMicLog->setItem(row, 3, new QTableWidgetItem(startTime.toString(QStringLiteral("hh:mm:ss"))));
    ui->tableMicLog->setItem(row, 4, new QTableWidgetItem(formatDurationMs(durationMs)));
    ui->tableMicLog->setItem(row, 5, new QTableWidgetItem(startTime.addMSecs(durationMs).toString(QStringLiteral("hh:mm:ss"))));
}

void MainWindow::onTranscribeUploadClicked()
{
    qDebug() << "[UI] Button: Transcribe upload, file=" << m_uploadFilePath;
    if (m_uploadFilePath.isEmpty()) {
        ui->labelUploadStatus->setText(tr("请先选择音频文件"));
        return;
    }
    ui->btnTranscribeUpload->setEnabled(false);
    ui->uploadProgressWrap->setVisible(true);
    ui->labelUploadStatus->setText(tr("转写中…"));

    m_transcribeStartTime = QDateTime::currentDateTime();
    m_transcribeFileName = QFileInfo(m_uploadFilePath).fileName();
    m_transcribeDurationSec = 0;
    if (m_uploadFileDurationMs > 0)
        m_transcribeDurationSec = m_uploadFileDurationMs / 1000.0;
    else if (m_uploadPlayer->duration() > 0)
        m_transcribeDurationSec = m_uploadPlayer->duration() / 1000.0;
    // 与网页端一致：有 duration 时按 1.2 倍估算并显示 " | 预计约 X 分钟"，无 duration 则不显示预估
    QString estStr;
    if (m_transcribeDurationSec > 0) {
        const int estSec = qMax(1, static_cast<int>(m_transcribeDurationSec * 1.2));
        estStr = (estSec >= 60)
            ? tr("预计约 %1 分钟").arg(qMax(1, (estSec + 59) / 60))
            : tr("预估约 %1 秒").arg(estSec);
    }
    ui->labelUploadProgressText->setText(estStr.isEmpty()
        ? tr("转写中… 已等待 0:00")
        : tr("转写中… 已等待 0:00 | %1").arg(estStr));

    QString lang = transcribeLanguageFromCombo(ui->comboUploadLanguage);
    m_uploadTranscribeReply = postTranscribe(m_uploadFilePath, lang);
    if (!m_uploadTranscribeReply) {
        ui->uploadProgressWrap->setVisible(false);
        ui->btnTranscribeUpload->setEnabled(true);
        ui->labelUploadStatus->setText(tr("无法读取文件"));
        return;
    }
    connect(m_uploadTranscribeReply, &QNetworkReply::finished, this, &MainWindow::onUploadTranscribeFinished);
    m_transcribeProgressTimer->start(1000);
}

void MainWindow::onTranscribeProgressTick()
{
    const qint64 elapsedSec = m_transcribeStartTime.secsTo(QDateTime::currentDateTime());
    const int m = static_cast<int>(elapsedSec / 60);
    const int s = static_cast<int>(elapsedSec % 60);
    const QString elapsedStr = QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    QString estStr;
    if (m_transcribeDurationSec > 0) {
        const int estSec = qMax(1, static_cast<int>(m_transcribeDurationSec * 1.2));
        estStr = (estSec >= 60)
            ? tr("预计约 %1 分钟").arg(qMax(1, (estSec + 59) / 60))
            : tr("预估约 %1 秒").arg(estSec);
    }
    const QString msg = estStr.isEmpty()
        ? tr("转写中… 已等待 %1").arg(elapsedStr)
        : tr("转写中… 已等待 %1 | %2").arg(elapsedStr).arg(estStr);
    if (m_uploadTranscribeReply)
        ui->labelUploadProgressText->setText(msg);
    else if (m_micTranscribeReply)
        ui->labelMicProgressText->setText(msg);
}

void MainWindow::onUploadTranscribeFinished()
{
    m_transcribeProgressTimer->stop();
    ui->uploadProgressWrap->setVisible(false);
    ui->btnTranscribeUpload->setEnabled(true);
    QNetworkReply *reply = m_uploadTranscribeReply;
    m_uploadTranscribeReply = nullptr;
    if (!reply) return;

    const qint64 durationMs = m_transcribeStartTime.msecsTo(QDateTime::currentDateTime());
    reply->deleteLater();

    QByteArray json = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(json, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            QString serverErr = errDoc.object().value(QStringLiteral("error")).toString();
            if (!serverErr.isEmpty()) errMsg = serverErr;
        }
        qDebug() << "[API] Upload transcribe response: error" << reply->error() << errMsg;
        ui->textUploadResult->setPlainText(tr("转录失败: %1").arg(errMsg));
        ui->labelUploadStatus->setText(tr("转录失败: %1").arg(errMsg));
        return;
    }
    qDebug() << "[API] Upload transcribe response: status=ok, body_size=" << json.size();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "[API] Upload transcribe response: JSON parse error" << err.errorString();
        ui->labelUploadStatus->setText(tr("解析响应失败"));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("success")).toBool()) {
        QString text = obj.value(QStringLiteral("text")).toString();
        QString lang = obj.value(QStringLiteral("language")).toString();
        qDebug() << "[API] Upload transcribe response: success=true, language=" << lang << "text_length=" << text.size();
        ui->textUploadResult->setPlainText(text);
        ui->labelUploadStatus->setText(tr("转录完成 (语言: %1)").arg(lang));
        appendUploadLogRow(m_transcribeFileName, m_transcribeDurationSec, m_transcribeStartTime, durationMs);
    } else {
        QString errMsg = obj.value(QStringLiteral("error")).toString();
        qDebug() << "[API] Upload transcribe response: success=false, error=" << errMsg;
        ui->textUploadResult->setPlainText(tr("转录失败: %1").arg(errMsg));
        ui->labelUploadStatus->setText(tr("转录失败: %1").arg(errMsg));
    }
}

void MainWindow::startMicTranscribe()
{
    qDebug() << "[UI] Start mic transcribe (record mode), path=" << m_micRecordPath;
    if (m_micRecordPath.isEmpty()) return;
    ui->micProgressWrap->setVisible(true);
    m_transcribeStartTime = QDateTime::currentDateTime();
    m_transcribeFileName = tr("录音转写");
    QFileInfo fi(m_micRecordPath);
    m_transcribeDurationSec = (fi.size() > 44) ? ((fi.size() - 44) / 2.0 / 16000.0) : 0;
    QString micEstStr;
    if (m_transcribeDurationSec > 0) {
        const int estSec = qMax(1, static_cast<int>(m_transcribeDurationSec * 1.2));
        micEstStr = (estSec >= 60)
            ? tr("预计约 %1 分钟").arg(qMax(1, (estSec + 59) / 60))
            : tr("预估约 %1 秒").arg(estSec);
    }
    ui->labelMicProgressText->setText(micEstStr.isEmpty()
        ? tr("转写中… 已等待 0:00")
        : tr("转写中… 已等待 0:00 | %1").arg(micEstStr));

    QString lang = transcribeLanguageFromCombo(ui->comboMicLanguage);
    m_micTranscribeReply = postTranscribe(m_micRecordPath, lang);
    if (!m_micTranscribeReply) {
        ui->micProgressWrap->setVisible(false);
        ui->labelMicStatus->setText(tr("转写请求失败"));
        return;
    }
    connect(m_micTranscribeReply, &QNetworkReply::finished, this, &MainWindow::onMicTranscribeFinished);
    m_transcribeProgressTimer->start(1000);
}

void MainWindow::onMicTranscribeFinished()
{
    m_transcribeProgressTimer->stop();
    ui->micProgressWrap->setVisible(false);
    QNetworkReply *reply = m_micTranscribeReply;
    m_micTranscribeReply = nullptr;
    if (!reply) return;

    const qint64 durationMs = m_transcribeStartTime.msecsTo(QDateTime::currentDateTime());
    reply->deleteLater();

    QByteArray json = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(json, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            QString serverErr = errDoc.object().value(QStringLiteral("error")).toString();
            if (!serverErr.isEmpty()) errMsg = serverErr;
        }
        qDebug() << "[API] Mic transcribe response: error" << reply->error() << errMsg;
        ui->textMicResult->setPlainText(tr("转录失败: %1").arg(errMsg));
        ui->labelMicStatus->setText(tr("转录失败: %1").arg(errMsg));
        return;
    }
    qDebug() << "[API] Mic transcribe response: status=ok, body_size=" << json.size();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "[API] Mic transcribe response: JSON parse error" << err.errorString();
        ui->labelMicStatus->setText(tr("解析响应失败"));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("success")).toBool()) {
        QString text = obj.value(QStringLiteral("text")).toString();
        QString lang = obj.value(QStringLiteral("language")).toString();
        qDebug() << "[API] Mic transcribe response: success=true, language=" << lang << "text_length=" << text.size();
        ui->textMicResult->setPlainText(text);
        ui->labelMicStatus->setText(tr("转录完成 (语言: %1)").arg(lang));
        appendMicLogRow(m_transcribeFileName, m_transcribeDurationSec, m_transcribeStartTime, durationMs);
    } else {
        QString errMsg = obj.value(QStringLiteral("error")).toString();
        qDebug() << "[API] Mic transcribe response: success=false, error=" << errMsg;
        ui->textMicResult->setPlainText(tr("转录失败: %1").arg(errMsg));
        ui->labelMicStatus->setText(tr("转录失败: %1").arg(errMsg));
    }
}
