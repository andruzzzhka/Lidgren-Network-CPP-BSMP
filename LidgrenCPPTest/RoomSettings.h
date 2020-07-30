#pragma once

#include "LidgrenNetwork.h"

namespace Multiplayer
{
    enum class SongSelectionType : unsigned char
    {
        Manual,
        Random
    };

    class RoomSettings
    {
    public:
        std::string Name;

        bool UsePassword;
        std::string Password;

        SongSelectionType SelectionType;
        int MaxPlayers;
        float ResultsShowTime;
        bool PerPlayerDifficulty;

        RoomSettings();
        RoomSettings(LidgrenNetwork::NetIncomingMessage& msg);

        void AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg);

    };
}