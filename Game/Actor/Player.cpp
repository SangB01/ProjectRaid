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
    if (!newPath.empty())
    {
        ClearReservedCard();
    }

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

bool Player::ReserveCard(int cardId, const std::shared_ptr<Actor>& target)
{
    if (!IsActive() || cardId <= 0 || !target)
    {
        return false;
    }

    ClearReservedPath();
    reservedCardId = cardId;
    reservedCardTarget = target;
    reservedCardPosition.reset();
    return true;
}

bool Player::ReserveCardAt(int cardId, const std::shared_ptr<Actor>& target, const Vector2& position)
{
    if (!ReserveCard(cardId, target))
    {
        return false;
    }
    reservedCardPosition = position;
    return true;
}

void Player::ClearReservedCard()
{
    reservedCardId = 0;
    reservedCardTarget.reset();
    reservedCardPosition.reset();
}

bool Player::HasReservedCard() const
{
    return reservedCardId != 0;
}

int Player::GetReservedCardId() const
{
    return reservedCardId;
}

std::shared_ptr<Actor> Player::GetReservedCardTarget() const
{
    return reservedCardTarget.lock();
}

const std::optional<Vector2>& Player::GetReservedCardPosition() const
{
    return reservedCardPosition;
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

    if (hasBarrier)
    {
        hasBarrier = false;
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
    ClearReservedCard();
    SetSelected(false);
    // Keep the player registered with the level so Revive can reactivate the same actor.
    isActive = false;
}

void Player::Heal(int amount)
{
    if (IsActive() && amount > 0)
    {
        health += amount;
        if (health > MaxHealth)
        {
            health = MaxHealth;
        }
    }
}

void Player::ApplyBarrier()
{
    if (IsActive())
    {
        hasBarrier = true;
    }
}

void Player::ClearBarrier()
{
    hasBarrier = false;
}

bool Player::HasBarrier() const
{
    return hasBarrier;
}

bool Player::Revive(const Vector2& position, int restoredHealth)
{
    if (!IsDead() || restoredHealth <= 0)
    {
        return false;
    }
    health = restoredHealth < MaxHealth ? restoredHealth : MaxHealth;
    isActive = true;
    ClearPath();
    ClearReservedPath();
    ClearReservedCard();
    ClearBarrier();
    SetSelected(false);
    SetPosition(position);
    SavePrevioussState();
    return true;
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
