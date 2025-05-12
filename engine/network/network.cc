#include "config.h"
#include "network.h"

using namespace Protocol;

NetworkManager net_instance = NetworkManager::Instance();

NetworkManager::NetworkManager()
{
	if(enet_initialize() != 0){
		std::cout << "Failed to initalize ENET!\n";
	}
	std::cout << "Successful initalize ENET NETWORK\n";

}

NetworkManager::~NetworkManager()
{
	enet_deinitialize();
}

void NetworkManager::SendToServer(ENetPeer* peer,const FlatBufferBuilder& builder)
{
	if (peer == nullptr) return; //No connection to server
	ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer,0,packet);
}

void NetworkManager::SendToServer(ENetPeer* peer, uint32_t id)
{
	//Currently the program uses peer->incomingPeerId so id are for custom id management 
	enet_peer_disconnect(peer,id);
}

void NetworkManager::SendToClient(ENetPeer* peer, const FlatBufferBuilder& builder)
{
	if (peer == nullptr) return;
	ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
}

void NetworkManager::Broadcast(ENetHost* serverHost, const FlatBufferBuilder& builder)
{
	if (serverHost == nullptr) return;
	ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(serverHost, 0, packet);
}


namespace packet
{
	//Server to client
	FlatBufferBuilder ClienConnectsS2C(const uint32_t senderID, unsigned long long serverTime)
	{
		FlatBufferBuilder fbb;
		const auto clientConnect = CreateClientConnectS2C(fbb, senderID, serverTime);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_ClientConnectS2C, clientConnect.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder GameStateS2C(const std::vector<Player>& players, const std::vector<Laser>& lasers)
	{
		FlatBufferBuilder fbb;
		const auto gameState = CreateGameStateS2CDirect(fbb, &players, &lasers);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_GameStateS2C, gameState.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder SpawnPlayerS2C(const Player* player)
	{
		FlatBufferBuilder fbb;
		const auto spawnP = CreateSpawnPlayerS2C(fbb, player);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_SpawnPlayerS2C, spawnP.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder DespawnPlayerS2C(const uint32_t playerID)
	{
		FlatBufferBuilder fbb;
		const auto despawnP = CreateDespawnPlayerS2C(fbb, playerID);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_DespawnPlayerS2C, despawnP.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder UpdatePlayerS2C(const uint64_t timeMs, const Player* player)
	{
		FlatBufferBuilder fbb;
		const auto updateP = CreateUpdatePlayerS2C(fbb, timeMs, player);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_UpdatePlayerS2C, updateP.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder TeleportPlayerS2C(const uint64_t timeMs, const Player* player)
	{
		FlatBufferBuilder fbb;
		const auto teleP = CreateTeleportPlayerS2C(fbb, timeMs, player);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_TeleportPlayerS2C, teleP.Union());
		fbb.Finish(wrapper);
		return fbb; 
	}

	FlatBufferBuilder SpawnLaserS2C(const Laser* laser)
	{
		FlatBufferBuilder fbb;
		const auto spawnL = CreateSpawnLaserS2C(fbb, laser);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_SpawnLaserS2C, spawnL.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder DespawnLaserS2C(const uint32_t laserID)
	{
		FlatBufferBuilder fbb;
		const auto despawnL = CreateDespawnLaserS2C(fbb, laserID );
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_DespawnLaserS2C, despawnL.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder CollisionS2C(const uint32_t entity1ID, const uint32_t entity2ID)
	{
		FlatBufferBuilder fbb;
		const auto collision = CreateCollisionS2C(fbb,entity1ID, entity2ID);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_CollisionS2C, collision.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder TextS2C(const std::string& text)
	{
		FlatBufferBuilder fbb;
		const auto textS2C = CreateTextS2CDirect(fbb, text.c_str());
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_TextS2C, textS2C.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	//Client to Server
	FlatBufferBuilder InputC2S(uint64 timeMs, uint16 bitmap)
	{
		FlatBufferBuilder fbb;
		const auto input = CreateInputC2S(fbb,timeMs,bitmap);
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_InputC2S, input.Union());
		fbb.Finish(wrapper);
		return fbb;
	}

	FlatBufferBuilder TextC2S(const std::string& text)
	{
		FlatBufferBuilder fbb;
		const auto textC2S = CreateTextC2SDirect(fbb, text.c_str());
		const auto wrapper = CreatePacketWrapper(fbb, PacketType_TextC2S, textC2S.Union());
		fbb.Finish(wrapper);
		return fbb;
	}
}