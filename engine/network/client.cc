#include "config.h"
#include "client.h"

#include <chrono>

GameClient gameClient = GameClient::Instance();

void GameClient::Create()
{
	//Create the client host
	client = enet_host_create(nullptr, 1, 1, 0, 0);
	if (!client)
	{
		std::cout << "Failed to create ENet Client!\n";
		return;
	}

	isActive = true;
}

bool GameClient::ConnectToServer(const char* ip, const uint16_t port )
{
	//Connect to the server 
	if (!isActive) return false; //Attempt to connect before client creation

	//Assign the address -> connect to server address
	ENetAddress address;
	enet_address_set_host(&address, ip);
	address.port = port;
	peer = enet_host_connect(client, &address, 1, 0);
	if(!peer)
	{
		std::cout << "CLIENT: Failed to establish connection request to peer at: " << address.host
			<< "; " << port << "\n";
		return false;
	}
	
	std::cout << "CLIENT: Successful initiate connection to ENET Server: Awaiting for server respond to request\n";
	return true; //Successful connecting to server
}

void GameClient::Update()
{
	if (!isActive) return;
	currentTime = Time::Now();
	
	ENetEvent event;
	while (enet_host_service(client, &event, 0) > 0) //Pool
	{
		switch (event.type)
		{
		//case ENET_EVENT_TYPE_CONNECT: {
		//	std::cout << "CLIENT TYPE CONNECT WAS GENERATED\n";
		//	//Call the respective callback
		//	//if (OnClientConnect)
		////	OnClientConnect(event.peer);
		//	break;
		//}

		case ENET_EVENT_TYPE_RECEIVE: {
			OnRecievepacket(event.packet);
			break;
		}

		}
	}
}

void GameClient::SendInput(const FlatBufferBuilder& builder)
{
	net_instance.SendToServer(this->peer, builder);
}

void GameClient::DisconnectFromServer()
{
	net_instance.SendToServer(this->peer, this->myPlayerID);
}


