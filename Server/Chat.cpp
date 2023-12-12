#include "Chat.h"

#include "../Common/constantLiterals.h"

void Chat::Add(const utils::Message& message)
{
	m_messages.push_back(message);
}

void Chat::Emplace(utils::Message&& message)
{
	m_messages.emplace_back(std::move(message));
}

std::vector<utils::Message> Chat::GetMessagesOrdered(uint64_t start, const std::string& skipAuthor) const
{
	std::vector<utils::Message> messages;
	for (auto it = m_messages.rbegin(); it != m_messages.rend() && it->timeMilliseconds > start; it++)
		if (start == 0 || it->author != skipAuthor)
			messages.emplace_back(*it);
	return messages;
}

std::vector<crow::json::wvalue> Chat::GetMessagesOrderedJsonList(uint64_t start, const std::string& skipAuthor) const
{
	std::vector<crow::json::wvalue> messagesJson;
	for (auto it = m_messages.rbegin(); it != m_messages.rend() && it->timeMilliseconds > start; it++)
		if (start == 0 || it->author != skipAuthor)
			messagesJson.emplace_back(crow::json::wvalue{
				{ literals::jsonKeys::message::author, it->author },
				{ literals::jsonKeys::message::content, it->content },
				{ literals::jsonKeys::message::timestamp, it->timeMilliseconds } });
	return messagesJson;
}

size_t Chat::Size() const noexcept
{
	return m_messages.size();
}

bool Chat::Empty() const noexcept
{
	return m_messages.empty();
}

utils::Message& Chat::operator[](size_t index)
{
	return m_messages[index];
}