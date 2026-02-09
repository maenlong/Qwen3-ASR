#ifndef AUDIOCOLLECTOR_H
#define AUDIOCOLLECTOR_H

#include <QObject>
#include <QAudioInput>
#include <QIODevice>
#include <QAudioFormat>
#include <QStringList>
#include <QByteArray>
#include <QAudioDeviceInfo>

class AudioCollector : public QObject
{
    Q_OBJECT

public:
    explicit AudioCollector(QObject *parent = nullptr);
    ~AudioCollector();

    QStringList listAvailableDevices();
    bool selectDevice(const QString &deviceName);
    void startCapture();
    void stopCapture();

    static int sampleRate() { return 16000; }
    static int channelCount() { return 1; }
    static int sampleSize() { return 16; }

signals:
    void audioDataCaptured(const QByteArray &data);

private:
    QAudioInput *audioInput;
    QIODevice *audioDevice;
    QAudioFormat format;
    QString selectedDevice;
};

#endif
