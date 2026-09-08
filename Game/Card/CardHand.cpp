#include "CardHand.h"
#include <algorithm>
#include <Util/Util.h>

void CardHand::DrawCards(int count)
{
    for (int index = 0; index < count; ++index)
    {
        if (drawPile.empty())
        {
            RefillDeck();
        }

        AddCard(drawPile.back());
        drawPile.pop_back();
    }
}

void CardHand::RefillDeck()
{
    drawPile.clear();
    drawPile.reserve(DeckSize);

    for (int type = 0; type < static_cast<int>(CardType::Count); ++type)
    {
        const CardType cardType = static_cast<CardType>(type);
        const int copies = Card{0, cardType}.GetDefinition().copiesPerDeck;

        for (int copy = 0; copy < copies; ++copy)
        {
            drawPile.emplace_back(cardType);
        }
    }

    std::shuffle(drawPile.begin(), drawPile.end(), Util::GetRandomEngine());
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

int CardHand::GetRemainingDeckCount() const
{
    return static_cast<int>(drawPile.size());
}
