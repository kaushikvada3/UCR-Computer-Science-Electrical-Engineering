#include <iostream>
#include "Wizard.h"

using namespace std;


Wizard::Wizard(const string &name, double health, double attackStrength, int r):Character(WIZARD, name, health, attackStrength), rank(r){}

void Wizard::attack(Character &competitor){
    if(competitor.getType() == WIZARD){
        Wizard &comp = dynamic_cast<Wizard&>(competitor);
        double dmg = this->attackStrength*(static_cast<double>(this->rank) / static_cast<double>(comp.rank));
        competitor.damage(dmg);
        cout << "Wizard " << this->name << " attacks " << competitor.getName() << " --- POOF!!" << endl;
        cout << competitor.getName() << " takes " << dmg << " damage." << endl;
    }
    else{
        competitor.damage(this->attackStrength);
        cout << "Wizard " << this->name << " attacks " << competitor.getName() << " --- POOF!!" << endl;
        cout << competitor.getName() << " takes " << this->attackStrength << " damage." << endl;
    }

}