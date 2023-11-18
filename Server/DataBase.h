#pragma once
#include <string>
#include <vector>
#include <crow.h>
#include <sqlite_orm/sqlite_orm.h>
namespace sql = sqlite_orm;

struct Words
{
	int id;
    std::string word;
	int difficulty;
};

struct Player
{
	std::string playerName;//primary key
	std::string password;
	int gamesPlayed;
	int totalScore;
};

struct GameHistory
{
	int id;
	std::string playerName;//foreign key
	int score;
	int difficulty;
	std::string date;
};

inline auto createStorage(const std::string& filename)
{
	return sql::make_storage(filename,
		sql::make_table("words",
			sql::make_column("id", &Words::id, sql::primary_key().autoincrement()),
			sql::make_column("word", &Words::word),
			sql::make_column("difficulty", &Words::difficulty)),
		sql::make_table("player",
			sql::make_column("playerName", &Player::playerName, sql::primary_key()),
			sql::make_column("password", &Player::password),
			sql::make_column("gamesPlayed", &Player::gamesPlayed),
			sql::make_column("totalScore", &Player::totalScore)),
		sql::make_table("gameHistory",
			sql::make_column("id", &GameHistory::id, sql::primary_key().autoincrement()),
			//make column name same as foreign key
			sql::make_column("playerName", &GameHistory::playerName),
			sql::foreign_key(&GameHistory::playerName).references(&Player::playerName),
			sql::make_column("score", &GameHistory::score),
			sql::make_column("difficulty", &GameHistory::difficulty),
			sql::make_column("date", &GameHistory::date))
		);
}