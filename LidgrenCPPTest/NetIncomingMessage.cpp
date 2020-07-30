#include "LidgrenNetwork.h"

namespace LidgrenNetwork
{
    NetIncomingMessage::NetIncomingMessage() : msgType(NetMessageType::Unconnected), isFragment(false), sequenceNumber(0), messageBuffer(0), size(0), positionInBytes(0), positionInBits(0)
    {

    }

    NetIncomingMessage::NetIncomingMessage(byte* buffer, int size) : positionInBytes(0), positionInBits(0)
    {
        msgType = (NetMessageType)buffer[0];
        isFragment = (bool)(buffer[1] & 0x01);
        sequenceNumber = ((unsigned short)((buffer[2] & 0x7F) << 7) | (buffer[1] >> 1 & 0x7F));

        byte size1 = buffer[4];
        byte size2 = buffer[3];
        sizeInBits = ((unsigned short)(size1 << 8)) | ((unsigned short)size2);

        this->size = NetMath::RoundIntegerUp(sizeInBits, 8) / 8;

        messageBuffer = std::vector<byte>(this->size);
        std::memcpy(messageBuffer.data(), buffer + 5, this->size);
    }

    unsigned int NetIncomingMessage::ReadVariableUInt32()
    {
        int num1 = 0;
        int num2 = 0;
        while (size * 8 - positionInBytes * 8 >= 8)
        {
            byte num3 = Read<byte>();
            num1 |= (num3 & 0x7f) << num2;
            num2 += 7;
            if ((num3 & 0x80) == 0)
                return (unsigned int)num1;
        }

        // ouch; failed to find enough bytes; malformed variable length number?
        return (unsigned int)num1;
    }

    std::string NetIncomingMessage::ReadString()
    {
        byte size = Read<byte>();
        if (size < 128)
        {
            std::string result(size, ' ');

            for (int i = 0; i < size; i++)
            {
                result[i] = Read<char>();
            }

            return result;
        }
        else
        {
            throw std::runtime_error("Strings longer than 127 characters are not yet supported");
        }
    }

    bool NetIncomingMessage::ReadBoolean()
    {
        byte retval = messageBuffer[positionInBytes];
        retval = retval >> positionInBits & 0x01;
        positionInBits += 1;

        while (positionInBits >= 8)
        {
            positionInBytes++;
        }

        return (retval > 0 ? true : false);
    }

    int NetIncomingMessage::SkipPadBits()
    {
        for (int i = positionInBits; i < 8; i++)
        {
            ReadBoolean();
        }

        return 1;
    }

    void NetIncomingMessage::RewindMessage()
    {
        positionInBits = 0;
        positionInBytes = 0;
    }
}