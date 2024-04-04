#include "Deck.h"
#include <algorithm> // For random_shuffle
#include <ctime>     // For time, used in random_shuffle
#include <cstdlib>   // For rand and srand

using namespace std;

// Constructor for Deck
Deck::Deck() {
    char suits[] = {'c', 'd', 'h', 's'};
    for (int i = 0; i < 4; ++i) {
        for (int rank = 1; rank <= 13; ++rank) {
            theDeck.push_back(Card(suits[i], rank));
        }
    }
}

// Deals the top card from the deck
Card Deck::dealCard() {
    if (!theDeck.empty()) {
        Card dealt = theDeck.back();
        theDeck.pop_back();
        dealtCards.push_back(dealt);
        return dealt;
    }
    return Card(); 
}


// Shuffles the deck
void Deck::shuffleDeck() {
    // Put all dealt cards back into the deck
    for (unsigned i = 0; i < dealtCards.size(); ++i) {
        const Card &card = dealtCards[i];
        theDeck.push_back(card);
    }
    
    // Clear the dealt cards
    dealtCards.clear();

    random_shuffle(theDeck.begin(), theDeck.end());
}
