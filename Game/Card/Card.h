#pragma once

enum class CardType
{
    Fireball,
    Barrier,
    Teleport,
    Medkit,
    MeleeAttack,
    Revive,
    Turret,
    Barricade,
    Count
};

enum class CardTargetType
{
    Enemy,
    LivingPlayer,
    AllyDestination,
    DeadPlayer,
    AdjacentCell
};

enum class CardRarity
{
    Common,
    Rare,
    Epic
};

struct CardDefinition
{
    const wchar_t* name;
    CardTargetType targetType;
    int damage;
    int effectValue;
    const wchar_t* description;
    CardRarity rarity;
    int copiesPerDeck;
};

struct Card
{
    int id = 0;
    CardType type = CardType::Fireball;

    const CardDefinition& GetDefinition() const;
};
