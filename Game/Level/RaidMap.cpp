#include "RaidMap.h"

#include <Render/Renderer.h>

#include <Windows.h>

#include <fstream>
#include <iostream>

using namespace Craft;

bool RaidMap::Load(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        std::cout << "BaseMap Load Failed : " << filePath << '\n';

        return false;
    }

    mapData.clear();

    std::string line;

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        std::wstring wideLine = ConvertUTF8ToWide(line);

        mapData.emplace_back(wideLine);
    }

    file.close();

    if (mapData.empty())
    {
        std::cout << "Map Data Empty\n";

        return false;
    }

    return true;
}

void RaidMap::Draw() const
{
    const int height = static_cast<int>(mapData.size());

    for (int y = 0; y < height; ++y)
    {
        Renderer::Get().Submit(mapData[y],

                               Vector2(position.x, position.y + y),

                               Color::White,

                               0);
    }
}

bool RaidMap::IsInside(const Vector2& targetPosition) const
{
    const int mapX = targetPosition.x - position.x;

    const int mapY = targetPosition.y - position.y;

    if (mapY < 0 || mapY >= static_cast<int>(mapData.size()))
    {
        return false;
    }

    if (mapX < 0 || mapX >= static_cast<int>(mapData[mapY].size()))
    {
        return false;
    }

    return true;
}

bool RaidMap::IsWalkable(const Vector2& targetPosition) const
{
    if (!IsInside(targetPosition))
    {
        return false;
    }

    const wchar_t tile = GetTile(targetPosition);

    if (tile == L'#')
    {
        return false;
    }
    if (tile == L'─')
    {
        return false;
    }
    if (tile == L'│')
    {
        return false;
    }
    if (tile == L'┌')
    {
        return false;
    }
    if (tile == L'┐')
    {
        return false;
    }
    if (tile == L'┘')
    {
        return false;
    }
    if (tile == L'└')
    {
        return false;
    }

    return true;
}

wchar_t RaidMap::GetTile(const Vector2& targetPosition) const
{
    if (!IsInside(targetPosition))
    {
        return L'\0';
    }

    const int mapX = targetPosition.x - position.x;

    const int mapY = targetPosition.y - position.y;

    return mapData[mapY][mapX];
}

bool RaidMap::FindFirstWalkable(Vector2& outPosition) const
{
    for (int y = 0; y < static_cast<int>(mapData.size()); ++y)
    {
        for (int x = 0; x < static_cast<int>(mapData[y].size()); ++x)
        {
            const Vector2 targetPosition(position.x + x, position.y + y);

            if (!IsWalkable(targetPosition))
            {
                continue;
            }

            outPosition = targetPosition;

            return true;
        }
    }

    return false;
}

int RaidMap::GetWidth() const
{
    if (mapData.empty())
    {
        return 0;
    }

    int width = 0;

    for (const std::wstring& row : mapData)
    {
        const int rowWidth = static_cast<int>(row.size());

        if (rowWidth > width)
        {
            width = rowWidth;
        }
    }

    return width;
}

int RaidMap::GetHeight() const
{
    return static_cast<int>(mapData.size());
}

const Vector2& RaidMap::GetPosition() const
{
    return position;
}

std::wstring RaidMap::ConvertUTF8ToWide(const std::string& text) const
{
    if (text.empty())
    {
        return L"";
    }

    const int wideLength = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);

    if (wideLength <= 0)
    {
        return L"";
    }

    std::wstring result(wideLength, L'\0');

    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), wideLength);

    return result;
}
