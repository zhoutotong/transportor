#ifndef _TRANSLAYER_H_
#define _TRANSLAYER_H_

#include <iostream>
#include <thread>
#include <queue>

#define TRANSLAYER_HEAD (0xA55A)
#define TRANSLAYER_TAIL (0x5AA5)


class TransLayer
{
public:
    explicit TransLayer();
    ~TransLayer();

    void setup();
    void release();

    int sendData(uint8_t *data, size_t len);

protected:
    virtual int send2Destination(uint8_t *data, size_t len) = 0;
    virtual int recData(uint8_t *data, size_t *len) = 0;

private:
    bool mIsWorking;
    std::thread *mWorkThread;
    const size_t mMaxSizePerPkg;
    uint8_t *mRecBuf;

// mark 枚举量
typedef enum _MarkerDef{
    MarkUndef = 0,
    UnMark = 1,
    Marked = 2,
}MarkerDef;

// 传输协议头
typedef struct _Header{
    uint16_t head = TRANSLAYER_HEAD;
    MarkerDef marker;
    size_t index;
}Header __attribute__((aligned (8)));

// 传输协议尾
typedef struct _Tail {
    uint16_t tail = TRANSLAYER_TAIL;
}Tail;

// 数据缓存item
typedef struct _BufferItem {
    uint8_t *data;
    size_t size;
    size_t index;
}BufferItem;

std::vector<BufferItem> mBufferContainer;


private:
    Header mHeader;
    Tail mTail;


};

#endif // _TRANSLAYER_H_
