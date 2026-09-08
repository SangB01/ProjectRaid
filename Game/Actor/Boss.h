#pragma once

#include <Actor/Actor.h>
#include <Math/Vector2.h>

#include <vector>

using namespace Craft;

class Boss : public Actor
{
    TYPE_DECLARATIONS(Boss, Actor)

  public:
    enum class ActionType
    {
        None,
        LaserAttack,
        NearbyAttack,
        ConeAttack,
        TeleportAttack,
        SummonMinions,
        SpecialAttack
    };

    enum class BossPhase
    {
        Phase1,
        Phase2,
        Phase3
    };

    struct BossPhaseConfig
    {
        int nearbyRadius;
        int coneRange;
        int laserDirectionCount;
        int attackCount;
    };

    static constexpr int MaxHealth = 100;

  public:
    Boss(const Vector2& startPosition = Vector2::Zero);
    virtual ~Boss() = default;

    virtual void Tick(float deltaTime) override;

    void SetPath(const std::vector<Vector2>& newPath);
    void ClearPath();
    bool HasPath() const;

    void ReservePath(const std::vector<Vector2>& newPath);
    void ClearReservedPath();
    void ExecuteReservedPath();

    bool HasReservedPath() const;
    const std::vector<Vector2>& GetReservedPath() const;

    void PlanAction(ActionType actionType, const std::vector<Vector2>& movePath, const Vector2& destination,
                    const std::vector<Vector2>& warningPositions);
    void ClearPlannedAction();

    ActionType GetPlannedActionType() const;
    const Vector2& GetPlannedDestination() const;
    const std::vector<Vector2>& GetAttackWarningPositions() const;

    void TakeDamage(int damage);

    int GetHealth() const;
    bool IsDead() const;

  private:
    int health = MaxHealth;

    std::vector<Vector2> path;
    std::vector<Vector2> reservedPath;
    std::vector<Vector2> attackWarningPositions;

    ActionType plannedActionType = ActionType::None;
    Vector2 plannedDestination = Vector2::Zero;

    int pathIndex = 0;

    float moveTimer = 0.0f;
    float moveInterval = 0.06f;
};
