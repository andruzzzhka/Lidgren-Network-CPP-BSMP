#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "NetClient.h"
#include "NetMath.h"

namespace LidgrenNetwork
{
    NetClient::NetClient(std::string appId) : appId(appId), status(NetConnectionStatus::None), pingCount(0), socketId(-1), timeSinceStart(0), uniqueID(0), sendQueue(16), recvQueue(6400), msgsWaitingForAck()
    {
        std::fill(sentUserSequenceNumbers, sentUserSequenceNumbers + 98, 0x8000);
        std::fill(recvUserSequenceNumbers, recvUserSequenceNumbers + 98, 0x8000);
    };

    NetClient::~NetClient()
    {
        if (status != NetConnectionStatus::Disconnected)
        {
            status = NetConnectionStatus::Disconnected;
            netThread.join();
            closesocket(socketId);
#ifdef WIN32
            ClearWinSock();
#endif
        }
    }

    NetErrorCode NetClient::OpenConnection(const char* ipAddr, unsigned short port)
    {
#if defined WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
        {
            printf("Error at WSASturtup\n");
            return NetErrorCode::UnableToCreateSocket;
        }
#endif

        socketId = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socketId < 0 || !SetSocketBlockingEnabled(socketId, false))
        {
            closesocket(socketId);
#ifdef WIN32
            ClearWinSock();
#endif
            status = NetConnectionStatus::Disconnected;
            return NetErrorCode::UnableToCreateSocket;
        }

        struct sockaddr_in sad;
        memset(&sad, 0, sizeof(sad));
        sad.sin_family = AF_INET;
        sad.sin_addr.s_addr = inet_addr(ipAddr);
        sad.sin_port = htons(port);

        if (connect(socketId, (struct sockaddr*) & sad, sizeof(sad)) < 0)
        {
            closesocket(socketId);
#ifdef WIN32
            ClearWinSock();
#endif
            status = NetConnectionStatus::Disconnected;
            return NetErrorCode::UnableToOpenConnection;
        }

        status = NetConnectionStatus::InitiatedConnect;

        uniqueID = ((unsigned long long)rand()) << 32 | rand();
        timeSinceStart = 0;

        netThread = std::thread(&NetClient::NetLoopThread, this);
        //netThread.detach();

        appStartClock = std::chrono::high_resolution_clock::now();

        return NetErrorCode::OK;
    }

    NetErrorCode NetClient::SendConnectMessage(NetOutgoingMessage& hailMessage)
    {
        if (status != NetConnectionStatus::InitiatedConnect)
            return NetErrorCode::WrongStatus;

        NetOutgoingMessage message;

        message.msgType = NetMessageType::Connect;

        message.isFragment = false;
        message.sequenceNumber = 0;

        message.WriteString(appId);
        message.Write(uniqueID);
        message.Write(timeSinceStart);
        message.WriteMessage(hailMessage);

        if (std::this_thread::get_id() == netThread.get_id())
        {
            int bufferSize;
            byte* buffer = message.GetMessageBytes(bufferSize);

            if (send(socketId, (const char*)buffer, bufferSize, 0) != bufferSize)
            {
                closesocket(socketId);
#ifdef WIN32
                ClearWinSock();
#endif
                return NetErrorCode::UnableToSendMessage;
            }
        }
        else
        {
            sendQueue.push(message);
        }


        return NetErrorCode::OK;
    }

    NetErrorCode NetClient::SendUserMessage(NetOutgoingMessage& message)
    {
        message.isFragment = false;

        if (sentUserSequenceNumbers[(byte)message.msgType - 1] >= 0x8000)
        {
            sentUserSequenceNumbers[(byte)message.msgType - 1] = 0;
        }

        message.sequenceNumber = sentUserSequenceNumbers[(byte)message.msgType - 1]++;

        if (message.msgType >= NetMessageType::UserReliableUnordered && message.msgType <= NetMessageType::UserReliableOrdered32)
        {
            msgsWaitingForAck.push_back(std::make_pair(message, timeSinceStart));
        }

        if (std::this_thread::get_id() == netThread.get_id())
        {
            int bufferSize;
            byte* buffer = message.GetMessageBytes(bufferSize);

            if (send(socketId, (const char*)buffer, bufferSize, 0) != bufferSize)
            {
                closesocket(socketId);
#ifdef WIN32
                ClearWinSock();
#endif
                return NetErrorCode::UnableToSendMessage;
            }
        }
        else
        {
            sendQueue.push(message);
        }

        return NetErrorCode::OK;
    }

    NetErrorCode NetClient::GetNextUserMessage(NetIncomingMessage& msg)
    {
        if (recvQueue.pop(msg, true) == ThreadSafeQueue<NetOutgoingMessage>::QueueResult::OK)
        {
            return NetErrorCode::OK;
        }
        else
        {
            return NetErrorCode::InternalError;;
        }
    }


    NetErrorCode NetClient::TryGetUserMessage(NetIncomingMessage& msg)
    {
        auto result = recvQueue.pop(msg, false);

        if (result == ThreadSafeQueue<NetOutgoingMessage>::QueueResult::OK)
        {
            return NetErrorCode::OK;
        }
        else if (result == ThreadSafeQueue<NetOutgoingMessage>::QueueResult::EMPTY)
        {
            return NetErrorCode::QueueEmpty;
        }
        else
        {
            return NetErrorCode::InternalError;
        }
    }

