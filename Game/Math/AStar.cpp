#include "AStar.h"

#include <algorithm>
#include <cstdlib>

std::vector<Vector2> AStar::FindPath(const RaidMap& map, const Vector2& start, const Vector2& end,
                                     const std::vector<Vector2>& blockedPositions)
{
    std::vector<Vector2> emptyPath;

    // 시작점 / 목적지가
    // 맵 밖인지 확인.
    if (!map.IsInside(start) || !map.IsInside(end))
    {
        return emptyPath;
    }

    if (IsBlocked(end, blockedPositions))
    {
        return emptyPath;
    }

    // 시작점 / 목적지가
    // 이동 가능한 곳인지 확인.
    if (!map.IsWalkable(start) || !map.IsWalkable(end))
    {
        return emptyPath;
    }

    // 이미 목적지에 있음.
    if (start == end)
    {
        return emptyPath;
    }

    // 탐색 예정 노드.
    std::vector<Node> openList;

    // 탐색 완료 노드.
    std::vector<Node> closedList;

    Node startNode;

    startNode.position = start;

    startNode.gCost = 0;

    startNode.hCost = CalculateHCost(start, end);

    startNode.hasParent = false;

    openList.emplace_back(startNode);

    // 상 / 하 / 좌 / 우
    const Vector2 directions[] = {Vector2(0, -1), Vector2(0, 1), Vector2(-1, 0), Vector2(1, 0)};

    while (!openList.empty())
    {
        // F Cost가 가장 낮은
        // 노드 검색.
        int bestIndex = 0;

        for (int ix = 1; ix < static_cast<int>(openList.size()); ++ix)
        {
            const Node& candidate = openList[ix];

            const Node& best = openList[bestIndex];

            if (candidate.GetFCost() < best.GetFCost())
            {
                bestIndex = ix;

                continue;
            }

            // F가 같으면
            // H가 작은 쪽 우선.
            if (candidate.GetFCost() == best.GetFCost()

                &&

                candidate.hCost < best.hCost)
            {
                bestIndex = ix;
            }
        }

        Node currentNode = openList[bestIndex];

        openList.erase(openList.begin() + bestIndex);

        closedList.emplace_back(currentNode);

        // 목적지 도착.
        if (currentNode.position == end)
        {
            return BuildPath(closedList, start, end);
        }

        // 주변 네 방향 탐색.
        for (const Vector2& direction : directions)
        {
            const Vector2 nextPosition = currentNode.position + direction;

            // 벽 또는 맵 밖.
            if (!map.IsWalkable(nextPosition))
            {
                continue;
            }

            if (IsBlocked(nextPosition, blockedPositions))
            {
                continue;
            }

            // 이미 탐색 완료.
            if (FindNodeIndex(closedList, nextPosition) != -1)
            {
                continue;
            }

            // 한 칸 이동 비용 = 1
            const int newGCost = currentNode.gCost + 1;

            const int openIndex = FindNodeIndex(openList, nextPosition);

            // 처음 발견한 노드.
            if (openIndex == -1)
            {
                Node nextNode;

                nextNode.position = nextPosition;

                nextNode.gCost = newGCost;

                nextNode.hCost = CalculateHCost(nextPosition, end);

                nextNode.parentPosition = currentNode.position;

                nextNode.hasParent = true;

                openList.emplace_back(nextNode);

                continue;
            }

            // 이미 Open에 있던 노드.
            Node& openNode = openList[openIndex];

            // 기존 경로가 더 짧으면
            // 수정할 필요 없음.
            if (newGCost >= openNode.gCost)
            {
                continue;
            }

            // 새로운 경로가 더 짧음.
            openNode.gCost = newGCost;

            openNode.parentPosition = currentNode.position;

            openNode.hasParent = true;
        }
    }

    // 목적지까지 길이 없음.
    return emptyPath;
}

int AStar::CalculateHCost(const Vector2& current, const Vector2& end)
{
    const int distanceX = std::abs(end.x - current.x);

    const int distanceY = std::abs(end.y - current.y);

    return distanceX + distanceY;
}

int AStar::FindNodeIndex(const std::vector<Node>& list, const Vector2& position)
{
    for (int ix = 0; ix < static_cast<int>(list.size()); ++ix)
    {
        if (list[ix].position == position)
        {
            return ix;
        }
    }

    return -1;
}

std::vector<Vector2> AStar::BuildPath(const std::vector<Node>& closedList,

                                      const Vector2& start, const Vector2& end)
{
    std::vector<Vector2> path;

    Vector2 currentPosition = end;

    while (currentPosition != start)
    {
        path.emplace_back(currentPosition);

        const int nodeIndex = FindNodeIndex(closedList, currentPosition);

        if (nodeIndex == -1)
        {
            return {};
        }

        const Node& currentNode = closedList[nodeIndex];

        if (!currentNode.hasParent)
        {
            return {};
        }

        currentPosition = currentNode.parentPosition;
    }

    std::reverse(path.begin(), path.end());

    return path;
}

bool AStar::IsBlocked(const Vector2& position, const std::vector<Vector2>& blockedPosition)
{
    for (const Vector2& blockedPosition : blockedPosition)
    {
        if (position == blockedPosition)
        {
            return true;
        }
    }

    return false;
}
