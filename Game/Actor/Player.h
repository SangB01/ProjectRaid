#pragma once

#include <Actor/Actor.h>
#include <Math/Vector2.h>

#include <vector>
#include <optional>

using namespace Craft;

class Player : public Actor
{
    TYPE_DECLARATIONS(Player, Actor)

  public:
    static constexpr int MaxHealth = 10;

    static constexpr int MaxMoveDistance = 40;

  public:
    Player(int playerIndex, const Vector2& startPosition = Vector2::Zero);

    virtual ~Player() = default;

    virtual void Tick(float deltaTime) override;

    void SetPath(const std::vector<Vector2>& newPath);

    void ClearPath();

    bool HasPath() const;

    void ReservePath(const std::vector<Vector2>& newPath);
    void ClearReservedPath();
    void ExecuteReservedPath();

    bool HasReservedPath() const;
    const std::vector<Vector2>& GetReservedPath() const;

    bool ReserveCard(int cardId, const std::shared_ptr<Actor>& target);
    bool ReserveCardAt(int cardId, const std::shared_ptr<Actor>& target, const Vector2& position);
    void ClearReservedCard();
    bool HasReservedCard() const;
    int GetReservedCardId() const;
    std::shared_ptr<Actor> GetReservedCardTarget() const;
    const std::optional<Vector2>& GetReservedCardPosition() const;

    int GetHealth() const;

    void TakeDamage(int damage);
    bool IsDead() const;
    void Heal(int amount);
    void ApplyBarrier();
    void ClearBarrier();
    bool HasBarrier() const;
    bool Revive(const Vector2& position, int restoredHealth = 5);

    void SetSelected(bool selected);

    bool IsSelected() const;

    int GetPlayerIndex() const;

  private:
    int health = MaxHealth;
    bool hasBarrier = false;

    // A*가 계산한 경로.
    std::vector<Vector2> path;

    // 턴 종료 버튼을 누를 때 실행할 예약 경로.
    std::vector<Vector2> reservedPath;

    int reservedCardId = 0;
    std::weak_ptr<Actor> reservedCardTarget;
    std::optional<Vector2> reservedCardPosition;

    // 현재 이동할 경로 인덱스.
    int pathIndex = 0;

    // 이동 연출용 시간.
    float moveTimer = 0.0f;

    float moveInterval = 0.12f;

    int playerIndex = 0;

    bool isSelected = false;
};
