#pragma once


namespace Multiplayer
{
    typedef unsigned char byte;

    enum class CommandType : byte { Connect, Disconnect, GetRooms, CreateRoom, JoinRoom, GetRoomInfo, LeaveRoom, DestroyRoom, TransferHost, SetSelectedSong, StartLevel, UpdatePlayerInfo, PlayerReady, SetGameState, DisplayMessage, SendEventMessage, GetChannelInfo, JoinChannel, LeaveChannel, GetSongDuration, UpdateVoIPData, GetRandomSongInfo, SetLevelOptions, GetPlayerUpdates, RequestSong, RemoveRequestedSong, GetRequestedSongs };


    enum class JoinResult : byte { Success, RoomNotFound, IncorrectPassword, TooMuchPlayers };
}