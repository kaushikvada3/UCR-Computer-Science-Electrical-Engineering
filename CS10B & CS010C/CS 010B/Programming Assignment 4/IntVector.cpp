#include <iostream> 
#include "IntVector.h"
#include <stdexcept>
using namespace std;

//Constructors and Accessors
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

int& IntVector::at(unsigned index){
    if(index >= _size || index < 0){
        throw out_of_range("IntVector::at_range_check");
    }
        return _data[index];
}

//Destructor, Mutators, & Private Helper Functions
IntVector::~IntVector(){
    delete[] _data;
}

void IntVector::expand(){
    if(_capacity == 0)
        _capacity++;
    else
        _capacity *= 2;
    int *tempPointer = new int[_capacity];

    for(unsigned i = 0; i < _size; ++i){
        tempPointer[i] = _data[i];
    }
    delete[] _data;
    _data = tempPointer;
}

void IntVector::expand(unsigned amount){
    _capacity += amount;
    int *tempPointer = new int[_capacity];
    for(unsigned i = 0; i < _size; ++i){
        tempPointer[i] = _data[i];
    }
    delete[] _data;
    _data = tempPointer;
}

const int& IntVector::front() const{
    return _data[0];
}

int& IntVector::front(){
    return _data[0];
}

const int& IntVector::back() const{
    return _data[_capacity-1];
}

int& IntVector::back(){
    return _data[_capacity-1];
}

void IntVector::insert(unsigned index, int value){
    if(index > _size || index < 0)
        throw out_of_range("IntVector::insert_range_check");
    if(_size >= _capacity){
        expand();
    }
    _size++;
    for(unsigned i = _size-1; i > index; --i){
        _data[i] = _data[i-1];
    }
    _data[index] = value;
}

void IntVector::erase(unsigned index){
    if(index >= _size || index < 0){
        throw out_of_range("IntVector::erase_range_check");
    }
    for(unsigned i = index; i < _size-1; ++i){
        _data[i] = _data[i+1];
    }
    --_size;
}

void IntVector::push_back(int value){
    insert(_size, value);
}

void IntVector::pop_back(){
    _size--;
}

void IntVector::reserve(unsigned n){
    if(_capacity < n){
        expand(n - _capacity);
    }
}

void IntVector::assign(unsigned n, int value){
    
    if(_capacity < n){
        if(n < _capacity*2)
            expand();
        else   
            expand(n-_capacity);
    }
    _size = n;
    int *temp = new int[_capacity];
    for(unsigned i = 0; i < _size; ++i){
        temp[i] = value;
    }
    delete[] _data;
    _data = temp;
}

void IntVector::clear(){
    _size = 0;
}

void IntVector::resize(unsigned size, int value){
    if(size<_size)
        _size = size;
    else{
        if(_capacity < size)
        {
            if(size < _capacity*2 && _capacity != 0)
                expand();
            else
                expand(size-_capacity);
        }
    }
    for(unsigned i = _size; i < size; ++i){
        _data[i] = value;
    }
    _size = size;
}