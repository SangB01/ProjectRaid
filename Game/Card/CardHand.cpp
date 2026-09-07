#include "CardHand.h"
#include <algorithm>
#include <Util/Util.h>

void CardHand::DrawCards(int count)
{
    int totalWeight = 0;
    for (int type = 0; type < static_cast<int>(CardType::Count); ++type)
    {
        totalWeight += Card{0, static_cast<CardType>(type)}.GetDefinition().drawWeight;
    }

    for (int index = 0; index < count; ++index)
    {
        int roll = Util::RandomRange(1, totalWeight);
        for (int type = 0; type < static_cast<int>(CardType::Count); ++type)
        {
            const CardType cardType = static_cast<CardType>(type);
            roll -= Card{0, cardType}.GetDefinition().drawWeight;
            if (roll <= 0)
            {
                AddCard(cardType);
                break;
            }
        }
    }
}

void CardHand::AddCard(CardType type)
{
    if (type < CardType::Fireball || type >= CardType::Count)
    {
        return;
    }
    if (static_cast<int>(cards.size()) >= MaxCards)
    {
        cards.erase(cards.begin());
    }
    cards.push_back({nextCardId++, type});
}

bool CardHand::RemoveCard(int cardId)
{
    const auto card = std::find_if(cards.begin(), cards.end(), [cardId](const Card& entry) {
        return entry.id == cardId;
    });

    if (card == cards.end())
    {
        return false;
    }

    cards.erase(card);
    return true;
}

const Card* CardHand::FindCard(int cardId) const
{
    for (const Card& card : cards)
    {
        if (card.id == cardId)
        {
            return &card;
        }
    }

    return nullptr;
}

const std::vector<Card>& CardHand::GetCards() const
{
    return cards;
}
