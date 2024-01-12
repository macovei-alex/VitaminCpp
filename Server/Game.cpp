#include "Game.h"

Game::Game() noexcept :
	m_players{},
	m_roundNumber{ 0 },
	m_ownerID{ 0 },
	m_gameSettings{},
	m_gameState{ common::game::GameState::NONE },
	m_turn{},
	m_image{},
	m_chat{},
	m_sharedMutex{ std::make_shared<std::mutex>() },
m_stopped{ false }
{
	m_turn.SetPlayersMutex(m_sharedMutex);
}

Game::Game(Game&& other) noexcept :
	m_players{ std::move(other.m_players) },
	m_roundNumber{ other.m_roundNumber },
	m_ownerID{ other.m_ownerID },
	m_gameSettings{ std::move(other.m_gameSettings) },
	m_turn{ std::move(other.m_turn) },
	m_image{ std::move(other.m_image) },
	m_chat{ std::move(other.m_chat) },
	m_gameState{ std::move(other.m_gameState) },
	m_sharedMutex{ std::move(other.m_sharedMutex) },
m_stopped{ other.m_stopped }
{
	other.m_roundNumber = 0;
	other.m_ownerID = 0;
	other.m_gameState = common::game::GameState::NONE;
	other.m_sharedMutex.reset();
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
	m_sharedMutex = std::move(other.m_sharedMutex);
	m_stopped = other.m_stopped;

	other.m_roundNumber = 0;
	other.m_ownerID = 0;
	other.m_gameState = common::game::GameState::NONE;
	other.m_sharedMutex.reset();
	other.m_stopped = true;

	return *this;
}

const std::vector<Player>& Game::GetPlayers() const
{
	return m_players;
}

const Player& Game::GetPlayer(const std::string& name) const
{
	if (auto playerFoundIt{ std::find_if(m_players.begin(), m_players.end(),
		[&name](const Player& player) -> bool {
			return player.GetName() == name;
		}) };
		playerFoundIt != m_players.end())
	{
		return *playerFoundIt;
	}

	throw std::exception{ "Player not found" };
}

uint8_t Game::GetRoundNumber() const
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

common::game::GameState Game::GetGameState() const
{
	return m_gameState;
}

void Game::SetGameState(common::game::GameState gameState)
{
	m_gameState = gameState;
}

std::mutex& Game::GetPlayersMutex()
{
	return *m_sharedMutex;
}

bool Game::IsRunning() const
{
	return !m_stopped;
}

void Game::Run()
{
	m_stopped = false;
	Reset();

	std::unordered_map<std::string, bool> playersDone{ m_players.size() };

	const auto& findNextPlayerLambda{ [&playersDone](std::vector<Player>& m_players) {
			return std::find_if(m_players.begin(), m_players.end(),
				[&playersDone](const Player& player) {
					if (playersDone.find(player.GetName()) == playersDone.end())
					{
						playersDone.emplace(player.GetName(), false);
						return true;
					}
					return !playersDone.at(player.GetName()); });
			}
	};

	while (!m_stopped && m_roundNumber < m_gameSettings.m_roundCount)
	{
		for (const auto& player : m_players)
			playersDone[player.GetName()] = false;

		for (auto currPlayerIt{ m_players.begin() };
			currPlayerIt != m_players.end();
			currPlayerIt = findNextPlayerLambda(m_players))
		{
			m_turn.Reset(m_players, *currPlayerIt);

			m_gameState = common::game::GameState::PICK_WORD;

			while (!m_stopped && m_turn.GetWord() == "")
				std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });

			if (m_stopped)
			{
				m_gameState = common::game::GameState::NONE;
				return;
			}

			m_gameState = common::game::GameState::DRAW_AND_GUESS;
			m_turn.Start(m_players, std::chrono::seconds{ m_gameSettings.m_drawTime }, m_stopped);

			{
				std::lock_guard<std::mutex> lock{ *m_sharedMutex };

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
	m_stopped = true;
}

void Game::RemoveDisconnectedPlayers()
{
	m_players.erase(
		std::remove_if(m_players.begin(), m_players.end(),
			[](const Player& player) { return !player.IsConnected(); }),
		m_players.end());
}

void Game::Reset()
{
	m_gameState = common::game::GameState::NONE;
	m_roundNumber = 0;

	{
		std::lock_guard<std::mutex> lock{ *m_sharedMutex };
		RemoveDisconnectedPlayers();
	}
}

void Game::Stop()
{
	m_stopped = true;
	while(m_gameState != common::game::GameState::NONE)
		std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
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

void Game::AddPlayer(const Player& player)
{
	m_players.emplace_back(player);
}
