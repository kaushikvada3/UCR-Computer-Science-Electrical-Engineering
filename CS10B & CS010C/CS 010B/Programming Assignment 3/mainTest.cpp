#include "Deck.h"
#include "Card.h"
#include <vector>
#include <iostream>
#include <fstream>

// Function to check for a pair in a hand of cards
bool hasPair(const vector<Card> &hand) {
    for (size_t i = 0; i < hand.size(); ++i) {
        for (size_t j = i + 1; j < hand.size(); ++j) {
            if (hand[i].getRank() == hand[j].getRank()) {
                return true;
            }
        }
    }
    return false;
}

// Overloading the << operator for a vector of Cards
ostream &operator<<(ostream &out, const vector<Card> &hand) {
    for (size_t i = 0; i < hand.size(); ++i) {
        out << hand[i];
        if (i < hand.size() - 1) out << ", ";
    }
    return out;
}

int main() {
    srand(2222);  // Seed the random number generator

    // User inputs
    string outputFile;
    cout << "Do you want to output all hands to a file?(Yes/No) ";
    cin >> outputFile;
    bool toFile = (outputFile == "Yes" || outputFile == "yes");

    ofstream fileStream;
    if (toFile) {
        cout << "Enter name of output file: ";
        cin >> outputFile;
        fileStream.open(outputFile);
    }

    int cardsPerHand, numDeals;
    cout << "Enter number of cards per hand: ";
    cin >> cardsPerHand;
    cout << "Enter number of deals (simulations): ";
    cin >> numDeals;

    Deck deck;
    int pairCount = 0;
    for (int i = 0; i < numDeals; ++i) {
        deck.shuffleDeck();
        vector<Card> hand;
        for (int j = 0; j < cardsPerHand; ++j) {
            hand.push_back(deck.dealCard());
        }

        bool pair = hasPair(hand);
        if (pair) ++pairCount;

        if (toFile) {
            if (pair) fileStream << "Found Pair!! ";
            fileStream << hand << endl;
        }
    }

    double pairProbability = (double)pairCount / numDeals * 100;
    cout << "Chances of receiving a pair in a hand of " << cardsPerHand << " cards is: " << pairProbability << "%" << endl;


    if (toFile) {
        fileStream.close();
    }

    return 0;
}
