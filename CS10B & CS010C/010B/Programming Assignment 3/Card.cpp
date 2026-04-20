#include "Card.h"
#include <cctype>
#include <iostream>

using namespace std;

// Default constructor
Card::Card() {
    suit = 'c'; // Default to Clubs
    rank = 2;   // Default to rank 2
}

// Constructor with specified suit and rank
Card::Card(char s, int r) {
    s = tolower(s); // Convert suit to lowercase
    if (s == 'c' || s == 'd' || s == 'h' || s == 's') {
        suit = s;
    } else {
        suit = 'c'; // Default to Clubs if invalid suit
    }

    if (r >= 1 && r <= 13) {
        rank = r;
    } else {
        rank = 2; // Default to rank 2 if invalid rank
    }
}

// Get suit of the card
char Card::getSuit() const {
    return suit;
}

// Get rank of the card
int Card::getRank() const {
    return rank;
}

// Output the card in the format "Rank of Suit"
ostream & operator<<(ostream &out, const Card &card) {
    string ranks[] = {"Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
    string suits[] = {"Clubs", "Diamonds", "Hearts", "Spades"};

    out << ranks[card.getRank() - 1] << " of ";

    char suit = card.getSuit();
    if (suit == 'c') {
        out << suits[0]; // Clubs
    } else if (suit == 'd') {
        out << suits[1]; // Diamonds
    } else if (suit == 'h') {
        out << suits[2]; // Hearts
    } else if (suit == 's') {
        out << suits[3]; // Spades
    } else {
        out << "Unknown"; // Default case for unknown suit
    }

    return out;
}

