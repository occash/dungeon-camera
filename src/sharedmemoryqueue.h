#pragma once

#include <QObject>

struct VideoQueue;

class SharedMemoryQueue  : public QObject
{
    Q_OBJECT

public:
    SharedMemoryQueue(QObject *parent = nullptr);
    ~SharedMemoryQueue();

    bool create(
        const std::uint32_t cx,
        const std::uint32_t cy,
        const std::uint64_t interval
    );
    void close();
    void write(
        const std::uint8_t **data,
        const std::uint32_t *linesize,
        const std::uint64_t timestamp
    );

private:
    VideoQueue *vq;

};
