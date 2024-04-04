// Cambio.h
#ifndef CAMBIO_H
#define CAMBIO_H

#include <vector>
#include <map>

// Card Definitions
enum class CardValue { A = 1, Two = 2, J = 11, Q = 12, Red_K = -1, Black_K = 13, Joker = 0 };
enum class CardType { Hearts, Diamonds, Clubs, Spades, Joker };

struct Card {
    CardValue value;
    CardType type;
    int getPoints() const;
};

class Player {
public:
    std::vector<Card> hand; // Cards in the player's hand
    bool isCambioCalled = false;
    void drawCard(std::vector<Card>& deck);
    // Other player methods
};

class Game {
public:
    std::vector<Card> deck;
    std::vector<Card> discardPile;
    std::vector<Player> players;
    void initializeDeck();
    void dealInitialCards();
    void startGame();
    // Other game methods
};

#endif // CAMBIO_H
