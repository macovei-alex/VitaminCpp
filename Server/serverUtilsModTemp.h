#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <map>

namespace utils
{
	struct Message
	{
		std::string content;
		std::string author;
		uint64_t timeMilliseconds;

		Message() noexcept = default;
		Message(std::string&& content, std::string&& author, uint64_t milliseconds) noexcept;
		Message(const Message& other) noexcept = default;
		Message(Message&& other) noexcept = default;
		Message& operator=(const Message& other) noexcept = default;
		Message& operator=(Message&& other) noexcept = default;
	};

	std::vector<std::string> SplitToVec(const std::string& str, const std::string& delim);
	// Daca functia de mai sus intoarce un std::vector&&, atunci se creeaza o eroare la instructiunea [return std::move(result);]

	std::pair<std::string, std::string> SplitToPair(const std::string& str, const std::string& delim);
	std::string DecodeMessage(const std::string& message);
	uint64_t DateTimeAsInteger(std::chrono::system_clock::time_point dateTime = std::chrono::system_clock::now());
	std::map<std::string, std::string> ParseRequestBody(const std::string& requestBody);
}