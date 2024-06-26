#include "common.h"

#include <algorithm>

using namespace common::game;

const uint16_t GameSettings::s_defaultChooseWordOptionCount = 3;
const uint16_t GameSettings::s_defaultDrawTime = 30;
const uint16_t GameSettings::s_defaultRoundCount = 4;

GameSettings::GameSettings() noexcept :
	m_roundCount{ s_defaultRoundCount },
	m_drawTime{ s_defaultDrawTime },
	m_chooseWordOptionCount{ s_defaultChooseWordOptionCount }
{
	/* empty */
}

GameSettings::GameSettings(uint16_t roundCount, uint16_t drawTime, uint16_t chooseWordOtionCount) noexcept :
	m_roundCount{ roundCount },
	m_drawTime{ drawTime },
	m_chooseWordOptionCount{ chooseWordOtionCount }
{
	/* empty */
}
