#include "Deployable.h"

Deployable::Deployable(Kind kind, const Vector2& position)
    : Actor(kind == Kind::Turret ? L"T" : L"#", position, kind == Kind::Turret ? Color::Yellow : Color::Cyan),
      kind(kind), remainingTurns(kind == Kind::Turret ? 3 : 2)
{
    sortingOrder = 20;
}

Deployable::Kind Deployable::GetKind() const
{
    return kind;
}

int Deployable::GetRemainingTurns() const
{
    return remainingTurns;
}

void Deployable::CompleteTurn()
{
    if (IsActive() && --remainingTurns <= 0)
    {
        Destroy();
    }
}
