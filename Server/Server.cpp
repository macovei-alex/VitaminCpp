#include "Server.h"

#include <format>
#include <stack>

#include "constantLiterals.h"

Server::Server() :
	m_app{ },
	m_games{ },
	m_port{ 0 },
	m_IPAddress{ "127.0.0.1" },
	m_database{ "database.sqlite" },
	m_logger{ std::make_unique<Logger>("server.log") }
{
	Log(m_database.ResetPlayerAccounts().reason);
}

Server& Server::AllHandlers()
{
	return (*this)
		.TestHandlers()
		.ChatHandlers()
		.RoomHandlers()
		.AccountHandlers()
		.DrawingHandlers()
		.GameSettingsHandlers();
}

Server& Server::TestHandlers()
{
	CROW_ROUTE(m_app, literals::routes::test)([]() {
		return "Test connection succesful\n";
		});

	Log("Test handler set");
	return *this;
}

Server& Server::ChatHandlers()
{
	CROW_ROUTE(m_app, literals::routes::game::chat::param).methods(crow::HTTPMethod::Put)
		([this](const crow::request& request, uint64_t gameID) {

		auto gameIt = this->m_games.find(gameID);
		if (gameIt == this->m_games.end())
		{
			auto responseMessage{ std::format("Invalid room ID < {} >", gameID) };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		if (request.body.empty())
		{
			auto responseMessage{ "Empty request body" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		auto& chat = gameIt->second.GetChat();

		auto jsonMap{ utils::ParseRequestBody(request.body) };

		auto contentIterator = jsonMap.find(literals::jsonKeys::message::content);
		auto authorIterator = jsonMap.find(literals::jsonKeys::message::author);
		if (contentIterator == jsonMap.end() || authorIterator == jsonMap.end())
		{
			auto responseMessage{ "Invalid parameter keys" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		utils::Message message{
			utils::DecodeMessage(contentIterator->second),
			utils::DecodeMessage(authorIterator->second),
			utils::DateTimeAsInteger(std::chrono::system_clock::now()) };

		Log(std::format("New message at ({}) from [{}]: {}\n", message.timeMilliseconds, message.author, message.content));
		chat.Emplace(std::move(message));

		return crow::response(200);
			});


	CROW_ROUTE(m_app, literals::routes::game::chat::param).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request, uint64_t gameID) {

		static const crow::json::wvalue	errorValue{ crow::json::wvalue::list{{literals::error, literals::emptyCString}} };

		auto gameIt = this->m_games.find(gameID);
		if (gameIt == this->m_games.end())
		{
			Log(std::format("Invalid room ID < {} >", gameID), Logger::Level::Error);
			return errorValue;
		}

		const auto& chat = gameIt->second.GetChat();

		if (chat.Empty())
			return crow::json::wvalue{ crow::json::wvalue::list{} };

		uint64_t start;
		std::string author;
		try
		{
			if (char* startChar = request.url_params.get(literals::jsonKeys::message::timestamp);
				startChar != nullptr)
				start = std::stoull(startChar);
			else
				throw std::exception("Timestamp key not found");

			if (char* authorChar = request.url_params.get(literals::jsonKeys::message::author);
				authorChar != nullptr)
				author = std::string{ authorChar };
			else
				throw std::exception("Author key not found");
		}
		catch (const std::exception& ex)
		{
			Log(ex.what(), Logger::Level::Error);
			std::cerr << ex.what() << '\n';
			return errorValue;
		}

		return crow::json::wvalue{ chat.GetMessagesOrderedJsonList(start, author) };
			});

	Log("Chat handlers set");
	return *this;
}

Server& Server::RoomHandlers()
{
	CROW_ROUTE(m_app, literals::routes::game::create).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request) {
		uint64_t newGameID = 0;
		if (!this->m_games.empty())
			newGameID = m_games.rbegin()->first + 1;

		m_games.emplace(newGameID, Game());

		Log(std::format("New room created with ID < {} >", newGameID), Logger::Level::Info);
		return crow::json::wvalue{ {literals::jsonKeys::game::ID, newGameID } };
			});


	CROW_ROUTE(m_app, literals::routes::game::connectParam).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request, uint64_t gameID) {

		auto gameIt = this->m_games.find(gameID);
		if (gameIt == m_games.end())
		{
			auto responseMessage{ std::format("Invalid room id < {} >", gameID) };
			Log(responseMessage);
			return crow::response(404, responseMessage);
		}

		auto responseMessage{ std::format("Connection to room < {} > successful", gameID) };
		Log(responseMessage, Logger::Level::Info);
		return crow::response(200, responseMessage);
			});


	/*CROW_ROUTE(m_app, literals::routes::game::disconnectParam).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request, uint64_t) {
		std::string name = request.url_params.get("name");
		std::string lobbyStateParam = request.url_params.get("lobbyState");
		if (name.empty() || lobbyStateParam.empty())
		{
			return crow::response(404);
		}
		return crow::response(200, "Player left Lobby");
		});*/

	Log("Room handlers set");
	return *this;
}