#ifdef WIN32
    #pragma comment(lib,"ws2_32.lib")
    void NetClient::ClearWinSock()
    {
        WSACleanup();
    }
#endif

    void NetClient::DebugPrint(const char* message)
    {
#ifdef _DEBUG
        printf(message);
#endif
    }

    bool NetClient::SetSocketBlockingEnabled(int fd, bool blocking)
    {
        if (fd < 0) return false;

#ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
    }

    NetErrorCode NetClient::SendLibraryMessage(NetOutgoingMessage& message)
    {
        if (std::this_thread::get_id() == netThread.get_id())
        {
            int bufferSize;
            byte* buffer = message.GetMessageBytes(bufferSize);

            if (send(socketId, (const char*)buffer, bufferSize, 0) != bufferSize)
            {
                closesocket(socketId);
#ifdef WIN32
                ClearWinSock();
#endif
                return NetErrorCode::UnableToSendMessage;
            }
        }
        else
        {
            sendQueue.push(message);
        }

        return NetErrorCode::OK;
    }

    void NetClient::NetLoopThread()
    {

        int bytesRcvd;
        char buf[1400];

        while (status != NetConnectionStatus::Disconnected)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto now = std::chrono::high_resolution_clock::now();

            timeSinceStart = std::chrono::duration<float>(now - appStartClock).count();


            NetOutgoingMessage msgToSend;

            while (sendQueue.pop(msgToSend, false) == ThreadSafeQueue<NetOutgoingMessage>::QueueResult::OK)
            {
                printf("Sending message with ID 0x%x (%d)\n", (byte)msgToSend.msgType & 0xFF, msgToSend.msgType);

                int bufferSize;
                byte* buffer = msgToSend.GetMessageBytes(bufferSize);

                if (send(socketId, (const char*)buffer, bufferSize, 0) != bufferSize)
                {
                    closesocket(socketId);
#ifdef WIN32
                    ClearWinSock();
#endif
                    printf("Unable to send message from queue!\n");
                    continue;
                }
            }

            if (!msgsWaitingForAck.empty())
            {
                for (int i = 0; i < msgsWaitingForAck.size(); i++)
                {
                    if (timeSinceStart - msgsWaitingForAck[i].second > 0.2f)
                    {
                        SendLibraryMessage(msgsWaitingForAck[i].first);
                        msgsWaitingForAck[i].second = timeSinceStart;
                    }
                }
            }

            unsigned long availableData;


#ifdef WIN32
            ioctlsocket(socketId, FIONREAD, &availableData);
#else
            ioctl(socketId, FIONREAD, &availableData);
#endif
            while (availableData > 0)
            {
                if ((bytesRcvd = recv(socketId, buf, 1400, 0)) <= 0)
                {
                    int errorCode = 0;

#ifdef WIN32
                    errorCode = WSAGetLastError();
#else
                    errorCode = errno;
#endif

#if WIN32
                    if (errorCode != WSAEWOULDBLOCK)
#else
                    if (errorCode != EWOULDBLOCK && errorCode != EAGAIN)
#endif
                    {
                        printf("recv() failed or connection closed prematurely. ErrNo: %d\n", errorCode);
                        closesocket(socketId);
#ifdef WIN32
                        ClearWinSock();
#endif
                        return;
                    }
                    else
                    {
                        continue;
                    }

                }
                else
                {
                    int position = 0;


                    while (position < bytesRcvd)
                    {
                        byte size1 = buf[4 + position];
                        byte size2 = buf[3 + position];

                        unsigned short realSizeInBits = ((unsigned short)(size1 << 8)) | ((unsigned short)size2);

                        int realMsgSize = NetMath::RoundIntegerUp(realSizeInBits, 8 ) / 8;

#ifdef _VERBOSE
                        printf("Received packet with size: %d (%d)\n", realMsgSize + 5, position);
#endif

                        NetIncomingMessage msg((byte*)(buf + position), realMsgSize + 5);

                        ProcessReceivedMessage(msg);

                        position += realMsgSize + 5;
                    }

                }

#ifdef WIN32
                ioctlsocket(socketId, FIONREAD, &availableData);
#else
                ioctl(socketId, FIONREAD, &availableData);
#endif
            }

        }

    }

    void NetClient::ProcessReceivedMessage(NetIncomingMessage &msg)
    {
        if (msg.msgType >= NetMessageType::UserUnreliable && msg.msgType <= NetMessageType::UserReliableOrdered32)
        {
            //if (msg.msgType == NetMessageType::UserSequenced3) //VoIP channel
            //    return;

#if defined(_DEBUG) && defined(_VERBOSE)
            printf("Received user message: 0x%x (%d)\n", (byte)msg.msgType & 0xFF, msg.msgType);
            for (int i = 0; i < msg.size; i++)
            {
                printf("0x%x ", msg.Read<byte>() & 0xFF);
            }
            msg.RewindMessage();
            printf("\nEND\n");
#endif

            if (msg.msgType != NetMessageType::UserUnreliable && msg.msgType != NetMessageType::UserReliableUnordered)
            {
                if(recvUserSequenceNumbers[(byte)msg.msgType - 1] >= 0x8000)
                {
                    recvUserSequenceNumbers[(byte)msg.msgType - 1] = msg.sequenceNumber;
                }
                else if (recvUserSequenceNumbers[(byte)msg.msgType - 1] >= msg.sequenceNumber)
                {
                    return;
                }
                else
                {
                    recvUserSequenceNumbers[(byte)msg.msgType - 1] = msg.sequenceNumber;

                    //IMPORTANT: ReliableOrdered messages work like ReliableSequenced right now
                }
            }

            if (msg.msgType >= NetMessageType::UserReliableUnordered)
            {
                NetOutgoingMessage ackMsg;

                ackMsg.msgType = NetMessageType::Acknowledge;

                ackMsg.Write((byte)msg.msgType);
                ackMsg.Write((byte)(msg.sequenceNumber & 0xFF));
                ackMsg.Write((byte)((msg.sequenceNumber >> 8) & 0xFF));

                SendLibraryMessage(ackMsg);
            }

            recvQueue.push(msg);
        }
        else
        {
            switch (msg.msgType)
            {
            case NetMessageType::ConnectResponse:
            {
                DebugPrint("Received ConnectResponse!\n");
                status = NetConnectionStatus::Connected;

                NetOutgoingMessage response;
                response.msgType = NetMessageType::ConnectionEstablished;

                response.Write(timeSinceStart);

                SendLibraryMessage(response);

                DebugPrint("Responded with ConnectionEstablished!\n");
            }
            break;

            case NetMessageType::Ping:
            {
                pingCount = msg.Read<byte>();

                NetOutgoingMessage response;
                response.msgType = NetMessageType::Pong;

                response.Write(pingCount);
                response.Write(timeSinceStart);

                SendLibraryMessage(response);
            }
            break;

            case NetMessageType::Disconnect:
            {
                DebugPrint("Received Disconnect!\n");

                closesocket(socketId);
                status = NetConnectionStatus::Disconnected;
            }
            return;

            case NetMessageType::Acknowledge:
            {
                if (msgsWaitingForAck.empty())
                    break;

                int pos = 0;
                while (pos < msg.size)
                {
                    NetMessageType msgType = (NetMessageType)msg.Read<byte>();
                    unsigned short seqNumber = msg.Read<unsigned short>();

                    for (int i = 0; i < msgsWaitingForAck.size(); i++)
                    {
                        if (msgsWaitingForAck[i].first.msgType == msgType && msgsWaitingForAck[i].first.sequenceNumber == seqNumber)
                        {
                            msgsWaitingForAck.erase(msgsWaitingForAck.begin() + i);
                            break;
                        }
                    }
                    pos += 3;
                }
            }
            break;

            default:
            {
#ifdef _DEBUG
                printf("Received unknown library message: 0x%x (%d)\n", (byte)msg.msgType & 0xFF, msg.msgType);
                for (int i = 0; i < msg.size; i++)
                {
                    printf("0x%x ", msg.Read<byte>() & 0xFF);
                }
                msg.RewindMessage();
                printf("\nEND\n");
#endif
            }
            break;
            }
        }
    }

}

