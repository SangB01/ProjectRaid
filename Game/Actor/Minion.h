#pragma once

#include <Actor/Actor.h>
#include <Math/Vector2.h>

#include <vector>

using namespace Craft;

class Minion : public Actor
{
    TYPE_DECLARATIONS(Minion, Actor)

  public:
    static constexpr int MaxHealth = 1;
    static constexpr int MaxLifetimeTurns = 3;

  public:
    Minion(const Vector2& startPosition = Vector2::Zero);
    virtual ~Minion() = default;

    virtual void Tick(float deltaTime) override;

    void ReserveCharge(const std::vector<Vector2>& movePath, const std::vector<Vector2>& warningPositions);
    void ExecuteReservedCharge();
    void StopMovement();
    void ClearCharge();

    bool HasPath() const;
    bool HasReservedPath() const;
    const std::vector<Vector2>& GetReservedPath() const;
    const std::vector<Vector2>& GetWarningPositions() const;
    bool WasPlannedThisTurn() const;

    void CompleteTurn();
    int GetRemainingTurns() const;

    void TakeDamage(int damage);
    bool IsDead() const;

  private:
    int health = MaxHealth;
    int remainingTurns = MaxLifetimeTurns;

    std::vector<Vector2> path;
    std::vector<Vector2> reservedPath;
    std::vector<Vector2> warningPositions;

    int pathIndex = 0;
    float moveTimer = 0.0f;
    float moveInterval = 0.05f;
    bool wasPlannedThisTurn = false;
};
