#include "audiocollector.h"
#include <QAudioDeviceInfo>
#include <QDebug>

AudioCollector::AudioCollector(QObject *parent)
    : QObject(parent), audioInput(nullptr), audioDevice(nullptr)
{
    format.setSampleRate(sampleRate());
    format.setChannelCount(channelCount());
    format.setSampleSize(sampleSize());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
}

AudioCollector::~AudioCollector()
{
    if (audioInput) {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }
}

QStringList AudioCollector::listAvailableDevices()
{
    QStringList list;
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        list.append(info.deviceName());
    return list;
}

bool AudioCollector::selectDevice(const QString &deviceName)
{
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == deviceName) {
            selectedDevice = deviceName;
            return true;
        }
    }
    return false;
}

void AudioCollector::startCapture()
{
    if (selectedDevice.isEmpty()) {
        qWarning() << "AudioCollector: no device selected";
        return;
    }

    QAudioDeviceInfo deviceInfo;
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == selectedDevice) {
            deviceInfo = info;
            break;
        }
    }
    if (deviceInfo.deviceName().isEmpty()) {
        qWarning() << "AudioCollector: device not found" << selectedDevice;
        return;
    }

    if (audioInput) {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }

    audioInput = new QAudioInput(deviceInfo, format, this);
    audioDevice = audioInput->start();
    connect(audioDevice, &QIODevice::readyRead, this, [this]() {
        QByteArray data = audioDevice->readAll();
        emit audioDataCaptured(data);
    });
}

void AudioCollector::stopCapture()
{
    if (audioInput) {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }
    audioDevice = nullptr;
}
