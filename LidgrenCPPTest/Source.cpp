
#include <string>
#include <vector>
#include <iostream>

#include "LidgrenNetwork.h"
#include "Multiplayer.h"
#include <winsock.h>

void pause(bool outputMessage)
{
	if(outputMessage)
		std::cout << "Press ENTER to continue...\n";
	std::cin.clear();
	std::cin.sync();
	std::cin.get();
}

int main(void)
{

	LidgrenNetwork::NetClient client("BeatSaberMultiplayer");

	if (client.OpenConnection("127.0.0.1", 3700) < LidgrenNetwork::NetErrorCode::OK)
	{
		printf("Connection failed!\n");
		return 0;
	}

	printf("Connected!\n");

	LidgrenNetwork::NetOutgoingMessage hailMsg;

	byte versionBytes[] = { 0, 7, 2, 0 };

	for (int i = 0; i < 4; i++)
	{
		hailMsg.Write(versionBytes[i]);
	}

	Multiplayer::PlayerInfo player;
	//memset(&player, 0, sizeof(player));

	player.playerName = "andruzzzhka";
	player.playerId = 12345;

	player.AddToMessage(hailMsg);

	if (client.SendConnectMessage(hailMsg) < LidgrenNetwork::NetErrorCode::OK)
	{
		printf("Failed to send Connect message!");
		return 0;
	}
	printf("Sent Connect message!\n");


	while (client.status != LidgrenNetwork::NetConnectionStatus::Connected)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	printf("Status changed to Connected!\nSending CreateRoom message...\n");


	LidgrenNetwork::NetOutgoingMessage createRoomMsg;
	createRoomMsg.msgType = LidgrenNetwork::NetMessageType::UserReliableOrdered1;

	Multiplayer::RoomSettings settings;

	settings.Name = "TEST ROOM";
	settings.UsePassword = true;
	settings.Password = "123";
	settings.SelectionType = Multiplayer::SongSelectionType::Manual;
	settings.MaxPlayers = 2;
	settings.ResultsShowTime = 5;
	settings.PerPlayerDifficulty = false;

	createRoomMsg.Write((byte)Multiplayer::CommandType::CreateRoom);
	settings.AddToMessage(createRoomMsg);

	client.SendUserMessage(createRoomMsg);

	LidgrenNetwork::NetIncomingMessage createRoomResponseMsg;
	Multiplayer::CommandType cmdTypeResponse;
	unsigned int roomId;

	/* Non-blocking version
	LidgrenNetwork::NetErrorCode result;

	while ((result = client.TryGetNetMessage(createRoomResponseMsg)) == LidgrenNetwork::NetErrorCode::QueueEmpty)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	if (result == LidgrenNetwork::NetErrorCode::OK)
	{
		cmdTypeResponse = (CommandType)createRoomResponseMsg.Read<byte>();
		if(cmdTypeResponse == CommandType::CreateRoom)
			roomId = createRoomResponseMsg.Read<unsigned int>();
		else
		{
			printf("We should've received CreateRoom command, but received command with ID %d\n", cmdTypeResponse);
			pause(true);
			return 0;
		}
	}
	else
	{
		printf("Something went wrong while waiting for NetMessage\n");
		pause(true);
		return 0;
	}
	*/

	//Blocking version
	if(client.GetNextUserMessage(createRoomResponseMsg) == LidgrenNetwork::NetErrorCode::OK)
	{
		cmdTypeResponse = (Multiplayer::CommandType)createRoomResponseMsg.Read<byte>();
		if(cmdTypeResponse == Multiplayer::CommandType::CreateRoom)
			roomId = createRoomResponseMsg.Read<unsigned int>();
		else
		{
			printf("We should've received CreateRoom command, but received command with ID %d\n", cmdTypeResponse);
			pause(true);
			return 0;
		}
	}
	else
	{
		printf("Something went wrong while waiting for NetMessage\n");
		pause(true);
		return 0;
	}

	printf("Created room with ID %d!\n", roomId);

	LidgrenNetwork::NetOutgoingMessage joinRoomMsg;
	joinRoomMsg.msgType = LidgrenNetwork::NetMessageType::UserReliableOrdered1;

	joinRoomMsg.Write((byte)Multiplayer::CommandType::JoinRoom);
	joinRoomMsg.Write(roomId);
	joinRoomMsg.WriteString("123");

	client.SendUserMessage(joinRoomMsg);

	LidgrenNetwork::NetIncomingMessage joinRoomResponseMsg;

	if (client.GetNextUserMessage(joinRoomResponseMsg) == LidgrenNetwork::NetErrorCode::OK)
	{
		cmdTypeResponse = (Multiplayer::CommandType)joinRoomResponseMsg.Read<byte>();
		Multiplayer::JoinResult joinResult = (Multiplayer::JoinResult)joinRoomResponseMsg.Read<byte>();
		if (cmdTypeResponse != Multiplayer::CommandType::JoinRoom || joinResult != Multiplayer::JoinResult::Success)
		{
			printf("Unable to join room!");
			pause(true);
			return 0;
		}
	}
	else
	{
		printf("Something went wrong while waiting for NetMessage\n");
		pause(true);
		return 0;
	}

	printf("Joined room with ID %d!\nSending player info in a loop...", roomId);

	player.updateInfo.playerState = Multiplayer::PlayerState::Room;

	LidgrenNetwork::NetIncomingMessage updatePlayerInfoMsg;

	while (client.GetNextUserMessage(updatePlayerInfoMsg) == LidgrenNetwork::NetErrorCode::OK)
	{
		cmdTypeResponse = (Multiplayer::CommandType)updatePlayerInfoMsg.Read<byte>();


		if (cmdTypeResponse == Multiplayer::CommandType::UpdatePlayerInfo)
		{
			LidgrenNetwork::NetOutgoingMessage updateResponseMsg;
			updateResponseMsg.msgType = LidgrenNetwork::NetMessageType::UserReliableUnordered;

			updateResponseMsg.Write(Multiplayer::CommandType::UpdatePlayerInfo);
			updateResponseMsg.Write((byte)0);
			player.updateInfo.AddToMessage(updateResponseMsg);
			updateResponseMsg.Write((byte)0);

			player.updateInfo.playerScore++;

			client.SendUserMessage(updateResponseMsg);
		}
		else if (cmdTypeResponse != Multiplayer::CommandType::UpdateVoIPData)
		{
			printf("We should've received UpdatePlayerInfo command, but received command with ID %d!\n", cmdTypeResponse);
			//pause(true);
			//return 0;
		}
	}

	pause(true);

	return (0);
}