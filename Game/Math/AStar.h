#pragma once

#include <Level/RaidMap.h>
#include <Math/Vector2.h>

#include <vector>

using namespace Craft;

class AStar
{
  private:
    struct Node
    {
        Vector2 position = Vector2::Zero;

        int gCost = 0;
        int hCost = 0;

        Vector2 parentPosition = Vector2::Zero;

        bool hasParent = false;

        int GetFCost() const
        {
            return gCost + hCost;
        }
    };

  public:
    static std::vector<Vector2> FindPath(const RaidMap& map, const Vector2& start, const Vector2& end,
                                         const std::vector<Vector2>& blockedPositions = {});

  private:
    static int CalculateHCost(const Vector2& current, const Vector2& end);

    static int FindNodeIndex(const std::vector<Node>& list, const Vector2& position);

    static std::vector<Vector2> BuildPath(const std::vector<Node>& closedList, const Vector2& start,
                                          const Vector2& end);

    static bool IsBlocked(const Vector2& position, const std::vector<Vector2>& blockedPosition);
};