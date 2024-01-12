#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <mutex>
#include <memory>

#include "Player.h"
#include "Turn.h"
#include "Image.h"
#include "Chat.h"
#include "common.h"

class Game
{
public:
	Game() noexcept;
	Game(const Game& other) noexcept = default;
	Game& operator=(const Game& other) noexcept = default;
	Game(Game&& other) noexcept;
	Game& operator=(Game&& other) noexcept;
	~Game() noexcept = default;

public:
	const std::vector<Player>& GetPlayers() const;
	const Player& GetPlayer(const std::string& name) const;
	uint8_t GetRoundNumber() const;
	common::game::GameSettings& GetGameSettings();
	Turn& GetTurn();
	Image& GetImage();
	Chat& GetChat();
	common::game::GameState GetGameState() const;
	void SetGameState(common::game::GameState gameState);
	std::mutex& GetPlayersMutex();

public:
	void Run();
	void AddPlayer(const Player& player);
	void RemovePlayer(const std::string& playerName);
	void RemoveDisconnectedPlayers();
	void Reset();

private:
	uint8_t m_roundNumber;
	uint8_t m_ownerID;
	std::vector<Player> m_players;
	Turn m_turn;
	common::game::GameSettings m_gameSettings;
	Image m_image;
	Chat m_chat;
	common::game::GameState m_gameState;
	std::shared_ptr<std::mutex> m_sharedMutex;
};