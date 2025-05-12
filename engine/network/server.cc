#include "config.h"
#include "server.h"
#include "../projects/spacegame/code/spaceship.h"

#include <iostream>
#include <chrono>

#include "timer.h"

#include <gtx/string_cast.hpp> //DEBUG LOG VEC3

#pragma region UTILITY

Player GameServer::BatchShip(const Game::ServerSpaceship& ship) const
{
	const glm::vec3& pos = ship.position;
	const glm::vec3& vel = ship.linearVelocity;
	const glm::quat& orient = ship.orientation;

	Vec3 posVec = Vec3(pos.x, pos.y, pos.z);
	Vec3 velVec = Vec3(vel.x, vel.y, vel.z);
	Vec3 accelVec = Vec3(); // acceleration of the ship
	Vec4 OrientVec = Vec4(orient.w,orient.x, orient.y, orient.z);

	return Player(ship.id,posVec,velVec,accelVec,OrientVec);
}

Laser GameServer::BatchLaser(const Game::ServerLaser& laser) const
{
	//LASER PROPERTIES
	return 
	{
		laser.uuid,
		laser.startTime,
		laser.endTime,
		Vec3(laser.position.x, laser.position.y, laser.position.z),
		Vec4(laser.orientation.w, laser.orientation.x, laser.orientation.y, laser.orientation.z)
	};
}
#pragma endregion

/*
GAMESERVER RESPONSIBILITY:
	- Manages game state (player, entities)
	- Processes client input and sends authoritative updates
	- Network manager (network) for communication
*/

// CURRENT WORKING: TESTING THE ONLINE PLAY AND LATENCY
// CHECK FOR THE UPDATE TIME FOR SYNC BETWEEN SERVER UPDATE AND CLIENT

//DEAD RECKONING IMPLEMENTATION: WE ARE CURRENTLY USING THE SNAPSHOT INTERPOLATOR WITH EXTRAINTERPOLAT WHICH SHOULD SIMULATE A FORM INTERPOLATE POSITIONS
//TODO: ADD THE CONTROLL FOR NETWORK DELAY INTO THE EXISTING FUNCTIONALITY (ACCOUNT OF DELAYS/LATENCY WHICH COULD HAPPEND DURING ONLINE SECTION)
//TODO: SERVER AND CLIENT SYSTEM TIMER SYNCHRONIZATION CHECK (MAKE SURE THE SYNCHRONIZE BY SERVER fixed 60 fps matches with client current frame rates)

//BUG WHICH NEED FIXING
//FOUND BUG DURING TESTING AFTER DESPAWN (THEIR ARE REMAINS OF A INVISIBLE PLAYER OR ASTEROID SENT TO SERVER POSITION IS NOT ACCURATE)

//COMPLETED: ABLE TO DETECT LASER COLLISION WITH PLAYER AND ASTEROID
//COMPLETED: SOLVED LASER WACKY DOODOO & ADDED SERVER FRAMECAP TO 60 FRAME PER SECOND //STD::THREAD

//LATER FIX FOR BETTER EXPERIENCE
//NEXT WORKING AREA: FIX THE STUTTER IN THE CAMERA LOOKAT WHEN RESPAWNING (CONTROLLED AVATAR DISEAPER WHICH MAKE THE CAMERA SPAZ OUT WHEN TRYING TO UPDATE)
// FOUND ISSUE FIRST SPAWN: SPAWN IN THE CENTER FOR ONE FRAME AND THEN TELEPORT TO THE SPAWNPOINT (NEED A LOOK UP) 

//ONLINE TESTING POTENTIAL ISSUE NEED TO FIX
//WHEN TESTING CONNECTION: SYNCHRONIZE THE CLIENT LASER WITH THE SERVER REPRESENTATION (TIME) (SYNCHRONIZATION)

//Singelton Gameserver instance
GameServer gameServer = GameServer::instance();

GameServer::~GameServer() 
{
	std::cout << "GAMESERVER DESTRUCTOR";

}

void GameServer::StartServer(uint16_t port)
{
	InitNetwork(port);

	live = true; //Set the server into active
	playerMeshColliderID = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");

	//generate the spawnpoints for the connected user (circular)
	glm::vec3 center(0);
	const float radius = 50;
	for(int i = 0; i < 32; ++i)
	{
		const float angle = glm::two_pi<float>() * (float(i) / 32);
		const float x = center.x + radius * std::cos(angle);
		const float z = center.z + radius * std::sin(angle);
		spawnpoints[i].position = glm::vec3(x, 0.0f, z);
	}

	std::cout << "SERVER: Successful creating ENET server\n";

}

