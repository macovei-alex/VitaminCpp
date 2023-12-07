#include "Image.h"

#include "..\Common\constantLiterals.h"

Color::Color(uint32_t rgbHex) :
	r{ (rgbHex >> 16) & 0xFF },
	g{ (rgbHex >> 8) & 0xFF },
	b{ rgbHex & 0xFF }
{
	/* empty */
}

Color::Color(int32_t rgbHex) :
	r{ uint8_t((rgbHex >> 16) & 0xFF) },
	g{ uint8_t((rgbHex >> 8) & 0xFF) },
	b{ uint8_t(rgbHex & 0xFF) }
{
	/* empty */
}

void Image::AddUpdate(const Update& update)
{
	this->m_updates.push_back(update);
}

void Image::AddUpdate(Update&& update)
{
	this->m_updates.push_back(std::move(update));
}

std::vector<Update> Image::GetUpdatesAfter(uint64_t timestamp)
{
	std::vector<Update> updates;
	for (auto it{ m_updates.rbegin() }; it != m_updates.rend() && it->timestamp >= timestamp; it++)
		updates.push_back(*it);

	return updates;
}

crow::json::wvalue Image::GetUpdatesJsonAfter(uint64_t timestamp)
{
	return UpdatesToJson(GetUpdatesAfter(timestamp));
}

crow::json::wvalue Image::UpdatesToJson(const std::vector<Update>& updates)
{
	if (updates.size() == 0)
		return crow::json::wvalue{ crow::json::wvalue::list{} };

	crow::json::wvalue::list updatesJsonList{};
	updatesJsonList.reserve(updates.size());

	for (size_t i = 0; i < updates.size() - 1; i++)
	{
		const auto& update = updates[i];
		int32_t rgb{
			(update.point.color.r << 16)
			| (update.point.color.g << 8)
			| update.point.color.b };

		crow::json::wvalue updateJson;
		updateJson = crow::json::wvalue{
			{literals::jsonKeys::draw::pointX, update.point.x },
			{literals::jsonKeys::draw::pointY, update.point.y },
			{literals::jsonKeys::draw::color, rgb }
		};
		updatesJsonList.push_back(std::move(updateJson));
	}

	const auto& finalUpdate = updates[updates.size() - 1];

	int32_t rgb{
		(finalUpdate.point.color.r << 16)
		| (finalUpdate.point.color.g << 8)
		| finalUpdate.point.color.b };

	crow::json::wvalue finalUpdateJson{
		{literals::jsonKeys::draw::pointX, finalUpdate.point.x },
		{literals::jsonKeys::draw::pointY, finalUpdate.point.y },
		{literals::jsonKeys::draw::color, rgb },
		{literals::jsonKeys::draw::timestamp, finalUpdate.timestamp }
	};

	updatesJsonList.push_back(std::move(finalUpdateJson));

	return crow::json::wvalue{ updatesJsonList };
}
