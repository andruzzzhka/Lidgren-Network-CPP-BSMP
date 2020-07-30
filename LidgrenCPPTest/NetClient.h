#pragma once

#include <thread>

#include "NetIncomingMessage.h"
#include "NetOutgoingMessage.h"
#include "NetConstants.h"
#include "ThreadSafeQueue.cpp"

namespace LidgrenNetwork
{

    class NetClient
    {
    public:
        NetConnectionStatus status;
        std::string appId;

        NetClient(std::string appId);
        ~NetClient();

        NetErrorCode OpenConnection(const char* ipAddr, unsigned short port);

        NetErrorCode SendConnectMessage(NetOutgoingMessage& hailMessage);
        NetErrorCode SendUserMessage(NetOutgoingMessage& message);

        NetErrorCode GetNextUserMessage(NetIncomingMessage& msg);
        NetErrorCode TryGetUserMessage(NetIncomingMessage& msg);


    private:
        int socketId;
        std::thread netThread;

        byte pingCount;

        unsigned long long uniqueID;
        float timeSinceStart;

        std::chrono::high_resolution_clock::time_point appStartClock;

        ThreadSafeQueue<NetOutgoingMessage> sendQueue;
        ThreadSafeQueue<NetIncomingMessage> recvQueue;

        std::vector<std::pair<NetOutgoingMessage, float>> msgsWaitingForAck;

        unsigned short sentUserSequenceNumbers[98];
        unsigned short recvUserSequenceNumbers[98];

#ifdef WIN32
        #pragma comment(lib,"ws2_32.lib")
        void ClearWinSock();
#endif
        void DebugPrint(const char* message);
        bool SetSocketBlockingEnabled(int fd, bool blocking);

        NetErrorCode SendLibraryMessage(NetOutgoingMessage& message);

        void NetLoopThread();
        void ProcessReceivedMessage(NetIncomingMessage& msg);
    };
}