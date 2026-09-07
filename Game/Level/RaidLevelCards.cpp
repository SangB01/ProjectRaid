#include "RaidLevel.h"

#include <Input/Input.h>
#include <Render/Renderer.h>
#include <algorithm>
#include <cmath>

void RaidLevel::ClearMovementPreview()
{
    previewPath.clear();
    hasPreviewTarget = false;
}

std::shared_ptr<Player> RaidLevel::FindCardOwner(int cardId) const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (player && player->IsActive() && player->GetReservedCardId() == cardId)
        {
            return player;
        }
    }

    return nullptr;
}

std::shared_ptr<Actor> RaidLevel::FindEnemyAt(const Vector2& position) const
{
    if (boss && boss->IsActive() && boss->GetPosition() == position)
    {
        return boss;
    }

    for (const std::shared_ptr<Minion>& minion : minions)
    {
        if (minion && minion->IsActive() && minion->GetPosition() == position)
        {
            return minion;
        }
    }

    return nullptr;
}

bool RaidLevel::IsValidCardTarget(const Card& card, const std::shared_ptr<Actor>& target) const
{
    if (!target)
    {
        return false;
    }

    if (card.GetDefinition().targetType == CardTargetType::Enemy)
    {
        return target->IsActive() && (target == boss ||
            std::any_of(minions.begin(), minions.end(), [&target](const auto& minion) { return minion == target; }));
    }

    return std::any_of(players.begin(), players.end(), [&target, &card](const auto& player) {
        return player && player == target &&
               (card.GetDefinition().targetType == CardTargetType::DeadPlayer ? player->IsDead() : player->IsActive());
    });
}

bool RaidLevel::TryReserveSelectedCard(const std::shared_ptr<Actor>& target, const std::optional<Vector2>& position)
{
    const std::vector<Card>& cards = cardHand.GetCards();

    if (turnState != TurnState::PlayerPlanning || !selectedPlayer || !selectedPlayer->IsActive() ||
        selectedCardIndex < 0 || selectedCardIndex >= static_cast<int>(cards.size()))
    {
        return false;
    }

    const Card& card = cards[selectedCardIndex];
    const std::shared_ptr<Player> owner = FindCardOwner(card.id);

    if (owner && owner != selectedPlayer)
    {
        return false;
    }

    if (!CanUseCard(card, *selectedPlayer, target, position, true))
    {
        return false;
    }

    if (!(position ? selectedPlayer->ReserveCardAt(card.id, target, *position) : selectedPlayer->ReserveCard(card.id, target)))
    {
        return false;
    }

    ResetCardTargeting();
    ClearMovementPreview();
    return true;
}

void RaidLevel::HandleCardTargetClick(const Vector2& position)
{
    const auto& cards = cardHand.GetCards();
    if (!isSelectingCardTarget || !selectedPlayer || !selectedPlayer->IsActive() ||
        selectedCardIndex < 0 || selectedCardIndex >= static_cast<int>(cards.size()))
    {
        return;
    }

    const Card& card = cards[selectedCardIndex];
    switch (card.GetDefinition().targetType)
    {
    case CardTargetType::Enemy:
        TryReserveSelectedCard(FindEnemyAt(position));
        break;
    case CardTargetType::LivingPlayer:
        TryReserveSelectedCard(FindPlayerChoiceAt(position));
        break;
    case CardTargetType::AdjacentCell:
        TryReserveSelectedCard(selectedPlayer, position);
        break;
    case CardTargetType::AllyDestination:
    case CardTargetType::DeadPlayer:
    {
        const auto clickedPlayer = FindPlayerChoiceAt(position);
        if (IsValidCardTarget(card, clickedPlayer) && clickedPlayer != selectedPlayer)
        {
            pendingCardTarget = clickedPlayer;
        }
        else if (auto target = pendingCardTarget.lock())
        {
            TryReserveSelectedCard(target, position);
        }
        break;
    }
    }
}

Vector2 RaidLevel::GetCardSlotPosition(int index) const
{
    return raidMap->GetPosition() + Vector2(8 + index * CardSlotSpacing, raidMap->GetHeight() + 6);
}

int RaidLevel::FindCardSlotAt(const Vector2& position) const
{
    if (!raidMap)
    {
        return -1;
    }

    for (int index = 0; index < static_cast<int>(cardHand.GetCards().size()); ++index)
    {
        const Vector2 slot = GetCardSlotPosition(index);

        if (position.x >= slot.x && position.x < slot.x + CardSlotWidth &&
            position.y >= slot.y && position.y < slot.y + CardSlotHeight)
        {
            return index;
        }
    }

    return -1;
}

