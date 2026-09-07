#include "Boss.h"

Boss::Boss(const Vector2& startPosition) : Actor(L"B", startPosition, Color::Red)
{
    sortingOrder = 20;
}

void Boss::Tick(float deltaTime)
{
    super::Tick(deltaTime);

    if (!HasPath())
    {
        return;
    }

    moveTimer += deltaTime;

    if (moveTimer < moveInterval)
    {
        return;
    }

    moveTimer = 0.0f;
    SetPosition(path[pathIndex]);

    ++pathIndex;
    if (pathIndex >= static_cast<int>(path.size()))
    {
        ClearPath();
    }
}

void Boss::SetPath(const std::vector<Vector2>& newPath)
{
    path = newPath;
    pathIndex = 0;
    moveTimer = moveInterval;
}

void Boss::ClearPath()
{
    path.clear();
    pathIndex = 0;
    moveTimer = 0.0f;
}

bool Boss::HasPath() const
{
    return !path.empty() && pathIndex < static_cast<int>(path.size());
}

void Boss::ReservePath(const std::vector<Vector2>& newPath)
{
    reservedPath = newPath;
}

void Boss::ClearReservedPath()
{
    reservedPath.clear();
}

void Boss::ExecuteReservedPath()
{
    if (!HasReservedPath())
    {
        return;
    }

    SetPath(reservedPath);
    ClearReservedPath();
}

bool Boss::HasReservedPath() const
{
    return !reservedPath.empty();
}

const std::vector<Vector2>& Boss::GetReservedPath() const
{
    return reservedPath;
}

void Boss::PlanAction(ActionType actionType, const std::vector<Vector2>& movePath, const Vector2& destination,
                      const std::vector<Vector2>& warningPositions)
{
    ClearPlannedAction();

    if (actionType == ActionType::None)
    {
        return;
    }

    plannedActionType = actionType;
    plannedDestination = destination;
    attackWarningPositions = warningPositions;

    if (!movePath.empty())
    {
        ReservePath(movePath);
    }
}

void Boss::ClearPlannedAction()
{
    ClearReservedPath();
    attackWarningPositions.clear();
    plannedActionType = ActionType::None;
    plannedDestination = GetPosition();
}

Boss::ActionType Boss::GetPlannedActionType() const
{
    return plannedActionType;
}

const Vector2& Boss::GetPlannedDestination() const
{
    return plannedDestination;
}

const std::vector<Vector2>& Boss::GetAttackWarningPositions() const
{
    return attackWarningPositions;
}

void Boss::TakeDamage(int damage)
{
    if (damage <= 0 || IsDead())
    {
        return;
    }

    health -= damage;

    if (health <= 0)
    {
        health = 0;
        ClearPath();
        ClearPlannedAction();
        Destroy();
    }
}

int Boss::GetHealth() const
{
    return health;
}

bool Boss::IsDead() const
{
    return health <= 0;
}