void GameClient::OnRecievepacket(ENetPacket* packet)
{
	//On packet recieved from the server
	auto wrapper = GetPacketWrapper(packet->data)->UnPack()->packet;
	switch (wrapper.type)
	{
		case PacketType_ClientConnectS2C:{
			std::cout << "CLIENT: Recieved Connect package\n";
			const auto clientConnectS2C = wrapper.AsClientConnectS2C();
			this->myPlayerID = clientConnectS2C->uuid;
			this->serverTime = clientConnectS2C->time;

			//Testing with synchronize time between client and server
			clientTimeZero = currentTime;

			std::cout << "CLIENT: Connect package with uuid " << clientConnectS2C->uuid << "\n";
			std::cout << "CLIENT: Player ID " << myPlayerID << "\n";
			break;
		}
		case PacketType_GameStateS2C: {

			std::cout << "CLIENT: Recieved GAME STATE update\n";
			auto gameState = wrapper.AsGameStateS2C();
			//std::cout << "Client spaceship list count before adding: " << spaceships->size() << "\n";
			for(const auto& player : gameState->players)
			{
				spaceships.emplace(player.uuid(), Game::ClientSpaceship());
				Game::ClientSpaceship& ship = spaceships.at(player.uuid());
				ship.id = player.uuid();
				const Vec3& pos = player.position();
				const Vec3& vel = player.velocity();
				const Vec4& orient = player.direction();
				ship.position = glm::vec3(pos.x(), pos.y(), pos.z());
				ship.linearVelocity = glm::vec3(vel.x(), vel.y(), vel.z());
				ship.orientation = glm::quat(orient.x(), orient.y(), orient.z(), orient.w());
				ship.InitSpaceship();
				if(spaceships.count(player.uuid()))
				{
					std::cout << "Client spaceship with this ID: " << player.uuid() << " is present in the spaceshipMap\n";
				}
				else
					std::cout << "Client spaceship with this ID: " << player.uuid() << " is absent in the spaceshipMap\n";
			}
			break;
		}

		case PacketType_SpawnPlayerS2C:
		{
			//Spawn the player we initalized 
			std::cout << "CLIENT: RECIEVED SPAWNPLAYER PACKAGE\n";
			auto& player = wrapper.AsSpawnPlayerS2C()->player;

			// Clean safety check
			if (spaceships.contains(player->uuid()))
			{
				spaceships[player->uuid()].RemoveSpaceship();
				spaceships.erase(player->uuid());
				std::cout << "CLIENT: CLEANED OLD SPACESHIP BEFORE RESPAWN\n";
			}

			spaceships.emplace(player->uuid(), Game::ClientSpaceship(player->uuid()));
			Game::ClientSpaceship& spaceship = spaceships.at(player->uuid());
			const Vec3& pos = player->position();
			const Vec4& orient = player->direction();
			spaceship.position = glm::vec3(pos.x(), pos.y(), pos.z());
			spaceship.orientation = glm::quat(orient.x(), orient.y(), orient.z(), orient.w());
			spaceship.InitSpaceship();
			std::cout << "CLIENT: spaceships count " << spaceships.size() << "\n";
			break;
		}

		case PacketType_UpdatePlayerS2C:
		{
			//std::cout << "CLIENT: RECIEVED UpdatePlayerS2C PACKAGE\n";
			const auto updatePlayer = wrapper.AsUpdatePlayerS2C();
			auto& player = updatePlayer->player;
			if (!spaceships.contains(player->uuid())) return;
			Game::ClientSpaceship& ship = spaceships.at(player->uuid()); 
			glm::vec3 serverPs(player->position().x(), player->position().y(), player->position().z());
			glm::quat serverOr(player->direction().x(), player->direction().y(), player->direction().z(), player->direction().w());
			glm::vec3 serverVe(player->velocity().x(), player->velocity().y(), player->velocity().z());
			ship.CorrectFromServer(serverPs, serverOr, serverVe,updatePlayer->time); //UpdatePLayer change with currentTime for online test
			break;
		}

		case PacketType_DespawnPlayerS2C:
		{
			//DESPAWN THE SPACESHIP BASED ON THE ID FROM THE PACKAGE (HANDLES THE BOTH WHEN ITS DESPAWN AND DISSCONNECT)
			std::cout << "CLIENT: RECIEVED DESPAWNPLAYER PACKAGE\n";
			
			const auto despawn = wrapper.AsDespawnPlayerS2C();
			auto id = despawn->uuid;
			spaceships[id].RemoveSpaceship();
			spaceships.erase(id);
			this->myPlayerID - 1; //REMOVE THE PLAYER ID
			break;
		}

		case PacketType_SpawnLaserS2C:
		{
			//std::cout << "CLIENT: RECIEVED SPAWN LASER PACKAGE\n";
			auto& laserPacket = wrapper.AsSpawnLaserS2C()->laser;
			//Game::ClientLaser laser;
			glm::vec3 laserPos = glm::vec3(laserPacket->origin().x(), laserPacket->origin().y(), laserPacket->origin().z());
			glm::quat laserOr = glm::quat(laserPacket->direction().x(), laserPacket->direction().y(), laserPacket->direction().z(), laserPacket->direction().w());
			lasers[laserPacket->uuid()] = Game::ClientLaser();

			auto& laser = lasers.at(laserPacket->uuid());
			laser.uuid = laserPacket->uuid();
			laser.startTime = laserPacket->start_time();
			laser.endTime = laserPacket->end_time();
			laser.position = laserPos;
			laser.orientation = laserOr;

			//Sync the laser with the server
			const uint64_t packetSentTime = laserPacket->start_time() - serverTime;
			const uint64_t packetRecievedTime = currentTime - clientTimeZero;
			laser.serverSentTime = packetSentTime;
			laser.clientRecievedTime = packetRecievedTime;
			laser.elapsedTime = (float)(packetRecievedTime - packetSentTime) / 1000.0f;

			laser.transform = glm::translate(laserPos) * glm::mat4_cast(laserOr) * glm::scale(glm::vec3(1.0f));// * modelCorrection;
			break;
		}

		case PacketType_DespawnLaserS2C:
		{
			//std::cout << "CLIENT: RECIEVED DESPAWN LASER PACKAGE\n";
			const auto despawnLaser = wrapper.AsDespawnLaserS2C();
			lasers.erase(despawnLaser->uuid);
			break;
		}
		default:
			break;
	}
}

