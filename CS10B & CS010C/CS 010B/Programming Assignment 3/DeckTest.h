#ifndef __DECK_H__
#define __DECK_H__

#include <vector>
#include "Card.h"

using namespace std;

class Deck {
 private:
    vector<Card> theDeck;     // Vector to store the main deck of cards
    vector<Card> dealtCards;  // Vector to store the cards that have been dealt

 public:
    Deck();  // Constructor to create a deck of 52 cards

    Card dealCard();  // Deals the top card of the deck

    void shuffleDeck();  // Shuffles the deck into random order

    unsigned deckSize() const;  // Returns the size of the Deck
};

#endif // __DECK_H__
