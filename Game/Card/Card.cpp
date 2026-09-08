#include "Card.h"
#include <cassert>

const CardDefinition& Card::GetDefinition() const
{
    static constexpr CardDefinition definitions[] = {
        {L"Fireball", CardTargetType::Enemy, 8, 0, L"Damage 8", CardRarity::Common, 3},
        {L"Barrier", CardTargetType::LivingPlayer, 0, 1, L"Block 1 / 1T", CardRarity::Common, 2},
        {L"Teleport", CardTargetType::AllyDestination, 0, 0, L"Near ally", CardRarity::Rare, 2},
        {L"Medkit", CardTargetType::LivingPlayer, 0, 3, L"Heal 3", CardRarity::Common, 2},
        {L"Attack", CardTargetType::Enemy, 5, 0, L"Area damage 5", CardRarity::Common, 6},
        {L"Revive", CardTargetType::DeadPlayer, 0, 5, L"Revive HP 5", CardRarity::Epic, 1},
        {L"Turret", CardTargetType::AdjacentCell, 5, 3, L"Damage 5 / 3T", CardRarity::Rare, 2},
        {L"Barricade", CardTargetType::AdjacentCell, 0, 2, L"Cover / 2T", CardRarity::Common, 2}
    };
    static_assert(sizeof(definitions) / sizeof(definitions[0]) == static_cast<int>(CardType::Count));
    static_assert([] {
        int total = 0;
        for (const auto& definition : definitions)
        {
            total += definition.copiesPerDeck;
        }
        return total == 20;
    }(), "Card copies must fill the 20-card deck.");
    const int index = static_cast<int>(type);
    assert(index >= 0 && index < static_cast<int>(CardType::Count));
    return definitions[index];
}
