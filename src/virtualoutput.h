#pragma once

#include "sharedmemoryqueue.h"

#include <QObject>

class VirtualOutput  : public QObject
{
    Q_OBJECT

public:
    VirtualOutput(QObject *parent = nullptr);
    ~VirtualOutput();

    bool isStarted() const { return _output_running; }
    bool start(
        const std::uint32_t width,
        const std::uint32_t height,
        const double fps
    );
    void stop();
    void send(const std::uint8_t *frame);

private:
    std::uint64_t get_timestamp_ns();

private:
    SharedMemoryQueue m_queue;
    bool _output_running = false;
    std::uint32_t _frame_width;
    std::uint32_t _frame_height;
    //std::uint32_t _frame_fourcc; Always RGB
    std::vector<uint8_t> _buffer_tmp;
    std::vector<uint8_t> _buffer_output;
    bool _have_clockfreq = false;
    long long _clock_freq;

};
