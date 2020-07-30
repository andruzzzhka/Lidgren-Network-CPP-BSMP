#pragma once

#include <string>
#include <vector>
#include "NetConstants.h"

namespace LidgrenNetwork
{
    typedef unsigned char byte;

    class NetIncomingMessage
    {
    public:
        NetMessageType msgType;
        bool isFragment;
        unsigned short sequenceNumber;

        unsigned short size;
        unsigned short sizeInBits;

        NetIncomingMessage();

        NetIncomingMessage(byte* buffer, int size);

        template <class T>
        T Read()
        {
            T buf{};

            for (int i = 0; i < sizeof(T); i++)
            {
                //buf = (T)((unsigned long long)buf << 8 | messageBuffer[positionInBytes++]);

                buf = (T)((unsigned long long)buf << 8 | messageBuffer[positionInBytes + (sizeof(T) - 1 - i)]);
            }

            positionInBytes += sizeof(T);
            positionInBits += 8 * sizeof(T);

            return buf;
        }

        unsigned int ReadVariableUInt32();
        std::string ReadString();
        bool ReadBoolean();

        int SkipPadBits();

        void RewindMessage();

    private:
        std::vector<byte> messageBuffer;

        int positionInBytes;
        int positionInBits;
    };
}