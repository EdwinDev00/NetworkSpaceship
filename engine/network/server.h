#pragma once
//#include <vector>
//#include "proto.h"

#include "network.h"
#include <unordered_map>
#include "physics/physics.h"
#include <unordered_set>

//#include "../projects/spacegame/code/spaceship.h"


namespace Game
{
   struct ServerSpaceship;
   struct ServerLaser;
}

struct ServerAsteroid
{
    Physics::ColliderId colliderID;
    glm::mat4 transform;
    
 /*   ServerAsteroid(const Physics::ColliderId& id, const glm::mat4& transform) :
        colliderID(id), transform(transform) {
    };*/
};

struct PendingRespawn
{
    uint32_t playerID;
    float respawnTimer; //second left until respawn
};

struct SpawnPoint
{
    glm::vec3 position = glm::vec3(0);
    bool occupied = false;
    uint32_t ownerID = 0; //who is using this spawnpoint

    SpawnPoint* GetAvailableSpawnpoint()
    {
        if (!occupied)
            return this; //if not occupied return this spawnpoint
        return nullptr; //otherwise not available spawnpoint 
    }

    glm::quat calcOrientationToOrigin()
    {
        glm::vec3 directionToOrigin = normalize(-position);
        return glm::quat(glm::vec3(0, 0, 1),directionToOrigin);
    }
};

//Converter serverspaceship into a player

using namespace Protocol;

class GameServer
{
public:

    static GameServer& instance()
    {
        static GameServer instance;
        return instance;
    }

    void StartServer(uint16_t port = 1234);
    void ShutdownServer();
    void Run();
    void Update(float dt = 0.016f);
    void SetAsteroid(const ServerAsteroid& asteroid) { 
        asteroids.push_back(asteroid); }

    GameServer() = default;
    ~GameServer();

    bool live = false;
    //FOR CHECKING IF THE SERVER POSITION OF SPACESHIP UPDATES

private:
   
    //ENET / NETWORKING
    void InitNetwork(uint16_t port);
    void PollNetworkEvents();
    void OnClientConnect(ENetPeer* peer);
    void OnClientDisconnect(uint32_t clientID);
    void OnPacketRecieved(uint32_t senderID, const ENetPacket* packet);

    //GAMEPLAY
    void SpawnPlayer(uint32_t clientID);
    void RemovePlayer(uint32_t clientID);
    bool CheckCollision(Game::ServerSpaceship& shipA, Game::ServerSpaceship& shipB);

    //UTILITIY
    Player BatchShip(const Game::ServerSpaceship& ship) const;
    Laser BatchLaser(const Game::ServerLaser& laser) const;

    //SERVER STATE
    ENetHost* server;
    uint32_t serverPort;

    uint32_t nextClientID = 1; //Auto increment ID for new player
    uint64_t s_currentTime = 0; //current server time (ms)


    int serverTickCounter = 0;
    const int sendRate = 5; // every 5 physics tick

    //Server time related  (general time related)
    

    //CONNECTED USERS (CLIENTS)
    std::unordered_map<ENetPeer*, uint32_t> connections;

    //GAME STATE
    Physics::ColliderMeshId playerMeshColliderID;
    std::unordered_map<uint32_t, Game::ServerSpaceship> players; //AMount of player ship is registered in the server (for handling updates and changes)
    std::unordered_map<uint32_t, Physics::ColliderId> playerColliders; //Colliders for the players spaceship
    std::unordered_map<uint32_t, Game::ServerLaser> lasers; // All the registered laser in the server
    uint32_t laserUUIDCounter = 0; //Count for laser

    std::unordered_set<uint32_t> playerToDespawn; //Set to mark player for despawning and send package to related client
    std::unordered_set<uint32_t> collidedPlayer; //player which already mark for collided so to skip further collision check
    std::vector<PendingRespawn> pendingRespawns; //Stores the respawn package for the dead client until respawn
    std::unordered_set<uint32_t> laserToDespawn; //Set to mark laser for despawning and send package;


    //std::queue<uint32_t> laserToRemove; //Mark for the laser to be removed

    //Game related
   // std::vector<Physics::ColliderId> asteroidsColliders; //collider ID of the asterroids in the game
    std::vector<ServerAsteroid> asteroids;


    std::array<SpawnPoint, 32> spawnpoints; //SpawnPoints of player spawn (ENet set to max 32)

    //ADD PREVENT COPY/MOVE OPERATOR FOR OUR INSTANCE(ENSURE ONLY SINGLE INSTANCE EXIST)
};

extern GameServer gameServer;