#ifndef WIZARD_H
#define WIZARD_H
#include <iostream>
#include "Character.h"
using namespace std;

class Wizard : public Character{
 private:
    int rank;
 public:
    Wizard(const string &name, double health, double attackStrength, int r);
    void attack(Character &competitor);

};

#endif