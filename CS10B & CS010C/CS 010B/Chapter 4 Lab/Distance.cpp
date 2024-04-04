#include <iostream>
#include "Distance.h"
using namespace std;

Distance::Distance(){
    this->_feet = 0;
    this->_inches = 0.0;
}
Distance::Distance(unsigned ft, double in){
    this->_feet = ft;
    this->_inches = in;
    init();
}

Distance::Distance(double in){
    this->_feet = 0;
    this->_inches = in;
    init();
}

unsigned Distance::getFeet() const{
    return this->_feet;
}

double Distance::getInches() const{
    return this->_inches;
}

double Distance::distanceInInches() const{
    double totalInches = this->_inches + this->_feet*12;
    return totalInches;
}

double Distance::distanceInFeet() const{
    double totalFeet = this->_feet + this->_inches/12;
    return totalFeet;
}

double Distance::distanceInMeters() const{
    double distInMeters = distanceInInches()*0.0254;
    return distInMeters;
}

Distance Distance::operator+(const Distance &rhs) const{
    Distance addDist;
    addDist._feet = this->_feet + rhs._feet;
    addDist._inches = this->_inches + rhs._inches;
    addDist.init();
    return addDist;
}

Distance Distance:: operator-(const Distance &rhs) const{
    Distance subDist;
    if (this->_feet > rhs._feet){
        subDist._inches = this->_inches - rhs._inches;
        subDist._feet = this->_feet - rhs._feet;
        while (subDist._inches < 0){
            subDist._inches = 12 - subDist._inches * -1;
            subDist._feet--;
        }
    }
    if(this->_feet <= rhs._feet) {
        subDist._inches =  rhs._inches - this->_inches;
        subDist._feet =  rhs._feet - this->_feet;
        while (subDist._inches < 0){
            subDist._inches = 12 - subDist._inches * -1;
            subDist._feet--;
        }
    }    
    subDist.init();
    return subDist;
}

ostream& operator<<(ostream& out, const Distance &rhs){
    out << rhs._feet << "' " << rhs._inches << "\"";
    return out;
}

void Distance::init(){
    if(this->_inches < 0)
        this->_inches = this->_inches*-1;
    if(this->_feet < 0)
        this->_feet = this->_feet*-1;
    while(this->_inches > 12){
        this->_feet++;
        this->_inches -= 12;
    } 
    
}