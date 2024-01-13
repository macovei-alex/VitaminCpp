#include "Server.h"

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

		auto contentIterator = jsonMap.find(literals::jsonKeys::message::text);
		auto authorIterator = jsonMap.find(literals::jsonKeys::message::author);
		if (contentIterator == jsonMap.end() || authorIterator == jsonMap.end())
		{
			auto responseMessage{ "Invalid parameter keys" };
			Log(responseMessage, Logger::Level::Error);
			return crow::response(404, responseMessage);
		}

		common::Message message{
			utils::DecodeMessage(contentIterator->second),
			utils::DecodeMessage(authorIterator->second),
			utils::MillisFromDateTime(std::chrono::system_clock::now()) };

		Log(std::format("New message at ({}) from [{}]: {}\n", message.timestamp, message.author, message.text));
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