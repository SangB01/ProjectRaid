#pragma once
#include <Actor/Actor.h>

using namespace Craft;

// 터렛 및 바리케이트를 생성하는 클래스
class Deployable : public Actor
{
    TYPE_DECLARATIONS(Deployable, Actor)

  public:
    enum class Kind { Turret, Barricade };
    static constexpr int MaxTurrets = 3;
    static constexpr int TurretDamage = 5;

    Deployable(Kind kind, const Vector2& position);
    Kind GetKind() const;
    int GetRemainingTurns() const;
    void CompleteTurn();

  private:
    Kind kind;
    int remainingTurns;
};