bool RaidLevel::ProcessCardInput()
{
    const Input& input = Input::Get();
    const int cardCount = static_cast<int>(cardHand.GetCards().size());

    if (input.GetKeyDown(VK_LBUTTON))
    {
        const Vector2 mousePosition = input.GetMousePosition();
        const int clickedSlot = FindCardSlotAt(mousePosition);

        if (clickedSlot >= 0)
        {
            selectedCardIndex = clickedSlot;
            ResetCardTargeting();
            ClearMovementPreview();
            return true;
        }

        if (isSelectingCardTarget)
        {
            HandleCardTargetClick(mousePosition);
            return true;
        }

        const auto clickedPlayer = FindPlayerChoiceAt(mousePosition);
        if (clickedPlayer && clickedPlayer->IsActive())
        {
            SelectPlayer(clickedPlayer);
            return true;
        }
    }

    if (input.GetKeyDown(VK_LEFT) || input.GetKeyDown(VK_RIGHT))
    {
        if (cardCount > 0)
        {
            const int direction = input.GetKeyDown(VK_RIGHT) ? 1 : -1;
            selectedCardIndex = (selectedCardIndex + direction + cardCount) % cardCount;
        }

        ResetCardTargeting();
        ClearMovementPreview();
        return true;
    }

    if (input.GetKeyDown(VK_SPACE))
    {
        if (isSelectingCardTarget)
        {
            ResetCardTargeting();
        }
        else if (selectedPlayer && selectedPlayer->IsActive() && cardCount > 0)
        {
            const Card& card = cardHand.GetCards()[selectedCardIndex];
            const std::shared_ptr<Player> owner = FindCardOwner(card.id);

            if (owner == selectedPlayer)
            {
                selectedPlayer->ClearReservedCard();
            }
            else if (!owner)
            {
                isSelectingCardTarget = true;
                pendingCardTarget.reset();
            }
        }

        ClearMovementPreview();
        return true;
    }

    return isSelectingCardTarget;
}

void RaidLevel::BeginPlayerCards()
{
    turnState = TurnState::PlayerCards;
    ResetCardTargeting();
    isCardInFlight = false;
    activeCardTarget.reset();
    activeCardCaster.reset();
    activeCardPosition.reset();
    nextCardPlayerIndex = 0;
    cardEffectTimer = 0.0f;
    ClearMovementPreview();

    if (turnEndButton)
    {
        turnEndButton->SetEnabled(false);
    }
}

void RaidLevel::ProcessPlayerCards(float deltaTime)
{
    if (isCardInFlight)
    {
        cardEffectTimer += deltaTime;

        if (cardEffectTimer < CardEffectDuration)
        {
            return;
        }

        isCardInFlight = false;
        const std::shared_ptr<Actor> target = activeCardTarget.lock();
        const auto caster = activeCardCaster.lock();

        if (caster && ApplyCardEffect(activeCard, *caster, target, activeCardPosition))
        {
            cardHand.RemoveCard(activeCard.id);
        }

        const int cardCount = static_cast<int>(cardHand.GetCards().size());
        selectedCardIndex = cardCount == 0 ? 0 : (std::min)(selectedCardIndex, cardCount - 1);
        activeCardTarget.reset();
        activeCardCaster.reset();
        activeCardPosition.reset();

        CheckBattleEnd();
        return;
    }

    while (nextCardPlayerIndex < static_cast<int>(players.size()))
    {
        const std::shared_ptr<Player>& player = players[nextCardPlayerIndex++];

        if (!player)
        {
            continue;
        }

        const Card* card = cardHand.FindCard(player->GetReservedCardId());
        const std::shared_ptr<Actor> target = player->GetReservedCardTarget();
        const auto position = player->GetReservedCardPosition();
        player->ClearReservedCard();

        if (!card || !CanUseCard(*card, *player, target, position, false))
        {
            continue;
        }

        activeCard = *card;
        activeCardTarget = target;
        activeCardCaster = player;
        activeCardPosition = position;
        cardEffectStart = player->GetPosition();
        cardEffectEnd = position.value_or(target->GetPosition());
        cardEffectTimer = 0.0f;
        isCardInFlight = true;
        return;
    }

    BeginBossTurn();
}

