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

int main(int argc, char *argv[])
{
    std::cout << "transpotor test program" << std::endl;
    int a = 10;
    MyTransfor t;
    t.setup();

    std::string msg = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123";
    size_t len = msg.size();

    sleep(1);

    while (true)
    {
        t.sendData((uint8_t *)msg.data(), len);
        sleep(1);
    }
    t.release();

    return 0;
}