Server& Server::AccountHandlers()
{
	CROW_ROUTE(m_app, literals::routes::account::signIn).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request) {

		const char* usernameChar = request.url_params.get(literals::jsonKeys::account::username);
		const char* passwordChar = request.url_params.get(literals::jsonKeys::account::password);
		if (!usernameChar || !passwordChar)
			return crow::response(404, "Invalid parameter keys");

		std::string username{ usernameChar };
		std::string password{ passwordChar };

		if (username.empty() || password.empty())
		{
			auto responseMessage{ std::format("Invalid username < {} > or password < {} >", username, password) };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		db::ReturnValue returnValue{ std::move(m_database.SignIn(username, password)) };
		if (!returnValue.success)
		{
			Log(returnValue.reason, Logger::Level::Error);
			return crow::response(404, returnValue.reason);
		}

		auto responseMessage{ std::format("Player logged in as < {} >", username) };
		Log(responseMessage, Logger::Level::Info);
		return crow::response(200, responseMessage);
			});


	CROW_ROUTE(m_app, literals::routes::account::singUp).methods(crow::HTTPMethod::Post)
		([this](const crow::request& request) {

		if (request.body.empty())
		{
			auto responseMessage{ "Empty request body" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, "Empty request body");
		}

		auto jsonMap{ utils::ParseRequestBody(request.body) };

		auto usernameIterator = jsonMap.find(literals::jsonKeys::account::username);
		auto passwordIterator = jsonMap.find(literals::jsonKeys::account::password);
		if (usernameIterator == jsonMap.end() || passwordIterator == jsonMap.end())
		{
			auto responseMessage{ "Invalid parameter keys" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		std::string username{ std::move(usernameIterator->second) };
		std::string password{ std::move(passwordIterator->second) };

		if (username.empty() || password.empty())
		{
			auto responseMessage{ std::format("Invalid username < {} > or password < {} >", username, password) };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		db::ReturnValue returnValue{ std::move(m_database.SignUp(username, password)) };
		if (!returnValue.success)
		{
			Log(returnValue.reason, Logger::Level::Error);
			return crow::response(404, returnValue.reason);
		}

		auto responseMessage{ std::format("Player logged in as < {} >", username) };
		Log(responseMessage, Logger::Level::Info);
		return crow::response(200, responseMessage);
			});


	CROW_ROUTE(m_app, literals::routes::account::signOut).methods(crow::HTTPMethod::Put)
		([this](const crow::request& request) {

		if (request.body.empty())
		{
			auto responseMessage{ "Empty request body" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		auto jsonMap{ utils::ParseRequestBody(request.body) };

		auto usernameIterator = jsonMap.find(literals::jsonKeys::account::username);
		if (usernameIterator == jsonMap.end())
		{
			auto responseMessage{ "Invalid parameter keys" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		std::string username{ std::move(usernameIterator->second) };

		if (username.empty())
		{
			auto responseMessage{ std::format("Invalid username < {} >", username) };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		db::ReturnValue returnValue{ std::move(m_database.SignOut(username)) };
		if (!returnValue.success)
		{
			Log(returnValue.reason, Logger::Level::Error);
			return crow::response(404, returnValue.reason);
		}

		Log(std::format("Player < {} > logged out", username), Logger::Level::Info);
		return crow::response(200, std::format("Player < {} > logged out", username));
			});

	Log("Account handlers set");
	return *this;
}

Server& Server::DrawingHandlers()
{
	CROW_ROUTE(m_app, literals::routes::game::draw::updatesParam).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request, uint64_t gameID) {

		static const crow::json::wvalue errorValue{ crow::json::wvalue::list{{literals::error, literals::emptyCString}} };

		uint64_t timestamp;

		try
		{
			if (char* timestampChar = request.url_params.get(literals::jsonKeys::draw::timestamp);
				timestampChar != nullptr)
				timestamp = std::stoull(timestampChar);
			else
				throw std::exception("Timestamp key not found");
		}
		catch (const std::exception& ex)
		{
			Log(ex.what(), Logger::Level::Error);
			std::cerr << ex.what() << '\n';
			return errorValue;
		}

		auto updates = m_games[gameID].GetImage().GetUpdatesJsonAfter(timestamp);
		return updates;
			});


	CROW_ROUTE(m_app, literals::routes::game::draw::updatesParam).methods(crow::HTTPMethod::Put)
		([this](const crow::request& request, uint64_t gameID) {

		const static std::string pointXStrKey{ literals::jsonKeys::draw::pointX };
		const static std::string pointYStrKey{ literals::jsonKeys::draw::pointY };
		const static std::string colorStrKey{ literals::jsonKeys::draw::color };

		if (request.body.empty())
			return crow::response(404, "empty request body");

		auto gameIt = m_games.find(gameID);
		if (gameIt == m_games.end())
			return crow::response(404, std::format("Invalid game ID < {} >", gameID));

		auto jsonMap{ utils::ParseRequestBody(utils::DecodeMessage(request.body)) };
		auto jsonVector{ utils::ListOfMapsFromJsonListStr(jsonMap[literals::jsonKeys::draw::points]) };

		/*for (const auto& jsonPoint : jsonVector)
		{
			for (const auto& [key, value] : jsonPoint)
				if (std::holds_alternative<int64_t>(value))
					std::cout << key << " : " << std::get<int64_t>(value) << '\n';
				else
					std::cout << key << " : " << std::get<std::string>(value) << '\n';
			std::cout << "\n\n";
		}*/

		for (const auto& pointMap : jsonVector)
		{
			try
			{
				common::img::Point point{
					std::get<int64_t>(pointMap.at(pointXStrKey)),
					std::get<int64_t>(pointMap.at(pointYStrKey)),
					std::get<int64_t>(pointMap.at(colorStrKey))
				};

				gameIt->second.GetImage().AddUpdate(common::img::Update{ point, utils::DateTimeAsInteger(std::chrono::system_clock::now()) });
			}
			catch (const std::exception& exception)
			{
				Log(exception.what(), Logger::Level::Error);
				std::cerr << exception.what() << '\n';
			}
		}

		Log("new updates added", Logger::Level::Info);
		return crow::response(200);
			});

	Log("Draw handlers set");
	return *this;
}

Server& Server::GameSettingsHandlers()
{
	CROW_ROUTE(m_app, literals::routes::game::settings::param).methods(crow::HTTPMethod::Get)
		([this](const crow::request& request, uint64_t gameID) {

		static const crow::json::wvalue errorValue{ {literals::error, literals::emptyCString} };

		if (m_games.find(gameID) == m_games.end())
		{
			Log(std::format("Invalid game ID < {} >", gameID), Logger::Level::Error);
			return errorValue;
		}

		Log(std::format("Game settings requested for game < {} >", gameID), Logger::Level::Info);
		return crow::json::wvalue{
			{literals::jsonKeys::settings::drawTime, m_games[gameID].GetGameSettings().GetDrawTime()},
			{literals::jsonKeys::settings::roundCount, m_games[gameID].GetGameSettings().GetRoundCount()},
			{literals::jsonKeys::settings::chooseWordOptionCount, m_games[gameID].GetGameSettings().GetChooseWordOptionCount()} };
			});


	CROW_ROUTE(m_app, literals::routes::game::settings::param).methods(crow::HTTPMethod::Put)
		([this](const crow::request& request, uint64_t gameID) {

		if (request.body.empty())
		{
			Log("Empty request body", Logger::Level::Error);
			return crow::response(404, "Empty request body");
		}

		if (m_games.find(gameID) == m_games.end())
		{
			std::string errorString{ std::format("Invalid game ID < {} >", gameID) };
			Log(errorString, Logger::Level::Error);
			return crow::response(404, errorString);
		}

		auto& game = m_games[gameID];

		auto jsonMap{ utils::ParseRequestBody(request.body) };

		try
		{
			uint64_t drawTime = std::stoi(jsonMap.find(literals::jsonKeys::settings::drawTime)->second);
			game.GetGameSettings().SetDrawTime(drawTime);

			int64_t roundCount = std::stoi(jsonMap.find(literals::jsonKeys::settings::roundCount)->second);
			game.GetGameSettings().SetRoundCount(roundCount);

			int64_t chooseWordOptionCount = std::stoi(jsonMap.find(literals::jsonKeys::settings::chooseWordOptionCount)->second);
			game.GetGameSettings().SetDrawTime(chooseWordOptionCount);
		}
		catch (const std::exception& exception)
		{
			Log("Invalid parameter values", Logger::Level::Error);
			return crow::response(404, "Invalid parameter values");
		}

		return crow::response(200);
			});

	Log("Game settings handlers set");
	return *this;
}

Server& Server::IPAddress(const std::string& IPAddress)
{
	m_IPAddress = IPAddress;
	return *this;
}

Server& Server::Port(uint16_t portNumber)
{
	m_port = portNumber;
	return *this;
}

void Server::Run()
{
	if (m_port == 0)
	{
		Log("Port not set", Logger::Level::Error);
		return;
	}
	if (m_IPAddress.empty())
	{
		Log("IP Address not set", Logger::Level::Error);
		return;
	}

	Log("Server ready to run");
	m_app.bindaddr(m_IPAddress).port(m_port).multithreaded().run();
}

void Server::Log(const std::string_view& message, Logger::Level level)
{
	m_logger->Log(message, level);
}

Server& Server::SetSettingsFromFile(const std::string& filePath)
{
	utils::NavigateToProjectDirectory();

	enum ServerSetting
	{
		allHandlers,
		chatHandlers,
		roomHandlers,
		drawingHandlers
	};
	static const std::map<std::string, ServerSetting> settingsMap
	{
		{"allHandlers", allHandlers},
		{"chatHandlers", chatHandlers},
		{"roomHandlers", roomHandlers},
		{"drawingHandlers", drawingHandlers}
	};

	std::ifstream file{ filePath };

	if(!file.is_open())
	{
		Log(std::format("Failed to open settings file < {} >", filePath), Logger::Level::Error);
		return *this;
	}

	std::string line;

	std::getline(file, line);
	this->IPAddress(line);
	std::getline(file, line);
	this->Port(std::stoi(line));

	while (std::getline(file, line))
	{
		ServerSetting setting = settingsMap.find(line)->second;
		switch (setting)
		{
		case allHandlers:
			this->AllHandlers();
			return *this;
		case chatHandlers:
			this->ChatHandlers();
			break;
		case roomHandlers:
			this->RoomHandlers();
			break;
		case drawingHandlers:
			this->DrawingHandlers();
			break;
		}
	}

	m_logger = std::make_unique<Logger>("server.log");
	m_logger->Log(std::format("Settings Successfuly set from the settings file < {} >", filePath));
	return *this;
}
