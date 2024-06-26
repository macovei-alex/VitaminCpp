#include "services.h"

void services::SendImageUpdates(uint64_t gameID, const std::vector<common::img::Point>& points, std::ostream& outStream, std::ostream& errStream)
{
	static const std::string urlBlueprint{ std::string{literals::routes::baseAddress} + literals::routes::game::draw::updates + "/" };

	std::string url{ urlBlueprint + std::to_string(gameID) };

	try
	{
		crow::json::wvalue test;
		crow::json::wvalue::list pointsJsonList;
		pointsJsonList.reserve(points.size());

		std::ranges::for_each(points, [&pointsJsonList](const auto& point) {
			pointsJsonList.emplace_back(crow::json::wvalue{ {
				{literals::jsonKeys::draw::pointX, point.x},
				{literals::jsonKeys::draw::pointY, point.y},
				{literals::jsonKeys::draw::color, point.color.ToInt32()}} });
			});

		/*for (auto& point : points)
			pointsJsonList.emplace_back(crow::json::wvalue{ {
				{literals::jsonKeys::draw::pointX, point.x},
				{literals::jsonKeys::draw::pointY, point.y},
				{literals::jsonKeys::draw::color, point.color.ToInt32()}} });*/

		crow::json::wvalue flattened{ pointsJsonList };
		std::string str{ flattened.dump() };
		outStream << str;

		auto response{ cpr::Put(
			cpr::Url{ url },
			cpr::Payload{ {literals::jsonKeys::draw::points, str} }) };

		if (response.status_code != 200 && response.status_code != 201)
		{
			if (!response.reason.empty())
				throw std::exception{ response.reason.c_str() };
			else
				throw std::exception{ "Server didn't provide an explanation" };
		}
	}
	catch (const std::exception& exception)
	{
		errStream << "[Drawing sender]: " << exception.what() << '\n';
	}
}

std::vector<common::img::Point> services::ReceiveImageUpdates(uint64_t gameID, std::ostream& errStream)
{
	static const std::string urlBlueprint{ std::string{literals::routes::baseAddress} + literals::routes::game::draw::updates + "/" };
	static uint64_t lastTimestamp{ 0 };
	static bool serverErrorDetected{ false };

	std::string url{ urlBlueprint + std::to_string(gameID) };

	try
	{
		auto response{ cpr::Get(
			cpr::Url{ url },
			cpr::Parameters{ {literals::jsonKeys::draw::timestamp, std::to_string(lastTimestamp)} }) };

		if (response.status_code != 200 && response.status_code != 201)
		{
			if (!serverErrorDetected)
			{
				serverErrorDetected = true;
				if (!response.reason.empty())
					throw std::exception{ response.reason.c_str() };
				else
					throw std::exception{ "Server didn't provide an explanation" };
			}
		}
		else
			serverErrorDetected = false;

		if (serverErrorDetected)
			return {};

		auto pointsJsonList{ crow::json::load(response.text) };
		if (pointsJsonList.size() == 0)
			lastTimestamp = std::max(lastTimestamp, 1ULL);

		else if (pointsJsonList.size() == 1 && pointsJsonList[0].has(literals::error))
			throw std::exception{ "Json has error" };

		else
			lastTimestamp = pointsJsonList[pointsJsonList.size() - 1][literals::jsonKeys::draw::timestamp].u() + 1;

		std::vector<common::img::Point> points;
		points.reserve(pointsJsonList.size());

		std::ranges::for_each(pointsJsonList, [&points](const auto& pointJson) {
			common::img::Point point{
				pointJson[literals::jsonKeys::draw::pointX].i(),
				pointJson[literals::jsonKeys::draw::pointY].i(),
				pointJson[literals::jsonKeys::draw::color].i() };

			points.emplace_back(point.x, point.y, common::img::Color{ point.color.r, point.color.g, point.color.b });
			});

		/*for (auto& pointJson : pointsJsonList)
		{
			common::img::Point point{
				pointJson[literals::jsonKeys::draw::pointX].i(),
				pointJson[literals::jsonKeys::draw::pointY].i(),
				pointJson[literals::jsonKeys::draw::color].i() };

			points.emplace_back(point.x, point.y, common::img::Color{ point.color.r, point.color.g, point.color.b });
		}*/

		return points;
	}
	catch (const std::exception& exception)
	{
		errStream << "[Drawing updater]: " << exception.what() << '\n';
		throw exception;
	}
}