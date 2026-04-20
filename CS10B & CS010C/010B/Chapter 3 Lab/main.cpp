#include <iostream>

using namespace std;

// Rational Class declaration
class Rational {
   private:
      int numerator;
      int denominator; 
   public:
      Rational();
      explicit Rational(int numerator); 
      Rational(int numerator, int denominator); 
      Rational add(const Rational & input) const; 
      Rational subtract(const Rational & input) const; 
      Rational multiply(const Rational & input) const; 
      Rational divide(const Rational & input) const;
      void simplify();
      void display() const;
   private:
      int gcd(int num, int denom) const;
};

// Implement Rational class member functions here
Rational::Rational() {
   this->numerator = 0;
   this->denominator = 1;
}

Rational::Rational(int numerator){
   this-> numerator = numerator;
   this-> denominator = 1;
} 

Rational::Rational(int numerator, int denominator){
   this-> numerator = numerator;
   this-> denominator = denominator;
}

Rational Rational::add(const Rational & input) const{
   Rational t;
   t.numerator = (input.numerator*this->denominator) + (input.denominator*this->numerator);
   t.denominator = input.denominator*this->denominator;
   return t;
}
Rational Rational::subtract(const Rational & input) const{
   Rational t;
   t.numerator = (input.denominator*this->numerator) - (input.numerator*this->denominator);
   t.denominator = input.denominator*this->denominator;
   return t;
}
Rational Rational::multiply(const Rational & input) const{
   Rational t;
   t.numerator = input.numerator*this->numerator;
   t.denominator = input.denominator*this->denominator;
   return t;
}
Rational Rational::divide(const Rational & input) const{
   Rational t;
   t.numerator = input.denominator*this->numerator;
   t.denominator = input.numerator*this->denominator;
   return t;
}

int Rational::gcd(int num, int denom) const{
   int temp;
   int tNum = num;
   int tDen = denom;
   while ((tNum % tDen) > 0)  {
      temp = tNum % tDen;
      tNum = tDen;
      tDen = temp;
   }
   return tDen;
}

void Rational::simplify(){
   int temp = this->numerator;
   this->numerator = this->numerator/(gcd(this->numerator, this->denominator));
   this->denominator = this->denominator/(gcd(temp, this->denominator));
  }

void Rational::display() const{
   cout << this->numerator << " / " << this->denominator;
}

// Do not change any of the code below this line!!

Rational getRational();
void displayResult(const string &, const Rational &, const Rational&, const Rational&);

int main() {
   Rational A, B, result;
   char choice;
   
   cout << "Enter Rational A:" << endl;
   A = getRational();
   cout << endl;
   
   cout << "Enter Rational B:" << endl;
   B = getRational();
   cout << endl;
   
   cout << "Enter Operation:" << endl
      << "a - Addition (A + B)" << endl
      << "s - Subtraction (A - B)" << endl
      << "m - Multiplication (A * B)" << endl
      << "d - Division (A / B)" << endl
      << "y - Simplify A" << endl;
   cin >> choice;
   cout << endl;
   
   if (choice == 'a') {
      result = A.add(B);
      displayResult("+", A, B, result);
   } else if (choice == 's') {
      result = A.subtract(B);
      displayResult("-", A, B, result);
   } else if (choice == 'm') {
      result = A.multiply(B);
      displayResult("*", A, B, result);
   } else if (choice == 'd') {
      result = A.divide(B);
      displayResult("/", A, B, result);
   } else if (choice == 'y') {
      A.simplify();
      A.display();
   } else {
      cout << "Unknown Operation";
   }
   cout << endl;
   
   return 0;
}

Rational getRational() {
   int choice;
   int numer, denom;
   
   cout << "Which Rational constructor? (Enter 2, 1, or 0)" << endl
      << "2 - 2 parameters (numerator & denominator)" << endl
      << "1 - 1 parameter (numerator)" << endl
      << "0 - 0 parameters (default)" << endl;
   cin >> choice;
   cout << endl;
   
   if (choice == 2) {
      cout << "numerator? ";
      cin >> numer;
      cout << endl;
      cout << "denominator? ";
      cin >> denom;
      cout << endl;
      return Rational(numer, denom);
   } else if (choice == 1) {
      cout << "numerator? ";
      cin >> numer;
      cout << endl;
      return Rational(numer);
   } else {
      return Rational();
   }
}

void displayResult(const string &op, const Rational &lhs, const Rational&rhs, const Rational &result) {
   cout << "(";
   lhs.display();
   cout << ") " << op << " (";
   rhs.display();
   cout << ") = (";
   result.display();
   cout << ")";
}