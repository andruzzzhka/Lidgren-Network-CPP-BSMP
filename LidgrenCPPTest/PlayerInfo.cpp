#include <string>
#include "PlayerInfo.h"

namespace Multiplayer
{
    void PlayerUpdate::AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg)
    {
        msg.Write(playerNameColor.r);
        msg.Write(playerNameColor.g);
        msg.Write(playerNameColor.b);

        msg.Write((byte)playerState);

        msg.Write((byte)0); //Full-body tracking (not available on Quest?)
        msg.WriteVariableUInt32(playerScore);
        msg.WriteVariableUInt32(playerCutBlocks);
        msg.WriteVariableUInt32(playerComboBlocks);
        msg.WriteVariableUInt32(playerTotalBlocks);
        msg.Write(playerEnergy);
        msg.Write(playerProgress);

        /* TODO
        if (playerLevelOptions == default)
            playerLevelOptions = new LevelOptionsInfo(BeatmapDifficulty.Hard, GameplayModifiers.defaultModifiers, "Standard");

        playerLevelOptions.AddToMessage(msg);O

        playerFlags.AddToMessage(msg);
        */

        msg.Write((byte)0);
        msg.Write((byte)0);
        msg.Write((byte)0);

        rightHandPos.AddToMessage(msg);
        leftHandPos.AddToMessage(msg);
        headPos.AddToMessage(msg);

        rightHandRot.AddToMessage(msg);
        leftHandRot.AddToMessage(msg);
        headRot.AddToMessage(msg);
    }


    void PlayerInfo::AddToMessage(LidgrenNetwork::NetOutgoingMessage& msg)
    {
        msg.WriteString(playerName);
        msg.Write(playerId);

        updateInfo.AddToMessage(msg);

        for (int i = 0; i < 16; i++)
            msg.Write((byte)0xFF);

        msg.Write((byte)0);
    }
}