#include <iostream>
#include "Elf.h"

using namespace std;


Elf::Elf(const string &name, double health, double attackStrength, string fam):Character(ELF, name, health, attackStrength), famName(fam){}

void Elf::attack(Character &competitor){
    if(competitor.getType() == ELF){
        Elf &comp = dynamic_cast<Elf&>(competitor);

        if(comp.famName == this->famName){
            cout << "Elf " << this->getName() << " does not attack Elf " << comp.getName() << "." << endl;
            cout << "They are both members of the " << this->famName << " family." << endl;
        }

        else{
            double dmg = (this->health / MAX_HEALTH)*this->attackStrength;
            competitor.damage(dmg);
            cout << "Elf " << this->name << " shoots an arrow at " << competitor.getName() << " --- TWANG!!" << endl;
            cout << competitor.getName() << " takes " << dmg << " damage." << endl;
        }
    }
    else{
        double dmg = (this->health / MAX_HEALTH)*this->attackStrength;
        competitor.damage(dmg);
        cout << "Elf " << this->name << " shoots an arrow at " << competitor.getName() << " --- TWANG!!" << endl;
        cout << competitor.getName() << " takes " << dmg << " damage." << endl;
    }

}

