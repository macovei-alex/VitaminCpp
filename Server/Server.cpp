#include "Server.h"

#include <format>
#include <stack>

#include "..\Common\constantLiterals.h"

Server* Server::s_instance = nullptr;

Server::Server() :
	m_app{ },
	m_chats{ },
	m_port{ 0 },
	m_IPAddress{ "127.0.0.1" }
{
	/* empty */
}

Server& Server::GetInstance()
{
	if (!s_instance)
		s_instance = new Server();
	return *s_instance;
}

Server& Server::AllHandlers()
{
	this->TestHandlers().ChatHandlers().RoomHandlers().DrawingHandlers();
	return *this;
}

Server& Server::TestHandlers()
{
	CROW_ROUTE(m_app, literals::routes::test)([]() {
		return "Test connection succesful\n";
		});

	return *this;
}

Server& Server::ChatHandlers()
{
	// Input server controller
	CROW_ROUTE(m_app, literals::routes::game::chatParam).methods(crow::HTTPMethod::PUT)
		([this](const crow::request& request, uint64_t gameID) {

		if (this->m_chats.find(gameID) == this->m_chats.end())
			return crow::response(404, "Invalid game ID");
		if (request.body.empty())
			return crow::response(404, "Empty message");

		const auto informationVector{ std::move(utils::SplitToVec(request.body, "&")) };
		std::map<std::string, std::string> urlParamsMap;

		for (const auto& informationExpression : informationVector)
		{
			auto urlParamPair{ std::move(utils::SplitToPair(informationExpression, "=")) };
			urlParamsMap.emplace(std::move(urlParamPair));
		}

		uint64_t timeMillis = utils::DateTimeAsInteger(std::chrono::system_clock::now());
		utils::Message message{
			std::move(utils::DecodeMessage(urlParamsMap[literals::jsonKeys::message::content])),
			std::move(utils::DecodeMessage(urlParamsMap[literals::jsonKeys::message::author])),
			timeMillis
		};

		std::cout << std::format("[{} at {}]: {}\n",
			message.author,
			message.timeMilliseconds,
			message.content);
		this->m_chats[gameID].emplace_back(std::move(message));

		return crow::response(200);
		});


	// Output server controller
	CROW_ROUTE(m_app, literals::routes::game::chatParam).methods(crow::HTTPMethod::GET)
		([this](const crow::request& request, uint64_t gameID) {

		static const crow::json::wvalue	errorValue{ {
			{literals::jsonKeys::message::author, literals::error},
			{literals::jsonKeys::message::content, literals::error},
			{literals::jsonKeys::message::timePoint, std::to_string(0)}} };

		if (this->m_chats.find(gameID) == this->m_chats.end())
			return errorValue;
		if (!request.url_params.get(literals::jsonKeys::message::timePoint))
			return errorValue;

		auto& chat = this->m_chats[gameID];

		uint64_t from = std::stoll(request.url_params.get(literals::jsonKeys::message::timePoint));
		const std::string senderName{ std::move(request.url_params.get(literals::jsonKeys::message::author)) };

		std::stack<crow::json::wvalue> messagesStack;
		for (int i = chat.size() - 1; i >= 0 && chat[i].timeMilliseconds >= from; i--)
		{
			if (from == 0 || chat[i].author != senderName)
				messagesStack.push(crow::json::wvalue{
					{literals::jsonKeys::message::content, chat[i].content},
					{literals::jsonKeys::message::author, chat[i].author},
					{literals::jsonKeys::message::timePoint, chat[i].timeMilliseconds} });
		}

		std::vector<crow::json::wvalue> messagesVector;
		messagesVector.reserve(messagesStack.size());
		while (!messagesStack.empty())
		{
			messagesVector.emplace_back(std::move(messagesStack.top()));
			messagesStack.pop();
		}

		return crow::json::wvalue{ messagesVector };
		});

	return *this;
}

Server& Server::RoomHandlers()
{
	// Create roon
	CROW_ROUTE(m_app, literals::routes::room::create).methods(crow::HTTPMethod::GET)
		([this](const crow::request& request) {
		uint64_t newGameID = 0;
		if (!this->m_chats.empty())
			newGameID = m_chats.rbegin()->first + 1;

		m_chats.insert({ newGameID, {} });

		return crow::json::wvalue{ { {literals::jsonKeys::roomID, newGameID } } };
		});

	// Player join
	CROW_ROUTE(m_app, literals::routes::room::connectParam).methods(crow::HTTPMethod::GET)
		([this](const crow::request& request, uint64_t roomID) {

		if (this->m_chats.find(roomID) == this->m_chats.end())
			return crow::response(404, "Invalid room ID < " + std::to_string(roomID) + " >");
		return crow::response(200, "Connection succesful to room < " + std::to_string(roomID) + " >");
		});

	// Player left
	CROW_ROUTE(m_app, literals::routes::room::disconnectParam).methods(crow::HTTPMethod::GET)
		([this](const crow::request& request, uint64_t) {
		std::string name = request.url_params.get("name");
		std::string lobbyStateParam = request.url_params.get("lobbyState");
		if (name.empty() || lobbyStateParam.empty())
		{
			return crow::response(404);
		}
		return crow::response(200, "Player left Lobby");
		});

	return *this;
}

Server& Server::DrawingHandlers()
{
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
	try
	{
		if (m_port == 0)
			throw std::exception("Port number not set");
		if (m_IPAddress.empty())
			throw std::exception("IP Address not set");
		m_app.bindaddr(m_IPAddress).port(m_port).multithreaded().run();
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << '\n';
	}
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
	const std::map<std::string, ServerSetting> settingsMap
	{
		{"allHandlers", allHandlers},
		{"chatHandlers", chatHandlers},
		{"roomHandlers", roomHandlers},
		{"drawingHandlers", drawingHandlers}
	};

	std::ifstream file{ filePath };
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
			break;
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
	return *this;
}