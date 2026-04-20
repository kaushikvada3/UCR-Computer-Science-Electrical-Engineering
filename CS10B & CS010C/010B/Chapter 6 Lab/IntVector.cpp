#include <iostream> 
#include "IntVector.h"
#include <stdexcept>
using namespace std;

IntVector::IntVector(unsigned capacity, int value){
    _size = capacity;
    _capacity = capacity;
    if(capacity <= 0){
        _data = nullptr;  
    }
    else{
        _data = new int[capacity]; 
        for(unsigned i = 0; i < _size; ++i){
            _data[i] = value;
        }
    }
}

IntVector::~IntVector(){
    delete[] _data;
}

unsigned IntVector::size() const{
    return _size;
}

unsigned IntVector::capacity() const{
    return _capacity;
}

bool IntVector::empty() const{
    return _size == 0;
}

const int& IntVector::at(unsigned index) const{
    if(index >= _size || index < 0){
        throw out_of_range("IntVector::at_range_check");
    }
        return _data[index];
}

const int& IntVector::front() const{
    return _data[0];
}

const int& IntVector::back() const{
    return _data[_capacity-1];
}