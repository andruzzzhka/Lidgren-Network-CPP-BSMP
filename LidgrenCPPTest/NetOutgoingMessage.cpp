#include "LidgrenNetwork.h"

namespace LidgrenNetwork
{
    NetOutgoingMessage::NetOutgoingMessage() : msgType(NetMessageType::Unconnected), isFragment(false), sequenceNumber(0), positionInBits(5 * 8), messageBuffer(5)
    {
    }

    byte* NetOutgoingMessage::GetMessageBytes(int& size)
    {
        unsigned short sizeInBits = positionInBits - 5 * 8;

        messageBuffer[0] = (byte)msgType;

        byte fragAndSeq2 = (isFragment ? 1 : 0) | (sequenceNumber << 1 & 0xFE);

        messageBuffer[1] = fragAndSeq2;
        messageBuffer[2] = (sequenceNumber >> 8) & 0xFF;

        byte* sizeInBits_Bytes = (byte*)&sizeInBits;
        messageBuffer[3] = sizeInBits_Bytes[0];
        messageBuffer[4] = sizeInBits_Bytes[1];

        size = messageBuffer.size();
        return messageBuffer.data();
    }

    /*
    int NetOutgoingMessage::WriteByte(byte val)
    {
        messageBuffer.push_back(val);
        positionInBits += 8;
        return 1;
    }
    */

    int NetOutgoingMessage::WriteString(std::string val)
    {
        if (val.size() < 128)
        {
            messageBuffer.push_back(val.size());
            for (int i = 0; i < val.size(); i++)
            {
                messageBuffer.push_back(val[i]);
            }

            positionInBits += (val.size() + 1) * 8;

            return val.size() + 1;
        }
        else
        {
            throw std::runtime_error("Strings longer than 127 characters are not yet supported");
        }
    }

    int NetOutgoingMessage::WriteMessage(NetOutgoingMessage val)
    {
        int size;
        byte* msgBuffer = val.GetMessageBytes(size);

        for (int i = 5; i < size; i++)
            messageBuffer.push_back(msgBuffer[i]);

        positionInBits += val.positionInBits - 5 * 8;

        return size;
    }

    int NetOutgoingMessage::WriteVariableUInt32(unsigned int value)
    {
        int retval = 1;
        unsigned int num1 = (unsigned int)value;
        while (num1 >= 0x80)
        {
            this->Write((byte)(num1 | 0x80));
            num1 = num1 >> 7;
            retval++;
        }
        this->Write((byte)num1);

        return retval;
    }

    int NetOutgoingMessage::WriteBoolean(bool val)
    {
        if (positionInBits % 8 == 0)
        {
            byte buf = val ? 0x01 : 0x00;
            positionInBits += 1;

            messageBuffer.push_back(buf);

            return 1;
        }
        else
        {
            int index = messageBuffer.size() - 1;

            byte buf = messageBuffer[index];

            buf = buf | ((val & 0x01) << (positionInBits % 8));
            positionInBits += 1;

            messageBuffer[index] = buf;

            return 0;
        }
    }

    int NetOutgoingMessage::WritePadBits()
    {
        if (positionInBits % 8 == 0)
        {
            return 0;
        }
        else
        {

            for (int i = positionInBits % 8; i < 8; i++)
            {
                WriteBoolean(false);
            }

            return 1;
        }
    }
}