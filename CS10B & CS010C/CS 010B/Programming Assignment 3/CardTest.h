#ifndef __CARD_H__
#define __CARD_H__

#include <iostream>
#include <string>

using namespace std;

class Card {
 private:
    char suit; // c = Clubs, d = Diamonds, h = Hearts, s = Spades
    int rank;  // 1 - 13 (1 = Ace, 11 = Jack, 12 = Queen, 13 = King)

 public:
    Card();  // Assigns a default value of 2 of Clubs
    Card(char s, int r);  // Assigns the Card the suit and rank provided

    char getSuit() const;  // Returns the Card's suit
    int getRank() const;   // Returns the Card's rank

    // Overload << operator to output a Card in the format "Rank of Suit"
    friend ostream &operator<<(ostream &out, const Card &card);
};

#endif // __CARD_H__
