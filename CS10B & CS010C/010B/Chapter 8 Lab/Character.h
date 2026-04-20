#include <string>

using namespace std;

#ifndef __CHARACTER_H__
#define __CHARACTER_H__

enum HeroType {WARRIOR, ELF, WIZARD};

const double MAX_HEALTH = 100.0;

class Character {
 protected:
	HeroType type;
	string name;#ifndef CHARACTER_H
#define CHARACTER_H

#include <iostream>
using namespace std;

enum HeroType {WARRIOR, ELF, WIZARD};

const double MAX_HEALTH = 100.0;

class Character {

 protected:
    HeroType type;
    string name;
    double health;
    double attackStrength;

 public:
   Character(HeroType type, const string &name, double health, double attackStrength);
   HeroType getType() const;
   const string & getName() const;
   int getHealth() const;
   void damage(double d);
   bool isAlive() const;
   virtual void attack(Character &competitor) = 0;
};

#endif
#endif