#include <iostream>
#include <vector>
using namespace std;

// parameter order will always be jersey #s then ratings
void outputRoster(const vector<int> &, const vector<int> &);
void addPlayer(vector<int> &, vector<int> &);
void removePlayer(vector<int> &, vector<int> &);
void updatePlayerRating(const vector<int> &, vector<int> &);
void outputPlayersAboveRating(const vector<int> &, const vector<int> &);
void menuPrint();
void chooseMenu(vector<int>&, vector<int>&);




int main() {

   vector<int> jerseyNumber(5);//int vector for jersey
   vector<int> playerRating(5); //int vector for rating

   //prompt the user to input 5 pairs of numbers and update them into the vector
   for(int i = 1; i < 6; i++)
   {
      int tempJersey;
      int tempRating;
      

      cout<<"Enter player " << i <<"'s jersey number: "<<endl; 
      cin>>tempJersey;
      if(tempJersey >=0 && tempJersey <= 99)
      {
         jerseyNumber.at(i-1) = tempJersey;
      }
      

      cout<<"Enter player " << i <<"'s rating: "<<endl; 
      cin>>tempRating;
      if(tempRating >=1 && tempRating <= 9)
      {
         playerRating.at(i-1) = tempRating;
      }
      cout<<endl;
   }

   //output the vectors
   cout<<"ROSTER"<<endl;
   for(int i = 1; i < 6; i++)
   {
      cout<<"Player "<<i<<" -- Jersey number: "<<jerseyNumber.at(i-1)<<", Rating: "<<playerRating.at(i-1)<<endl;
   }
   cout<<endl;
   chooseMenu(jerseyNumber, playerRating);
   return 0;
}

//make a void function that is only meant to print out the menu so its easier to deal with an handle when doing the menu
//menu of options {
void menuPrint()
{
   cout<<"MENU"<<endl;
   cout<<"a - Add player"<<endl;
   cout<<"d - Remove player"<<endl;
   cout<<"u - Update player rating"<<endl;
   cout<<"r - Output players above a rating"<<endl;
   cout<<"o - Output roster"<<endl;
   cout<<"q - Quit"<<endl;

   cout<<endl<<"Choose an option: "<<endl;

}

void chooseMenu(vector<int>& jerseyNumber, vector<int>& playerRating)

{
      char tempChar;
      do
      {
         menuPrint();
         cin>>tempChar;
         if(tempChar == 'a')
         {
            addPlayer(jerseyNumber, playerRating);
         }
         if(tempChar == 'd')
         {
            removePlayer(jerseyNumber, playerRating);
         }
         if(tempChar == 'u')
         {
            updatePlayerRating(jerseyNumber, playerRating);
         }
         if(tempChar == 'r')
         {
            outputPlayersAboveRating(jerseyNumber, playerRating);
         }
         if(tempChar == 'o')
         {
           outputRoster(jerseyNumber, playerRating);
         }
      }
      while (tempChar != 'q');

}


void outputRoster(const vector<int> &jerseyNumber, const vector<int> &playerRating)
{
   cout<<"ROSTER"<<endl;
   for(int i = 1; i < 6; i++)
   {
      cout<<"Player "<<i<<" -- Jersey number: "<<jerseyNumber.at(i-1)<<", Rating: "<<playerRating.at(i-1)<<endl;
   }
   cout<<endl;
}


void addPlayer(vector<int> &jerseyNumber, vector<int> &playerRating)
{
   int tempJersey, tempRating;
   
   cout<<"Enter another player's jersey number: "<<endl;
   cin>>tempJersey;
   jerseyNumber.push_back(tempJersey);
   
   cout<<"Enter another player's rating: "<<endl;
   cin>>tempRating;
   cout<<endl;
   playerRating.push_back(tempRating);
}

void removePlayer(vector<int> &jerseyNumber, vector<int> &playerRating)
{
   int tempJerseyNumber;
   cout<<"Enter a jersey number: "; 
   cin>>tempJerseyNumber;
   for(int i = jerseyNumber.size() -1 ; i >=0; i--) //try to iterate through reverse? 🤷‍♂️
   {
      if(jerseyNumber.at(i) == tempJerseyNumber)
      {
         jerseyNumber.erase(jerseyNumber.begin() + i);
         playerRating.erase(playerRating.begin() + i);
         break;
      }
   }

}

void updatePlayerRating(const vector<int> &jerseyNumber, vector<int> & playerRating)
{
   int jerseyInput;
   int ratingInput;
   cout<<"Enter a jersey number: ";
   cin>>jerseyInput;

   for(unsigned i = 0; i < jerseyNumber.size(); i++)
   {
      if(jerseyNumber.at(i) == jerseyInput)
      {
         cout<<"Enter a new rating for player: ";
         cin>>ratingInput;
         playerRating.at(i) = ratingInput;
      }
   }

}

void outputPlayersAboveRating(const vector<int> &jerseyNumber, const vector<int> &playerRating)
{
   int userRatingInput; 
   cout<<"Enter a rating: ";
   cin>>userRatingInput;
   cout<<"ABOVE "<< userRatingInput<<endl;
   for(unsigned i = 1; i < jerseyNumber.size()+1; i++)
   {
      if(playerRating.at(i) > userRatingInput)
         {
            cout<<"Player "<<i<<" -- Jersey Number: "<<jerseyNumber.at(i-1)<<", Rating: "<<playerRating.at(i-1)<<endl;
         }
   }
}

//84 7
23 4
4 5
30 2
66 9
o
q