void GameServer::ShutdownServer()
{
	//shutdown server
	if (server != NULL)
	{
		enet_host_destroy(server);
		server = nullptr;
	}

	//Clear all the connect users (peers)
	live = false;
}

void GameServer::Run()
{
	//Main server run loop

	if(live && server != NULL) //As long the server exist run the server
	{
		s_currentTime = Time::Now();
		PollNetworkEvents();

		// Check for collision 
		//PLAYER VS PLAYER
		for(auto itA = players.begin(); itA != players.end(); itA++)
		{
			auto& [idA, shipA] = *itA;
			auto itB = itA;
			itB++;

			for(; itB != players.end(); itB++)
			{
				auto& [idB, shipB] = *itB;

				//Check for collision between shipA and shipB
				if(CheckCollision(shipA,shipB))
				{
					//MARK THEM BOTH FOR DESPAWN
					playerToDespawn.insert(idA);
					playerToDespawn.insert(idB);
					collidedPlayer.insert(idA);
					collidedPlayer.insert(idB);
				}
			}
		}

		//Player vs asteroids
		for(auto& [id,ship] : players)
		{
			//Already mark collided (player vs player) skip asteroid check
			if (collidedPlayer.find(id) != collidedPlayer.end()) continue;

			if(ship.CheckCollision())
			{
				playerToDespawn.insert(id);
			}
		}

		//PLAYER vs LASER
		for (auto& laser : lasers)
		{
			auto object = laser.second.CheckCollision(playerColliders);
			if (object.has_value())
			{
				if(object.value() != UINT32_MAX)
				{
					playerToDespawn.insert(object.value()); // std::unordered_set only inserts if the element isn't already present
					std::cout << "SERVER: PLAYER WITH ID " << object.value() << " GOT HIT BY LASER! MARKING FOR DESPAWN\n";
				}
				laserToDespawn.insert(laser.first);
			}
		}

		//HANDLE DESPAWN PLAYER
		const float respawnDelay = 3.0f; //3 seconds until respawn
		for (auto id : playerToDespawn)
		{
			auto despawn = packet::DespawnPlayerS2C(id);
			net_instance.Broadcast(server, despawn);
			pendingRespawns.push_back({ id,respawnDelay });
			players.erase(id);
			playerColliders.erase(id);
		}

		//UPDATE LASER PHYSICS
		for(auto& laser : lasers)
		{
			if (laser.second.isExpired(s_currentTime))
			{
				auto despawn = packet::DespawnLaserS2C(laser.first);
				net_instance.Broadcast(server, despawn);
				laserToDespawn.insert(laser.first);
			}
			else
			{
				laser.second.Update(0.01667f); //fixed timestep
			}
		}

		//DESPAWN LASER
		for (auto id : laserToDespawn)
		{
			if (lasers.contains(id)) // Avoid sending for already removed lasers
			{
				auto despawnLaser = packet::DespawnLaserS2C(id);
				net_instance.Broadcast(server, despawnLaser);
				lasers.erase(id);
			}
		}

		//PLAYER PHYSICS UPDATE (fixed update at 60 fps)
		for (auto& [uuid, ship] : players)
		{
			if (playerToDespawn.contains(uuid))
				continue; // Skip updating ships that are about to despawn

			ship.Update(0.01667f); //fixed 60 frames
			Physics::SetTransform(playerColliders[uuid], ship.transform);
		}


		//RESET THE LIST 
		playerToDespawn.clear();
		collidedPlayer.clear();
		laserToDespawn.clear();

		//HANDLE THE RESPAWNS
		float deltaTime = 0.01667f; //Follows the same flow as server fixed timestep 60fps
		for(auto it = pendingRespawns.begin(); it != pendingRespawns.end();)
		{
			it->respawnTimer -= deltaTime;
			if (it->respawnTimer <= 0.0f)
			{
				std::cout << "RESPAWNING PLAYER WITH ID " << it->playerID << "\n";
				SpawnPlayer(it->playerID); //respawn the player by sending the spawnPackage back to that user
				it = pendingRespawns.erase(it);
			}
			else
				it++;
		}

		serverTickCounter++;

		//NETWORK STATE SYNC (Every Nth frame) 
		if(serverTickCounter % 5 == 0) //every 5 frame of 60 fps (12 times / s)
		{
			//UPDATE PLAYERS
			for(const auto& player : players)
			{
				auto packPlayer = BatchShip(player.second);
				const auto fbb = packet::UpdatePlayerS2C(s_currentTime, &packPlayer);
				net_instance.Broadcast(server, fbb);
			}
		}

		while(Time::Now() - s_currentTime < 16) { /*WAIT*/ }
	}
}

