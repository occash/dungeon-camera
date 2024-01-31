#include "sharedmemoryqueue.h"

#include <windows.h>

#define VIDEO_NAME L"OBSVirtualCamVideo"

enum QueueType
{
    SHARED_QUEUE_TYPE_VIDEO,
};

enum QueueState
{
    SHARED_QUEUE_STATE_INVALID,
    SHARED_QUEUE_STATE_STARTING,
    SHARED_QUEUE_STATE_READY,
    SHARED_QUEUE_STATE_STOPPING,
};

struct QueueHeader
{
    volatile std::uint32_t write_idx;
    volatile std::uint32_t read_idx;
    volatile std::uint32_t state;

    std::uint32_t offsets[3];

    std::uint32_t type;

    std::uint32_t cx;
    std::uint32_t cy;
    std::uint64_t interval;

    std::uint32_t reserved[8];
};

struct VideoQueue
{
    HANDLE handle;
    bool ready_to_read;
    QueueHeader *header;
    std::uint64_t *ts[3];
    std::uint8_t *frame[3];
    bool is_writer;
};

#define ALIGN_SIZE(size, align) size = (((size) + (align - 1)) & (~(align - 1)))
#define FRAME_HEADER_SIZE 32

SharedMemoryQueue::SharedMemoryQueue(QObject *parent) :
    QObject(parent),
    vq{ new VideoQueue() }
{}

SharedMemoryQueue::~SharedMemoryQueue()
{
    delete vq;
}

bool SharedMemoryQueue::create(const std::uint32_t cx, const std::uint32_t cy, const std::uint64_t interval)
{
    DWORD frame_size = cx * cy * 3 / 2;
    uint32_t offset_frame[3]{ 0 };
    DWORD size = sizeof(QueueHeader);

    ALIGN_SIZE(size, 32);

    offset_frame[0] = size;
    size += frame_size + FRAME_HEADER_SIZE;
    ALIGN_SIZE(size, 32);

    offset_frame[1] = size;
    size += frame_size + FRAME_HEADER_SIZE;
    ALIGN_SIZE(size, 32);

    offset_frame[2] = size;
    size += frame_size + FRAME_HEADER_SIZE;
    ALIGN_SIZE(size, 32);

    struct QueueHeader header = { 0 };

    header.state = SHARED_QUEUE_STATE_STARTING;
    header.cx = cx;
    header.cy = cy;
    header.interval = interval;
    vq->is_writer = true;

    for (size_t i = 0; i < 3; i++) {
        uint32_t off = offset_frame[i];
        header.offsets[i] = off;
    }

    /* fail if already in use */
    /*vq->handle = OpenFileMappingW(FILE_MAP_READ, false, VIDEO_NAME);
    
    if (vq->handle) {
        CloseHandle(vq->handle);
        return false;
    }*/

    vq->handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, size, VIDEO_NAME);
    
    if (!vq->handle) {
        return false;
    }

    vq->header = (QueueHeader *)MapViewOfFile(vq->handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (!vq->header) {
        CloseHandle(vq->handle);
        return false;
    }

    memcpy(vq->header, &header, sizeof(header));

    for (size_t i = 0; i < 3; i++) {
        uint32_t off = offset_frame[i];
        vq->ts[i] = (uint64_t *)(((uint8_t *)vq->header) + off);
        vq->frame[i] = ((uint8_t *)vq->header) + off + FRAME_HEADER_SIZE;
    }

    return true;
}

void SharedMemoryQueue::close()
{
    if (!vq) {
        return;
    }

    if (vq->is_writer) {
        vq->header->state = SHARED_QUEUE_STATE_STOPPING;
    }

    UnmapViewOfFile(vq->header);
    CloseHandle(vq->handle);
}

#define get_idx(inc) ((unsigned long)inc % 3)

void SharedMemoryQueue::write(const std::uint8_t **data, const std::uint32_t *linesize, const std::uint64_t timestamp)
{
    QueueHeader *qh = vq->header;
    long inc = ++qh->write_idx;

    unsigned long idx = get_idx(inc);
    size_t size = linesize[0] * qh->cy;

    *vq->ts[idx] = timestamp;
    memcpy(vq->frame[idx], data[0], size);
    memcpy(vq->frame[idx] + size, data[1], size / 2);

    qh->read_idx = inc;
    qh->state = SHARED_QUEUE_STATE_READY;
}
