#include "Game.h"

#include <mutex>

Game::Game() noexcept :
	m_players{},
	m_roundNumber{ 0 },
	m_ownerID{ 0 },
	m_gameSettings{},
	m_gameState{ common::game::GameState::NONE },
	m_turn{},
	m_image{},
	m_chat{}
{
	/* empty */
}

Game::Game(Game&& other) noexcept :
	m_players{ std::move(other.m_players) },
	m_roundNumber{ other.m_roundNumber },
	m_ownerID{ other.m_ownerID },
	m_gameSettings{ std::move(other.m_gameSettings) },
	m_turn{ std::move(other.m_turn) },
	m_image{ std::move(other.m_image) },
	m_chat{ std::move(other.m_chat) },
	m_gameState{ std::move(other.m_gameState) }
{
	other.m_roundNumber = 0;
	other.m_ownerID = 0;
	other.m_gameState = common::game::GameState::NONE;
}

Game& Game::operator=(Game&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	m_players = std::move(other.m_players);
	m_roundNumber = std::move(other.m_roundNumber);
	m_ownerID = std::move(other.m_ownerID);
	m_turn = std::move(other.m_turn);
	m_gameSettings = std::move(other.m_gameSettings);
	m_image = std::move(other.m_image);
	m_chat = std::move(other.m_chat);
	m_gameState = std::move(other.m_gameState);

	other.m_roundNumber = 0;
	other.m_ownerID = 0;
	other.m_gameState = common::game::GameState::NONE;

	return *this;
}

const std::vector<Player>& Game::GetPlayers()
{
	return m_players;
}

const Player& Game::GetPlayer(const std::string& name) const
{
	for (auto& player : m_players)
		if (player.GetName() == name)
			return player;

	throw std::exception{ "Player not found" };
}

uint8_t Game::GetRoundNumber()
{
	return m_roundNumber;
}

common::game::GameSettings& Game::GetGameSettings()
{
	return m_gameSettings;
}

Turn& Game::GetTurn()
{
	return m_turn;
}

Image& Game::GetImage()
{
	return m_image;
}

Chat& Game::GetChat()
{
	return m_chat;
}

common::game::GameState Game::GetGameState()
{
	return m_gameState;
}

void Game::SetGameState(common::game::GameState gameState)
{
	m_gameState = gameState;
}

void Game::Run()
{
	Reset();

	std::unordered_map<std::string, bool> playersDone{ m_players.size() };

	const auto& findNextPlayerLambda{ [&playersDone](std::vector<Player>& m_players) {
			return std::find_if(m_players.begin(), m_players.end(),
				[&playersDone](const Player& player) {
					return !playersDone.at(player.GetName()); });
			}
	};

	while (m_roundNumber < m_gameSettings.GetRoundCount())
	{
		for (const auto& player : m_players)
			playersDone[player.GetName()] = false;

		for (auto currPlayerIt{ m_players.begin() };
			currPlayerIt != m_players.end();
			currPlayerIt = findNextPlayerLambda(m_players))
		{
			m_turn.Reset(m_players, *currPlayerIt);

			m_gameState = common::game::GameState::PICK_WORD;

			while (m_turn.GetWord() == "")
				std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });

			m_gameState = common::game::GameState::DRAW_AND_GUESS;
			m_turn.Start(m_players, std::chrono::seconds{ m_gameSettings.GetDrawTime() });

			{
				std::lock_guard<std::mutex> lock{ m_playersMutex };

				RemoveDisconnectedPlayers();

				if (m_players.empty())
				{
					m_gameState = common::game::GameState::NONE;
					return;
				}

				std::sort(m_players.begin(), m_players.end(),
					[](const Player& lhs, const Player& rhs) {
						return lhs.GetScore() > rhs.GetScore(); });
			}
		}

		m_roundNumber++;
	}

	m_gameState = common::game::GameState::NONE;
}

void Game::RemoveDisconnectedPlayers()
{
	m_players.erase(
		std::remove_if(m_players.begin(), m_players.end(), [](const Player& player) { return !player.IsConnected(); }),
		m_players.end());
}

void Game::Reset()
{
	m_gameState = common::game::GameState::NONE;
	m_roundNumber = 0;

	{
		std::lock_guard<std::mutex> lock{ m_playersMutex };
		RemoveDisconnectedPlayers();
	}
}

void Game::AddPlayer(const Player& player)
{
	m_players.emplace_back(player);
}

void Game::RemovePlayer(const std::string& name)
{
	for (auto& player : m_players)
		if (player.GetName() == name)
		{
			m_players.erase(std::remove(m_players.begin(), m_players.end(), player), m_players.end());
			return;
		}
}
