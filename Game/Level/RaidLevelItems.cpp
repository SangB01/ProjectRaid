#include "RaidLevel.h"

#include <algorithm>
#include <cstdlib>

bool RaidLevel::IsAdjacent(const Vector2& first, const Vector2& second) const
{
    return IsWithinRange(first, second, 1);
}

bool RaidLevel::IsWithinRange(const Vector2& first, const Vector2& second, int range) const
{
    return range > 0 && first != second && std::abs(first.x - second.x) <= range &&
           std::abs(first.y - second.y) <= range;
}

Vector2 RaidLevel::GetPlannedPlayerPosition(const Player& player) const
{
    return player.HasReservedPath() ? player.GetReservedPath().back() : player.GetPosition();
}

bool RaidLevel::IsOnReservedRoute(const Vector2& position, const Player* ignorePlayer) const
{
    const auto touchesPath = [&position](Vector2 previous, const std::vector<Vector2>& path) {
        for (const Vector2& next : path)
        {
            if (position == next || (previous.x != next.x && previous.y != next.y &&
                (position == Vector2(previous.x, next.y) || position == Vector2(next.x, previous.y))))
            {
                return true;
            }
            previous = next;
        }
        return false;
    };

    for (const auto& player : players)
    {
        if (!player || !player->IsActive() || player.get() == ignorePlayer)
        {
            continue;
        }
        if (touchesPath(player->GetPosition(), player->GetReservedPath()) ||
            (player->GetReservedCardPosition() && *player->GetReservedCardPosition() == position))
        {
            return true;
        }
    }
    if (boss && boss->IsActive())
    {
        if (touchesPath(boss->GetPosition(), boss->GetReservedPath()) ||
            (boss->GetPlannedActionType() != Boss::ActionType::None && boss->GetPlannedDestination() == position) ||
            (boss->GetPlannedActionType() == Boss::ActionType::SummonMinions &&
             ContainsPosition(boss->GetAttackWarningPositions(), position)))
        {
            return true;
        }
    }
    for (const auto& minion : minions)
    {
        if (minion && minion->IsActive() &&
            (touchesPath(minion->GetPosition(), minion->GetReservedPath()) ||
             ContainsPosition(minion->GetWarningPositions(), position)))
        {
            return true;
        }
    }
    return false;
}

bool RaidLevel::CanPlaceCardAt(const Vector2& position, const Player& caster) const
{
    return raidMap && raidMap->IsWalkable(position) && !IsOccupiedByActor(position) &&
           !IsOnReservedRoute(position, &caster);
}

int RaidLevel::GetTurretCount(const Player* ignoreReservationOwner, bool includeReservations) const
{
    int count = 0;
    for (const auto& object : deployables)
    {
        if (object && object->IsActive() && object->GetKind() == Deployable::Kind::Turret)
        {
            ++count;
        }
    }
    if (includeReservations)
    {
        for (const auto& player : players)
        {
            if (!player || !player->IsActive() || player.get() == ignoreReservationOwner)
            {
                continue;
            }
            const Card* card = cardHand.FindCard(player->GetReservedCardId());
            if (card && card->type == CardType::Turret)
            {
                ++count;
            }
        }
    }
    return count;
}

bool RaidLevel::CanUseCard(const Card& card, const Player& caster, const std::shared_ptr<Actor>& target,
                           const std::optional<Vector2>& position, bool planning) const
{
    if (!caster.IsActive() || !IsValidCardTarget(card, target))
    {
        return false;
    }
    const auto targetPlayer = std::dynamic_pointer_cast<Player>(target);
    switch (card.type)
    {
    case CardType::Fireball:
        return !position && HasLineOfSight(caster.GetPosition(), target->GetPosition());
    case CardType::MeleeAttack:
        return !position && IsWithinRange(caster.GetPosition(), target->GetPosition(), MeleeAttackRange) &&
               HasLineOfSight(caster.GetPosition(), target->GetPosition());
    case CardType::Barrier:
        return !position && targetPlayer && !targetPlayer->HasBarrier();
    case CardType::Medkit:
        return !position && targetPlayer && targetPlayer->GetHealth() < Player::MaxHealth;
    case CardType::Teleport:
        return position && targetPlayer && targetPlayer.get() != &caster &&
               IsAdjacent(*position, planning ? GetPlannedPlayerPosition(*targetPlayer) : targetPlayer->GetPosition()) &&
               CanPlaceCardAt(*position, caster);
    case CardType::Revive:
        return position && targetPlayer &&
               (*position == targetPlayer->GetPosition() || IsAdjacent(*position, targetPlayer->GetPosition())) &&
               CanPlaceCardAt(*position, caster);
    case CardType::Turret:
    case CardType::Barricade:
        return targetPlayer.get() == &caster && position && IsAdjacent(caster.GetPosition(), *position) &&
               CanPlaceCardAt(*position, caster) &&
               (card.type != CardType::Turret || GetTurretCount(&caster, planning) < Deployable::MaxTurrets);
    default:
        return false;
    }
}

