#include <iostream>
#include "Warrior.h"

using namespace std;

Warrior::Warrior(const string &name, double health, double attackStrength, string inp):Character(WARRIOR, name, health, attackStrength), allegiance(inp){}

void Warrior::attack(Character &competitor){
    if(competitor.getType() == WARRIOR){
        Warrior &comp = dynamic_cast<Warrior&>(competitor);

        if(comp.allegiance == this->allegiance){
            cout << "Warrior " << this->getName() << " does not attack Warrior " << comp.getName() << "." << endl;
            cout << "They share an allegiance with " << this->allegiance << "." << endl;
        }

        else{
            double dmg = (this->health / MAX_HEALTH)*this->attackStrength;
            competitor.damage(dmg);
            cout << "Warrior " << this->name << " attacks " << competitor.getName() << " --- SLASH!!" << endl;
            cout << competitor.getName() << " takes " << dmg << " damage." << endl;
        }
    }
    else{
        double dmg = (this->health / MAX_HEALTH)*this->attackStrength;
        competitor.damage(dmg);
        cout << "Warrior " << this->name << " attacks " << competitor.getName() << " --- SLASH!!" << endl;
        cout << competitor.getName() << " takes " << dmg << " damage." << endl;
    }

}