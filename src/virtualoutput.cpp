#include "virtualoutput.h"

#include <QDebug>
#include <libyuv.h>
#include <windows.h>

namespace
{
    const std::int32_t i420_frame_size(const std::int32_t width, const std::int32_t height)
    {
        return width * height * 3 / 2;
    }

    void rgb_to_i420(const uint8_t *rgb, uint8_t *i420, int32_t width, int32_t height)
    {
        int32_t height_ = height;
        height = std::abs(height);
        int32_t half_width = width / 2;
        int32_t half_height = height / 2;

        libyuv::ARGBToI420(
            rgb, width * 4,
            i420, width,
            i420 + width * height, half_width,
            i420 + width * height + half_width * half_height, half_width,
            width, height_);
    }

    void i420_to_nv12(const uint8_t *i420, uint8_t *nv12, int32_t width, int32_t height)
    {
        int32_t height_ = height;
        height = std::abs(height);
        int32_t half_width = width / 2;
        int32_t half_height = height / 2;

        libyuv::I420ToNV12(
            i420, width,
            i420 + width * height, half_width,
            i420 + width * height + half_width * half_height, half_width,
            nv12, width,
            nv12 + width * height, width,
            width, height_);
    }
}

#define nv12_frame_size i420_frame_size

VirtualOutput::VirtualOutput(QObject *parent) :
    QObject(parent)
{}

VirtualOutput::~VirtualOutput()
{}

bool VirtualOutput::start(const std::uint32_t width, const std::uint32_t height, const double fps)
{
    // https://github.com/obsproject/obs-studio/blob/9da6fc67/.github/workflows/main.yml#L484
    LPCWSTR guid = L"CLSID\\{A3FCE0F5-3493-419F-958A-ABA1250EC20B}";
    HKEY key = nullptr;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, guid, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        qCritical() << "OBS Virtual Camera device not found!";
        qInfo() << "Did you install OBS?";
        return false;
    }

    //_frame_fourcc = libyuv::CanonicalFourCC(fourcc);
    _frame_width = width;
    _frame_height = height;

    uint32_t out_frame_size = nv12_frame_size(width, height);
    // RGB|BGR -> I420 -> NV12
    _buffer_tmp.resize(i420_frame_size(width, height));
    _buffer_output.resize(out_frame_size);

    uint64_t interval = (uint64_t)(10000000.0 / fps);

    if (!m_queue.create(width, height, interval)) {
        qCritical() << "Virtual camera output could not be started";
        return false;
    }

    _output_running = true;
    return true;
}

void VirtualOutput::stop()
{
    if (!_output_running) {
        return;
    }

    m_queue.close();
    _output_running = false;
}

void VirtualOutput::send(const std::uint8_t *frame)
{
    if (!_output_running)
        return;

    uint8_t *tmp = _buffer_tmp.data();
    uint8_t *out_frame;

    // Always RGB
    out_frame = _buffer_output.data();
    rgb_to_i420(frame, tmp, _frame_width, _frame_height);
    i420_to_nv12(tmp, out_frame, _frame_width, _frame_height);

    // NV12 has two planes
    uint8_t *y = out_frame;
    uint8_t *uv = out_frame + _frame_width * _frame_height;

    // One entry per plane
    uint32_t linesize[2] = { _frame_width, _frame_width / 2 };
    const uint8_t *data[2] = { y, uv };

    uint64_t timestamp = get_timestamp_ns();

    m_queue.write(data, linesize, timestamp);
}

std::uint64_t VirtualOutput::get_timestamp_ns()
{
    if (!_have_clockfreq) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        _clock_freq = frequency.QuadPart;
        _have_clockfreq = true;
    }

    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    double time_val = (double)current_time.QuadPart;
    time_val *= 1000000000.0;
    time_val /= (double)_clock_freq;

    return static_cast<uint64_t>(time_val);
}