#include "Minion.h"

Minion::Minion(const Vector2& startPosition) : Actor(L"S", startPosition, Color::Purple)
{
    sortingOrder = 20;
}

void Minion::Tick(float deltaTime)
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
        path.clear();
        pathIndex = 0;
    }
}

void Minion::ReserveCharge(const std::vector<Vector2>& movePath, const std::vector<Vector2>& newWarningPositions)
{
    reservedPath = movePath;
    warningPositions = newWarningPositions;
    wasPlannedThisTurn = true;
}

void Minion::ExecuteReservedCharge()
{
    path = reservedPath;
    reservedPath.clear();
    pathIndex = 0;
    moveTimer = moveInterval;
}

void Minion::StopMovement()
{
    path.clear();
    pathIndex = 0;
    moveTimer = 0.0f;
}

void Minion::ClearCharge()
{
    StopMovement();
    reservedPath.clear();
    warningPositions.clear();
    wasPlannedThisTurn = false;
}

bool Minion::HasPath() const
{
    return !path.empty() && pathIndex < static_cast<int>(path.size());
}

bool Minion::HasReservedPath() const
{
    return !reservedPath.empty();
}

const std::vector<Vector2>& Minion::GetReservedPath() const
{
    return reservedPath;
}

const std::vector<Vector2>& Minion::GetWarningPositions() const
{
    return warningPositions;
}

bool Minion::WasPlannedThisTurn() const
{
    return wasPlannedThisTurn;
}

void Minion::CompleteTurn()
{
    --remainingTurns;
    warningPositions.clear();
    wasPlannedThisTurn = false;

    if (remainingTurns <= 0)
    {
        Destroy();
    }
}

int Minion::GetRemainingTurns() const
{
    return remainingTurns;
}

void Minion::TakeDamage(int damage)
{
    if (damage <= 0 || IsDead())
    {
        return;
    }

    health -= damage;

    if (health <= 0)
    {
        health = 0;
        ClearCharge();
        Destroy();
    }
}

bool Minion::IsDead() const
{
    return health <= 0;
}
