#include "RaidMap.h"

#include <Render/Renderer.h>

#include <Windows.h>

#include <fstream>
#include <algorithm>
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
    temporaryObstacles.clear();

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
    const auto isWall = [this](int targetX, int targetY) {
        return targetY >= 0 && targetY < static_cast<int>(mapData.size()) && targetX >= 0 &&
               targetX < static_cast<int>(mapData[targetY].size()) && mapData[targetY][targetX] == L'#';
    };

    for (int y = 0; y < height; ++y)
    {
        // 충돌용 '#'은 맵 데이터에 유지하고, 화면에는 별도의 벽 모양으로 그린다.
        std::wstring background = mapData[y];
        std::replace(background.begin(), background.end(), L'#', L' ');
        Renderer::Get().Submit(background, Vector2(position.x, position.y + y), Color::White, 0);

        for (int x = 0; x < static_cast<int>(mapData[y].size()); ++x)
        {
            if (mapData[y][x] != L'#')
            {
                continue;
            }

            const bool surrounded = isWall(x - 1, y) && isWall(x + 1, y) &&
                                    isWall(x, y - 1) && isWall(x, y + 1);
            const wchar_t* wallImage = surrounded ? L"█" : L"▓";

            Renderer::Get().Submit(wallImage, Vector2(position.x + x, position.y + y), Color::Darkgrey, 1);
        }
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

    if (std::find(temporaryObstacles.begin(), temporaryObstacles.end(), targetPosition) != temporaryObstacles.end())
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

void RaidMap::SetTemporaryObstacles(const std::vector<Vector2>& positions)
{
    temporaryObstacles = positions;
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
