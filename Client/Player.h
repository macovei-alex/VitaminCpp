#pragma once
#include <cstdint>
#include <string>
#include "Turn.h"

class Player
{
public:
	Player(const std::string& m_name);
	Player(const Player& player);
	~Player();
	uint16_t GetScore();
	uint16_t GetCurrentScore();
	void SetCurrentScore();
	void guessWord(std::string guessedWord, Turn turn);
	void addScore();

private:
	std::string m_name;
	uint16_t m_score;
	uint16_t m_currentScore;
	bool m_flagGuessedCorrrectWord;
};

