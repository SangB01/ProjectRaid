#pragma once

#include <Math/Vector2.h>

#include <string>
#include <vector>

using namespace Craft;

class RaidMap
{
  public:
    RaidMap() = default;

    bool Load(const std::string& filePath);

    void Draw() const;

    bool IsWalkable(const Vector2& targetPosition) const;
    bool IsInside(const Vector2& targetPosition) const;
    void SetTemporaryObstacles(const std::vector<Vector2>& positions);

    wchar_t GetTile(const Vector2& targetPosition) const;

    bool FindFirstWalkable(Vector2& outPosition) const;

    int GetWidth() const;
    int GetHeight() const;

    const Vector2& GetPosition() const;

  private:
    std::wstring ConvertUTF8ToWide(const std::string& text) const;

  private:
    std::vector<std::wstring> mapData;
    std::vector<Vector2> temporaryObstacles;

    Vector2 position = Vector2(0, 0);
};
