#pragma once

#include "Card.h"
#include <vector>

class CardHand
{
  public:
    static constexpr int MaxCards = 8;
    static constexpr int CardsPerTurn = 3;

    void DrawCards(int count = CardsPerTurn);
    void AddCard(CardType type);
    bool RemoveCard(int cardId);
    const Card* FindCard(int cardId) const;
    const std::vector<Card>& GetCards() const;

  private:
    std::vector<Card> cards;
    int nextCardId = 1;
};