#pragma region ENET / NETWORK

void GameServer::InitNetwork(uint16_t port)
{
	//Creation of the ENet Server 
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;

	server = enet_host_create(&address, 32, 2, 0, 0);
	if (server == NULL)
	{
		std::cout << "SERVER: Failed to create ENET server\n";
	}
	//Successful creating the ENet server
}

void GameServer::PollNetworkEvents()
{
	ENetEvent event;
	while (enet_host_service(server, &event, 0) > 0) //Pool
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_CONNECT: {
				//std::cout << "SERVER:TYPE CONNECT WAS GENERATED\n";
				OnClientConnect(event.peer);
				break;
			}

			case ENET_EVENT_TYPE_RECEIVE: {
				//std::cout << "SERVER: RECIEVED INCOMING PACKET FROM CLIENT WITH ID: " << event.peer->incomingPeerID << "\n";
				OnPacketRecieved(event.peer->incomingPeerID, event.packet);
				break;
			}

			case ENET_EVENT_TYPE_DISCONNECT: {
				//std::cout << "SERVER: RECIEVED DISCONNECT PACKET FROM CLIENT WITH ID: " << event.peer->incomingPeerID << "\n";
				OnClientDisconnect(event.peer->incomingPeerID);
				break;
			}
		}
	}
}

void GameServer::OnClientConnect(ENetPeer* peer)
{
	/*
	*  incomingPeerID
		Purpose: Represents the ID assigned to the remote peer (i.e., the ID that the local host assigned to this connection).
		use: Identifying a specific connected client from the server's side.

		has drawback for soley using incomingPeerID, use self assinged ID When client successful render out the peer
	*/
	
	//uint32_t uuid = nextClientID++; //assign the user with this GameServer unique identifier
	connections[peer] = peer->incomingPeerID; //insert the new element into the list

	auto fbb = packet::ClienConnectsS2C(peer->incomingPeerID, s_currentTime);
	net_instance.SendToClient(peer, fbb); //Send the packet to the connected peer

	//Change in game state (apply the change of new player joined) 

	//GAME STATE CONFIG (APPLY THE NECESSITY FOR THE ADDING THE NEW PLAYER TO THE STATE)
	std::vector<Player> playerVec;
	playerVec.reserve(players.size());

	//Reserve the all existing players so the new player get the data of the already connected peers
	//Below should function the same as BatchShip
	std::transform(players.begin(), players.end(), std::back_inserter(playerVec),
		[](const std::pair<const uint32_t, Game::ServerSpaceship>& entry)
		{
			//Pack our custom server side spaceship into a protocol player
			auto& ship = entry.second;
			const glm::vec3& pos = ship.position;
			const glm::vec3& vel = ship.linearVelocity;
			const glm::quat& orient = ship.orientation;

			Vec3 posVec = Vec3(pos.x, pos.y, pos.z);
			Vec3 velVec = Vec3(vel.x, vel.y, vel.z);
			Vec3 accelVec = Vec3(); // acceleration of the ship
			Vec4 OrientVec = Vec4(orient.w,orient.x, orient.y, orient.z);

			return Player(ship.id, posVec, velVec, accelVec, OrientVec);
		}
	);
	
	std::cout << "PlayerVec count " << playerVec.size() << "\n";
	std::vector<Laser> laserVec;
	laserVec.reserve(lasers.size()); //We let it be zero (laser functionality and properties yet implemented)
	//Do the same for lasers (pack the existing in lasers in the game)

	fbb.Clear();
	fbb = packet::GameStateS2C(playerVec, laserVec);
	net_instance.SendToClient(peer, fbb); //Send back to connected user about the current game state 

	SpawnPlayer(peer->incomingPeerID);

	//std::cout << "SERVER: Client " << peer->incomingPeerID << " connected.\n";
	//std::cout << "SERVER SPACESHIP COUNT " << players.size() << "\n";
	//std::cout << "SERVER: Connected USER COUNT " << connections.size() << "\n";
}