bool RaidLevel::CheckBattleEnd()
{
    if (turnState == TurnState::Victory || turnState == TurnState::Defeat)
    {
        return true;
    }

    if (boss && boss->IsDead())
    {
        turnState = TurnState::Victory;
    }
    else if (!players.empty() && std::none_of(players.begin(), players.end(), [](const std::shared_ptr<Player>& player) {
                 return player && player->IsActive();
             }))
    {
        turnState = TurnState::Defeat;
    }
    else
    {
        return false;
    }

    for (const std::shared_ptr<Player>& player : players)
    {
        if (player)
        {
            player->ClearPath();
            player->ClearReservedPath();
            player->ClearReservedCard();
        }
    }

    if (boss)
    {
        boss->ClearPath();
        boss->ClearPlannedAction();
    }

    for (const std::shared_ptr<Minion>& minion : minions)
    {
        if (minion)
        {
            minion->ClearCharge();
        }
    }

    ResetCardTargeting();
    isCardInFlight = false;
    activeCardTarget.reset();
    activeCardCaster.reset();
    activeCardPosition.reset();
    ClearMovementPreview();

    if (turnEndButton)
    {
        turnEndButton->SetEnabled(false);
    }

    if (onBattleEnded)
    {
        BattleResult result;
        result.outcome = turnState == TurnState::Victory ? BattleOutcome::Victory : BattleOutcome::Defeat;
        result.bossHealth = boss ? boss->GetHealth() : 0;
        result.elapsedTimeSeconds = elapsedTime;
        for (int index = 0; index < static_cast<int>(players.size()) && index < static_cast<int>(result.playerHealth.size()); ++index)
        {
            result.playerHealth[index] = players[index] ? players[index]->GetHealth() : 0;
        }
        onBattleEnded(result);
    }

    return true;
}

void RaidLevel::DrawCardTargets() const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (!player || !player->IsActive() || !player->HasReservedCard())
        {
            continue;
        }

        const std::shared_ptr<Actor> target = player->GetReservedCardTarget();

        if (player->GetReservedCardPosition())
        {
            Renderer::Get().Submit(L"X", *player->GetReservedCardPosition(), Color::Cyan, 25);
        }
        else if (target && target->IsActive())
        {
            const wchar_t* symbol = target == boss ? L"B" : (std::dynamic_pointer_cast<Player>(target) ? L"P" : L"S");
            Renderer::Get().Submit(symbol, target->GetPosition(), Color::Cyan, 25);
        }
    }

    const auto& cards = cardHand.GetCards();
    if (!isSelectingCardTarget || !selectedPlayer || selectedCardIndex < 0 || selectedCardIndex >= static_cast<int>(cards.size()))
    {
        return;
    }

    const Card& card = cards[selectedCardIndex];
    auto anchor = pendingCardTarget.lock();
    if (card.GetDefinition().targetType == CardTargetType::AdjacentCell)
    {
        anchor = selectedPlayer;
    }
    if (anchor)
    {
        const auto ally = std::dynamic_pointer_cast<Player>(anchor);
        const Vector2 center = card.type == CardType::Teleport && ally ? GetPlannedPlayerPosition(*ally) : anchor->GetPosition();
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const Vector2 point = center + Vector2(x, y);
                if (CanUseCard(card, *selectedPlayer, anchor, point, true))
                {
                    Renderer::Get().Submit(L"+", point, Color::Green, 30);
                }
            }
        }
    }
    else
    {
        const Vector2 mousePosition = Input::Get().GetMousePosition();
        const std::shared_ptr<Actor> target = card.GetDefinition().targetType == CardTargetType::Enemy ?
            FindEnemyAt(mousePosition) : FindPlayerChoiceAt(mousePosition);
        if (IsValidCardTarget(card, target) && target->IsActive())
        {
            const wchar_t* symbol = target == boss ? L"B" : (std::dynamic_pointer_cast<Player>(target) ? L"P" : L"S");
            const bool blocked = card.GetDefinition().targetType == CardTargetType::Enemy &&
                !CanUseCard(card, *selectedPlayer, target, std::nullopt, true);
            Renderer::Get().Submit(symbol, target->GetPosition(), blocked ? Color::Red : Color::Yellow, 30);
        }
    }
}

void RaidLevel::DrawCardEffect() const
{
    if (!isCardInFlight)
    {
        return;
    }

    const float progress = (std::min)(cardEffectTimer / CardEffectDuration, 1.0f);
    const Vector2 difference = cardEffectEnd - cardEffectStart;
    const Vector2 position = cardEffectStart + Vector2(static_cast<int>(std::lround(difference.x * progress)),
                                                       static_cast<int>(std::lround(difference.y * progress)));
    const wchar_t* symbol = activeCard.type == CardType::Medkit || activeCard.type == CardType::Revive ? L"+" :
        activeCard.type == CardType::Barrier ? L"O" : activeCard.type == CardType::Teleport ? L"@" :
        activeCard.type == CardType::Turret ? L"T" : activeCard.type == CardType::Barricade ? L"#" : L"*";
    Renderer::Get().Submit(symbol, position, Color::Yellow, 32);
}
