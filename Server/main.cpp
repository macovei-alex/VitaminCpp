#include "Server.h"
#include "Game.h"

import utilitiesMod;
#include "utilities.h"

// https://docs.google.com/document/d/17Up9pbhwRiUGXRCoE4aJbuTiBvuXILxOdLOBuDKdr3E/edit

int main()
{
	Server::getInstance().setSettingsFromFile("serverSettings.txt").run();
	return 0;
}