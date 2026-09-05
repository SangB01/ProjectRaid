#include "Player.h"

Player::Player(int playerIndex, const Vector2& startPosition)
    : Actor(L"P", startPosition, Color::White), playerIndex(playerIndex)
{
    sortingOrder = 20;
}

void Player::Tick(float deltaTime)
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

    // A* 경로의 다음 위치로 이동.
    SetPosition(path[pathIndex]);

    ++pathIndex;
    if (pathIndex >= static_cast<int>(path.size()))
    {
        ClearPath();
    }
}

void Player::SetPath(const std::vector<Vector2>& newPath)
{
    path = newPath;

    pathIndex = 0;

    moveTimer = moveInterval;
}

void Player::ClearPath()
{
    path.clear();

    pathIndex = 0;

    moveTimer = 0.0f;
}

bool Player::HasPath() const
{
    return !path.empty() && pathIndex < static_cast<int>(path.size());
}

void Player::ReservePath(const std::vector<Vector2>& newPath)
{
    reservedPath = newPath;
}

void Player::ClearReservedPath()
{
    reservedPath.clear();
}

void Player::ExecuteReservedPath()
{
    if (!HasReservedPath())
    {
        return;
    }

    SetPath(reservedPath);
    ClearReservedPath();
}

bool Player::HasReservedPath() const
{
    return !reservedPath.empty();
}

const std::vector<Vector2>& Player::GetReservedPath() const
{
    return reservedPath;
}

int Player::GetHealth() const
{
    return health;
}

void Player::TakeDamage(int damage)
{
    if (damage <= 0 || IsDead())
    {
        return;
    }

    health -= damage;

    if (health > 0)
    {
        return;
    }

    health = 0;
    ClearPath();
    ClearReservedPath();
    SetSelected(false);
    Destroy();
}

bool Player::IsDead() const
{
    return health <= 0;
}

void Player::SetSelected(bool selected)
{
    isSelected = selected;

    if (IsSelected())
    {
        color = Color::Blue;
    }
    else
    {
        color = Color::White;
    }
}

bool Player::IsSelected() const
{
    return isSelected;
}

int Player::GetPlayerIndex() const
{
    return playerIndex;
}