void GameServer::SpawnPlayer(uint32_t clientID)
{
	//Check if the player already owns a spawn point
	SpawnPoint* assignedSpawn = nullptr;
	for(auto& sp : spawnpoints)
	{
		if(sp.occupied && sp.ownerID == clientID)
		{
			assignedSpawn = &sp;
			break;
		}
	}

	//if not assigned find the next available spawnPoint
	if(!assignedSpawn)
	{
		for (auto& sp : spawnpoints)
		{
			if (sp.GetAvailableSpawnpoint())
			{
				sp.occupied = true;
				sp.ownerID = clientID; // player ID
				assignedSpawn = &sp;
				break;
			}
		}
	}

	// Fallback: no spawn points available
	if (!assignedSpawn)
	{
		std::cerr << "SERVER: No available spawn point for client " << clientID << "!\n";
		return;
	}

	//Create and initalize the player spaceship
	auto& ship = players[clientID];
	ship.id = clientID;
	ship.position = assignedSpawn->position;
	ship.orientation = assignedSpawn->calcOrientationToOrigin();
	ship.transform = glm::translate(ship.position) * glm::mat4_cast(ship.orientation) * glm::scale(glm::vec3(1.0f));

	//Setup the spaceship collider
	if (!playerColliders.contains(clientID))
		playerColliders[clientID] = Physics::CreateCollider(playerMeshColliderID, ship.transform);
	else
		Physics::SetTransform(playerColliders[clientID], ship.transform);

	//SEND THE BROADCAST PACKAGE TO ALL CLIENT ABOUT A NEW USER CONNECTED
	auto playerData = BatchShip(ship);
	auto fbb = packet::SpawnPlayerS2C(&playerData);
	net_instance.Broadcast(server, fbb);
}

bool GameServer::CheckCollision(Game::ServerSpaceship& shipA, Game::ServerSpaceship& shipB)
{
	// Save original transform
	glm::mat4 originalTransformA = shipA.transform;

	//Set A's transform temporarily to the relative difference toward shipB
	glm::vec3 offset = glm::vec3(shipB.transform[3]) - glm::vec3(shipA.transform[3]);
	shipA.transform[3] = glm::vec4(glm::vec3(shipA.transform[3]) + offset, 1.0f);

	//perform the collision check
	bool collided = shipA.CheckCollision();
	//restore the original transform (Important step!)
	shipA.transform = originalTransformA;

	return collided;
}

void GameServer::OnPacketRecieved(uint32_t senderID, const ENetPacket* packet)
{
	//if (packet == NULL) return; //NO PACKET
	auto wrapper = GetPacketWrapper(packet->data);
	switch(wrapper->packet_type())
	{
		case PacketType_InputC2S:{
			//std::cout << "SERVER: RECIEVES A INPUT REQUEST FROM CLIENT\n";
			auto inputData = wrapper->packet_as_InputC2S();
			if (!inputData) return;
			auto& player = players[senderID]; //maybe need to navigate through the connections to find
			//apply the input (valid data)
			if (inputData->time() < player.lastInputTimeStamp) return;
			player.lastInputBitmap = inputData->bitmap();
			player.lastInputTimeStamp = inputData->time();
			player.inputCooldown = 0;
			//RECIEVES A INPUT EVENT
	/*		std::cout << "Player input bitmap " << player.lastInputBitmap << "\n";
			std::cout << "Player input timestamp " << player.lastInputTimeStamp << "\n";*/

			if (inputData->bitmap() & (1 << 7)) //SPACE input
			{
				//Spawn laser forward from this ship
				Game::ServerLaser laser;
				// Get forward direction
				glm::vec3 forward = player.orientation * glm::vec3(0.0f, 0.0f, 1.0f);

				laser.uuid = laserUUIDCounter++;
				laser.ownerID = player.id;
				laser.position = player.position + forward * 2.0f;
				laser.orientation = player.orientation;
				//laser.velocity = forward * glm::vec3(0.0f, 0.0f, 20.0f); //20 units / s speed

				laser.startTime = inputData->time();
				laser.endTime = inputData->time() + 2500; // 2.5s before disapear
				laser.transform = glm::translate(laser.position) * glm::mat4_cast(laser.orientation) * glm::scale(glm::vec3(1.0f));

				lasers[laser.uuid] = laser; //add it to the server laser list
				
				//Send the spawnlaser package to all client
				auto laserData = BatchLaser(laser);
				auto fbb = packet::SpawnLaserS2C(&laserData);
				net_instance.Broadcast(server, fbb);
			}

			break;
		}
		case PacketType_TextS2C:
			break;
		default:
			break;
	}
	
}

void GameServer::OnClientDisconnect(uint32_t clientID) {

	//std::cout << "SERVER: Client " << clientID << " disconnected.\n";
	playerColliders.erase(clientID);
	const auto fbb = packet::DespawnPlayerS2C(clientID);
	net_instance.Broadcast(server, fbb);
	players.erase(clientID);
}

#pragma endregion