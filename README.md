# transportor
与通信方式无关的数据传输工具

## 使用说明

### translayer

> translayer 作为传输层，目的在于解决通信过程中的单包大小的限制，使用时将这个类作为父类，实现其中的三个虚函数。

* 要实现的三个虚函数：

```C++
int send2Destination(uint8_t *data, size_t len)
int recRawData(uint8_t *data, size_t *len)
void recData(uint8_t *data, size_t len)
```
|接口名称|解释|
|-|-|
|send2Destination|接口实现底层的传输功能，实现实际的数据发送，data为待发送的数据指针，len为待发送的数据大小，该大小已在代码中进行了限制，不超过512，可通过修改mMaxSizePerPkg的值来修改单包的大小限制|
|recRawData|接口实现底层的接收，数据接收到后，将数据拷贝值data指针指向的缓存区，并将接收到的数据大小写入len指向的内存|
|recData|数据进行拼包等处理后的接收接口，数据将保存在data指向的缓存区中，len指示接收的数据大小|

示例实现：

```C++
#include <iostream>
#include "src/translayer.h"

#include <thread>
#include <unistd.h>
#include <string.h>
#include <mutex>

class MyTransfor : public TransLayer
{
public:
    MyTransfor() : TransLayer() {}
    ~MyTransfor()
    {
        if (dataBuf)
        {
            delete[] dataBuf;
        }
    }

    std::mutex dataMutex;
    uint8_t *dataBuf = new uint8_t[1024];
    size_t recLen = 0;
    bool sendOk = false;

protected:
    int send2Destination(uint8_t *data, size_t len) override
    {
        dataMutex.lock();
        if (sendOk)
        {
            memcpy(dataBuf, data, len);
            recLen = len;
            sendOk = false;
        }

        dataMutex.unlock();
        usleep(1000);
    };
    int recRawData(uint8_t *data, size_t *len) override
    {
        dataMutex.lock();
        memcpy(data, dataBuf, recLen);
        *len = recLen;
        recLen = 0;
        sendOk = true;
        dataMutex.unlock();
    };
    void recData(uint8_t *data, size_t len) override
    {
        std::cout << "read available data: " << len << std::endl;
        std::string msg;
        msg.insert(0, (char *)data, len);
        std::cout << "msg: " << msg << std::endl;
    }
};

```

