#pragma once

#include "LidgrenNetwork.h"
#include "MiscStructs.h"

namespace Multiplayer
{

    enum class PlayerState : byte
    {
        Disconnected,
        Lobby,
        Room,
        Game,
        Spectating,
        DownloadingSongs
    };

    class PlayerUpdate
    {
    public:
        Color32 playerNameColor;
        PlayerState playerState = PlayerState::Disconnected;

        unsigned int playerScore = 0;
        unsigned int playerCutBlocks = 0;
        unsigned int playerComboBlocks = 0;
        unsigned int playerTotalBlocks = 0;
        float playerEnergy = 0;

        float playerProgress = 0;

        //LevelOptionsInfo playerLevelOptions; //TODO

        Vector3 headPos;
        Vector3 rightHandPos;
        Vector3 leftHandPos;

        Quaternion headRot;
        Quaternion rightHandRot;
        Quaternion leftHandRot;

        void AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg);
    };

    class PlayerInfo
    {
    public:
        std::string playerName;
        unsigned long long playerId = 0;

        //std::string avatarHash; //TODO?

        PlayerUpdate updateInfo;

        void AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg);
    };
}