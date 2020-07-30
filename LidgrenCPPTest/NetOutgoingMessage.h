#pragma once

#include <string>
#include <vector>
#include "NetConstants.h"

namespace LidgrenNetwork
{
    typedef unsigned char byte;

    class NetOutgoingMessage
    {
    public:
        NetMessageType msgType;
        bool isFragment;
        unsigned short sequenceNumber;

        int positionInBits;

        NetOutgoingMessage();

        byte* GetMessageBytes(int& size);

        int WriteString(std::string val);
        int WriteMessage(NetOutgoingMessage val);
        
        template <class T> int Write(T val)
        {
            byte* valBytes = (byte*)&val;
            for (int i = 0; i < sizeof(T); i++)
            {
                messageBuffer.push_back(valBytes[i]);
            }

            positionInBits += sizeof(T) * 8;
            return sizeof(T);
        }

        int WriteVariableUInt32(unsigned int value);
        int WriteBoolean(bool val);

        int WritePadBits();

    private:
        std::vector<byte> messageBuffer;
    };
}