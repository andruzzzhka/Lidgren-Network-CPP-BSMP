#pragma once

#include "LidgrenNetwork.h"

namespace Multiplayer
{
    typedef unsigned char byte;

    struct Color32
    {
    public:
        byte r = 0;
        byte g = 0;
        byte b = 0;
    };

    struct Vector3
    {
    public:
        float x = 0;
        float y = 0;
        float z = 0;

        void AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg)
        {
            msg.Write(x);
            msg.Write(y);
            msg.Write(z);
        }
    };

    struct Quaternion
    {
    public:
        float x = 0;
        float y = 0;
        float z = 0;
        float w = 0;

        void AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg)
        {
            msg.Write(x);
            msg.Write(y);
            msg.Write(z);
            msg.Write(w);
        }
    };
}