#pragma once
#include <iostream>
#include "proto.h"
#include "enet/enet.h"
#include "flatbuffers/flatbuffers.h"

using namespace flatbuffers;

using namespace Protocol;

class NetworkManager
{
public:

	/*
	* NETWORK MAIN PURPOSE
	*	- PACKAGING THE FLATBUFFER FUNCTIONALITY ALL THE DIFFERENT KIND
	*	- ESTABLISH THE ENET INITALIZE
	*/

	static NetworkManager& Instance()
	{
		static NetworkManager instance;
		return instance;
	}

	NetworkManager(); //initalize ENET
	~NetworkManager(); //Destroy ENET

	void SendToServer(ENetPeer* peer, const FlatBufferBuilder& builder);
	void SendToServer(ENetPeer* peer, uint32_t id); //For now disconnect request event C2S
	void SendToClient(ENetPeer*, const FlatBufferBuilder& builder); //In server find the client with the ID we want to send to
	void Broadcast(ENetHost* serverHost, const FlatBufferBuilder& builder); //Only server broadcast to all connected players
};

extern NetworkManager net_instance;

namespace packet {
	//Server To Client packet
	FlatBufferBuilder ClienConnectsS2C(const uint32_t senderID, unsigned long long timeMs); //server time ms
	FlatBufferBuilder GameStateS2C(const std::vector<Player>& players, const std::vector<Laser>& lasers); //const vector of laser should be implemented here also
	FlatBufferBuilder SpawnPlayerS2C(const Player* player);
	FlatBufferBuilder DespawnPlayerS2C(const uint32_t playerID);
	FlatBufferBuilder UpdatePlayerS2C(const uint64_t timeMs, const Player* player); //server time when it was sent back to client
	FlatBufferBuilder TeleportPlayerS2C(const uint64_t timeMs, const Player* player); //server time when it was sent back
	FlatBufferBuilder SpawnLaserS2C(const Laser* laser);
	FlatBufferBuilder DespawnLaserS2C(const uint32_t laserID);
	FlatBufferBuilder CollisionS2C(uint32_t entity1ID, uint32_t entity2ID);
	FlatBufferBuilder TextS2C(const std::string& text);

	// Client to server.
	FlatBufferBuilder InputC2S(uint64 timeMs, uint16 bitmap);
	FlatBufferBuilder TextC2S(const std::string& text);
}
