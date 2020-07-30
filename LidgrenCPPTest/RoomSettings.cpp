#include <string>
#include "RoomSettings.h"


namespace Multiplayer
{
    typedef unsigned char byte;

    RoomSettings::RoomSettings() : MaxPlayers(0), PerPlayerDifficulty(false), ResultsShowTime(0), SelectionType(SongSelectionType::Manual), UsePassword(false)
    {

    }

    RoomSettings::RoomSettings(LidgrenNetwork::NetIncomingMessage& msg)
    {

        Name = msg.ReadString();

        UsePassword = msg.ReadBoolean();
        PerPlayerDifficulty = msg.ReadBoolean();

        msg.SkipPadBits();

        if (UsePassword)
            Password = msg.ReadString();

        MaxPlayers = msg.Read<int>();
        ResultsShowTime = msg.Read<float>();
        SelectionType = (SongSelectionType)msg.Read<byte>();
    }

    void RoomSettings::AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg)
    {
        msg.WriteString(Name);

        msg.WriteBoolean(UsePassword);
        msg.WriteBoolean(PerPlayerDifficulty);

        msg.WritePadBits();

        if (UsePassword)
            msg.WriteString(Password);

        msg.Write(MaxPlayers);
        msg.Write(ResultsShowTime);
        msg.Write((byte)SelectionType);
    }
}