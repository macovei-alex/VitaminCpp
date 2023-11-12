#include "Player.h"

Player::Player() :
	m_name{ "" },
	m_score{ 0 },
	m_flagGuessedCorrectWord{ false },
	m_currentScore{ 0 },
	m_gameRole{ Player::GameRole::person_guessing },
	m_roomRole{ Player::RoomRole::participants }
{
	/*EMPTY*/
}

Player::Player(const std::string& name) :
	m_name{ name },
	m_score{ 0 },
	m_flagGuessedCorrectWord{ false },
	m_currentScore{ 0 },
	m_gameRole{ Player::GameRole::person_guessing },
	m_roomRole{ Player::RoomRole::participants }
{
	/*EMPTY*/
}

Player::Player(const Player& player) :
	m_name{ player.m_name },
	m_score{ player.m_score },
	m_flagGuessedCorrectWord{ player.m_flagGuessedCorrectWord },
	m_currentScore{ player.m_currentScore },
	m_gameRole{ player.m_gameRole },
	m_roomRole{ player.m_roomRole }
{
	/*EMPTY*/
}

Player& Player::operator=(const Player& player)
{
	if (this == &player)
	{
		return *this;
	}
	m_name = player.m_name;
	m_score = player.m_score;
	m_currentScore = player.m_currentScore;
	m_flagGuessedCorrectWord = player.m_flagGuessedCorrectWord;
	m_gameRole = player.m_gameRole;
	m_roomRole = player.m_roomRole;
	return *this;

}

Player::~Player()
{
	/*EMPTY*/
}

Player::Player(Player&& other) noexcept :
	m_name{ std::move(other.m_name) },
	m_score{ other.m_score },
	m_flagGuessedCorrectWord{ other.m_flagGuessedCorrectWord },
	m_currentScore{ other.m_currentScore },
	m_gameRole{ other.m_gameRole },
	m_roomRole{ other.m_roomRole }
{
	other.m_name.clear();
	other.m_score = 0;
	other.m_currentScore = 0;
	other.m_flagGuessedCorrectWord = false;
	other.m_gameRole = Player::GameRole::person_guessing;
	other.m_roomRole = Player::RoomRole::participants;
}

Player& Player::operator=(Player&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}
	m_name = std::move(other.m_name);
	m_score = other.m_score;
	m_currentScore = other.m_currentScore;
	m_flagGuessedCorrectWord = other.m_flagGuessedCorrectWord;
	m_gameRole = other.m_gameRole;
	m_roomRole = other.m_roomRole;

	other.m_name.clear();
	other.m_score = 0;
	other.m_currentScore = 0;
	other.m_flagGuessedCorrectWord = false;
	other.m_gameRole = Player::GameRole::person_guessing;
	other.m_roomRole = Player::RoomRole::participants;

	return *this;
}

std::string Player::GetName() const
{
	return m_name;
}

int Player::GetScore() const
{
	return m_score;
}

int Player::GetCurrentScore() const
{
	return m_currentScore;
}

void Player::resetCurrentScore()
{
	m_currentScore = 0;
}

void Player::resetScore()
{
	m_score = 0;
}

void Player::addScore()
{
	m_score += m_currentScore;
}

int Player::calculateScoreDrawingPlayer(int seconds, int playerCount)
{
	if (playerCount == 0)
	{
		m_currentScore += -100;
	}
	else
	{
		m_currentScore += ((60 - (seconds / playerCount)) * 100) / 60;
	}
	return m_currentScore;
}

int Player::calculateScoreGuessingPlayer(int seconds)
{
	if (seconds < 30)
	{
		m_currentScore += 100;
	}
	else if (seconds < 60)
	{
		m_currentScore += ((60 - seconds) * 100) / 30;
	}
	else
	{
		m_currentScore += -50;
	}

	return m_currentScore;
}

Player::GameRole Player::GetGameRole() const
{
	return m_gameRole;
}

void Player::SetGameRole(GameRole gameRole)
{
	m_gameRole = gameRole;
}

Player::RoomRole Player::GetRoomRole() const
{
	return m_roomRole;
}

void Player::SetRoomRole(RoomRole roomRole)
{
	m_roomRole = roomRole;
}