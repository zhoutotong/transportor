#include "translayer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_REC_BUF (1024 * 10)
#define MAX_SIG_REC_BUF (1024)

TransLayer::TransLayer() : mIsWorking(false), mWorkThread(nullptr), mMaxSizePerPkg(512), mRecBuf(new uint8_t[MAX_REC_BUF])
{
    mHeader.marker = UnMark;
    mHeader.index = 0;
}

TransLayer::~TransLayer()
{
    for (auto itor = mBufferContainer.begin(); itor != mBufferContainer.end(); itor++)
    {
        delete itor->data;
    }
}

void TransLayer::setup()
{
    if (mIsWorking || mWorkThread)
        return;
    mIsWorking = true;
    mWorkThread = new std::thread([this] {
        bool getHeader = false;
        size_t recSize = 0;
        size_t procIndex = 0;
        Header h;
        Tail t;

        while (mIsWorking)
        {
            usleep(10);
            uint8_t *pbuf = new uint8_t[MAX_SIG_REC_BUF];
            size_t recLen = 0;
            if (!pbuf)
                return;
            recRawData(pbuf, &recLen);
            if (recLen == 0)
                continue;
            procIndex = 0; // 使用索引来解决粘包问题

            // while (procIndex < recLen)
            {
                // 检查 tail，只有在检查到 header 的时候检查 tail
                for (int i = procIndex; i < recLen - 1; i++)
                {
                    uint16_t check = pbuf[i + 1] << 8 | pbuf[i];
                    if (check == TRANSLAYER_TAIL)
                    {
                        memcpy((void *)&t, &pbuf[i], sizeof(Tail));
                        if (i != 0 && getHeader) // 如果 tail 没有在这个包的头，那么这之前的数据也是数据本身，也应放入数据缓存区
                        {
                            memcpy(&mRecBuf[recSize], pbuf, i);
                            recSize += i;
                        }

                        if (getHeader && h.marker == Marked)
                        {
                            // 读到 marked 标志，说明出现完整包
                            recData(mRecBuf, recSize);
                            recSize = 0;
                            bzero(&h, sizeof(TRANSLAYER_HEAD));
                            bzero(&t, sizeof(TRANSLAYER_TAIL));
                            procIndex = i + 2;
                        }
                        getHeader = false;

                        break;
                    }
                }

                if (getHeader)
                { // 如果已经检查到 header,并且没有检查到 tail，那么此处数据为数据本身
                    if (recSize + recLen <= MAX_REC_BUF)
                    {
                        memcpy(&mRecBuf[recSize], &pbuf[procIndex], recLen - procIndex);
                        recSize += (recLen - procIndex);
                        procIndex += (recLen - procIndex);
                    }
                    else
                    {
                        std::cout << "rec package size is to large!!!" << std::endl;
                    }
                    continue;
                }

                // 检查 header
                for (int i = procIndex; i < recLen - 1; i++)
                {
                    uint16_t check = pbuf[i + 1] << 8 | pbuf[i];
                    if (check == TRANSLAYER_HEAD)
                    {
                        memcpy((uint8_t *)&h, &pbuf[i], sizeof(Header));
                        getHeader = true;
                        // 处理 header 粘包问题
                        size_t dataPos = i + sizeof(Header);
                        if (dataPos < recLen)
                        {
                            memcpy(&mRecBuf[recSize], &pbuf[dataPos], recLen - (dataPos));
                            recSize += dataPos;
                        }
                        break;
                    }
                }
            }

            delete[] pbuf;
        }
    });
}
void TransLayer::release()
{
    mIsWorking = false;
    if (mWorkThread)
    {
        mWorkThread->join();
        delete mWorkThread;
        mWorkThread = nullptr;
    }
}

int TransLayer::sendData(uint8_t *data, size_t len)
{
    // 超出MAX_PACKAGE_SIZE时应该分包发送
    int times = len / mMaxSizePerPkg;
    size_t last = len % mMaxSizePerPkg;
    mHeader.head = TRANSLAYER_HEAD;
    mTail.tail = TRANSLAYER_TAIL;
    if (times == 0)
    {
        mHeader.marker = Marked;
        mHeader.index++;
        // 发送头部
        send2Destination((uint8_t *)(&mHeader), sizeof(Header));
        // 发送数据体
        send2Destination(data, len);
        // 发送尾部
        send2Destination((uint8_t *)&mTail, sizeof(Tail));
    }
    else
    {
        mHeader.marker = UnMark;
        // 发送头部
        send2Destination((uint8_t *)(&mHeader), sizeof(Header));
        // 发送数据体
        send2Destination(data, mMaxSizePerPkg);
        // 发送尾部
        send2Destination((uint8_t *)&mTail, sizeof(Tail));

        for (int i = 1; i < times; i++)
        {
            // 最后一帧时，检查是否还有剩余的需要发送，有的话 UnMark,没有的话就Marked
            mHeader.marker = (i != times - 1) ? UnMark : ((last > 0) ? UnMark : Marked);
            if (mHeader.marker == Marked)
                mHeader.index++;
            // 发送头部
            send2Destination((uint8_t *)(&mHeader), sizeof(Header));
            // 发送数据体
            send2Destination((data + i * mMaxSizePerPkg), mMaxSizePerPkg);
            // 发送尾部
            send2Destination((uint8_t *)&mTail, sizeof(Tail));
        }
        if (last > 0)
        {
            mHeader.marker = Marked;
            mHeader.index++;

            // 发送头部
            send2Destination((uint8_t *)(&mHeader), sizeof(Header));
            // 发送数据体
            send2Destination((data + len - last), last);
            // 发送尾部
            send2Destination((uint8_t *)&mTail, sizeof(Tail));
        }
    }
}