bool RaidLevel::ApplyCardEffect(const Card& card, Player& caster, const std::shared_ptr<Actor>& target,
                                const std::optional<Vector2>& position)
{
    if (!CanUseCard(card, caster, target, position, false))
    {
        return false;
    }
    const auto targetPlayer = std::dynamic_pointer_cast<Player>(target);
    const auto damageEnemy = [](const std::shared_ptr<Actor>& enemy, int damage) {
        if (auto targetBoss = std::dynamic_pointer_cast<Boss>(enemy))
        {
            targetBoss->TakeDamage(damage);
        }
        else if (auto minion = std::dynamic_pointer_cast<Minion>(enemy))
        {
            minion->TakeDamage(damage);
        }
    };
    switch (card.type)
    {
    case CardType::Fireball:
        damageEnemy(target, card.GetDefinition().damage);
        break;
    case CardType::MeleeAttack:
        if (boss && boss->IsActive() &&
            IsWithinRange(caster.GetPosition(), boss->GetPosition(), MeleeAttackRange) &&
            HasLineOfSight(caster.GetPosition(), boss->GetPosition()))
        {
            damageEnemy(boss, card.GetDefinition().damage);
        }
        for (const auto& minion : minions)
        {
            if (minion && minion->IsActive() &&
                IsWithinRange(caster.GetPosition(), minion->GetPosition(), MeleeAttackRange) &&
                HasLineOfSight(caster.GetPosition(), minion->GetPosition()))
            {
                damageEnemy(minion, card.GetDefinition().damage);
            }
        }
        break;
    case CardType::Barrier:
        targetPlayer->ApplyBarrier();
        break;
    case CardType::Medkit:
        targetPlayer->Heal(card.GetDefinition().effectValue);
        break;
    case CardType::Teleport:
        caster.SetPosition(*position);
        caster.SavePrevioussState();
        break;
    case CardType::Revive:
        return targetPlayer->Revive(*position, card.GetDefinition().effectValue);
    case CardType::Turret:
    case CardType::Barricade:
        deployables.emplace_back(SpawnActor<Deployable>(card.type == CardType::Turret ? Deployable::Kind::Turret :
                                                        Deployable::Kind::Barricade, *position));
        SyncTemporaryObstacles();
        break;
    default:
        return false;
    }
    return true;
}

void RaidLevel::SyncTemporaryObstacles()
{
    std::vector<Vector2> obstacles;
    for (const auto& object : deployables)
    {
        if (object && object->IsActive() && object->GetKind() == Deployable::Kind::Barricade)
        {
            obstacles.push_back(object->GetPosition());
        }
    }
    if (raidMap)
    {
        raidMap->SetTemporaryObstacles(obstacles);
    }
}

void RaidLevel::ExecuteTurretAttacks()
{
    if (hasTurretsAttacked)
    {
        return;
    }
    hasTurretsAttacked = true;
    for (const auto& object : deployables)
    {
        if (object && object->IsActive() && object->GetKind() == Deployable::Kind::Turret &&
            boss && boss->IsActive() && HasLineOfSight(object->GetPosition(), boss->GetPosition()))
        {
            boss->TakeDamage(Deployable::TurretDamage);
        }
    }
}

void RaidLevel::CompleteTurnEffects()
{
    for (const auto& player : players)
    {
        if (player)
        {
            player->ClearBarrier();
        }
    }
    for (const auto& object : deployables)
    {
        if (object)
        {
            object->CompleteTurn();
        }
    }
    deployables.erase(std::remove_if(deployables.begin(), deployables.end(), [](const auto& object) {
        return !object || !object->IsActive();
    }), deployables.end());
    SyncTemporaryObstacles();
}

std::shared_ptr<Player> RaidLevel::FindPlayerChoiceAt(const Vector2& position) const
{
    if (auto player = FindPlayerAt(position))
    {
        return player;
    }
    if (!raidMap)
    {
        return nullptr;
    }
    const int interfaceX = raidMap->GetPosition().x + raidMap->GetWidth() + 3;
    for (int index = 0; index < static_cast<int>(players.size()); ++index)
    {
        const int top = raidMap->GetPosition().y + 10 + index * 4;
        if (position.x >= interfaceX + 3 && position.x < interfaceX + 27 && position.y >= top && position.y < top + 4)
        {
            return players[index];
        }
    }
    return nullptr;
}

void RaidLevel::ResetCardTargeting()
{
    isSelectingCardTarget = false;
    pendingCardTarget.reset();
}

std::wstring RaidLevel::GetCardTargetLabel(const Player& player) const
{
    if (player.GetReservedCardPosition())
    {
        const Vector2 point = *player.GetReservedCardPosition();
        return L"(" + std::to_wstring(point.x) + L"," + std::to_wstring(point.y) + L")";
    }
    const auto target = player.GetReservedCardTarget();
    if (target == boss)
    {
        return L"Boss";
    }
    if (auto targetPlayer = std::dynamic_pointer_cast<Player>(target))
    {
        return L"P" + std::to_wstring(targetPlayer->GetPlayerIndex());
    }
    return L"Minion";
}
