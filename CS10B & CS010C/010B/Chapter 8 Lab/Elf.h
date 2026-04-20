#ifndef ELF_H
#define ELF_H
#include <iostream>
#include "Character.h"
using namespace std;

class Elf : public Character{
 private:
    string famName;
 public:
    Elf(const string &name, double health, double attackStrength, string fam);
    void attack(Character &competitor);

};

#endif