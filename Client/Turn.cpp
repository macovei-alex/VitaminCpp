#include "Turn.h"
#include <iostream>
#include <limits>
#include <array>

const std::string Turn::wordsFilePath = "words.in";
std::random_device Turn::seed; 
std::mt19937 Turn::randomNumberGenerator(seed());

Turn::Turn(uint8_t turnNumber, const std::string& word, int8_t turnToDraw) :
	m_turnNumber{ turnNumber },
	m_word{ word },
	m_showLetterIDs{},
	m_turnToDraw{ turnToDraw }
{
	m_chooseWordStartTime = clock();
	m_startTime = std::numeric_limits<double>::max();
	m_fin = std::ifstream(wordsFilePath);
}

Turn::Turn(Turn& turn) :
	m_turnNumber{ turn.m_turnNumber },
	m_word{ turn.m_word },
	m_showLetterIDs{ turn.m_showLetterIDs },
	m_turnToDraw{ turn.m_turnToDraw },
	m_startTime{ turn.m_startTime },
	m_chooseWordStartTime{ turn.m_chooseWordStartTime },
	m_fin{ wordsFilePath }
{
	m_fin.seekg(turn.m_fin.tellg());
}

Turn& Turn::operator=(Turn& turn)
{
	m_turnNumber = turn.m_turnNumber;
	m_word = turn.m_word;
	m_showLetterIDs = turn.m_showLetterIDs;
	m_turnToDraw = turn.m_turnToDraw;
	m_startTime = turn.m_startTime;
	m_chooseWordStartTime = turn.m_chooseWordStartTime;

	m_fin = std::ifstream(wordsFilePath);
	m_fin.seekg(turn.m_fin.tellg());

	return *this;
}

Turn::~Turn()
{
	m_fin.close();
}

uint8_t Turn::GetTurnNumber() const
{
	return m_turnNumber;
}

double_t Turn::GetChooseTime() const
{
	return double_t(clock() - m_chooseWordStartTime) / CLOCKS_PER_SEC;
}

double_t Turn::GetPlayTime() const
{
	return double_t(clock() - m_startTime) / CLOCKS_PER_SEC;
}

std::string Turn::GetWord() const
{
	return m_word;
}

std::vector<std::string> Turn::generateWordChoices(const size_t wordsCount)
{
	std::vector<std::string> words(wordsCount);
	for(uint8_t i = 0; i < wordsCount && !m_fin.eof(); i++)
	{
		m_fin >> words[i];
	}
	return words;
}

std::string Turn::pickWord(const std::vector<std::string>& words, const size_t wordsCount) const
{
	// random word choice
	std::uniform_int_distribution<> distribution(0, wordsCount - 1);
	return words[distribution(randomNumberGenerator)];
}

void Turn::startNewTurn(std::vector<Player>& players)
{
	m_word = "";
	for (auto& player : players)
	{
		//player.resetCurrentScore();
	}
	m_startTime = clock();
}

void Turn::endTurn(std::vector<Player>& players)
{
	for (Player& player : players)
	{
		player.addScore();
	}
}