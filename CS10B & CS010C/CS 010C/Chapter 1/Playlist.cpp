#include <iostream> 
#include "Playlist.h"
using namespace std;

//constructors
//default constructor -- initialize all variables
PlaylistNode::PlaylistNode():uniqueID("none"), songName("none"), artistName("none"), songLength(0), nextNodePtr(0){}

//constructor with 4 parameters
PlaylistNode::PlaylistNode(string inputID, string inputSongName, string inputArtistName, int inputSongLength)
:uniqueID(inputID), songName(inputSongName), artistName(inputArtistName), songLength(inputSongLength), nextNodePtr(0){}

//insert after takes a parameter, what is before what is being inserted
void PlaylistNode::InsertAfter(PlaylistNode *input){
    PlaylistNode *temp = nextNodePtr;
    nextNodePtr = input;
    input->SetNext(temp);
}

//setnext sets input as the next node
void PlaylistNode::SetNext(PlaylistNode *input){
    nextNodePtr = input;
}

//accessors
string PlaylistNode::GetID() const{
    return uniqueID;
}

string PlaylistNode::GetSongName() const{
    return songName;
}

string PlaylistNode::GetArtistName() const{
    return artistName;
}
int PlaylistNode::GetSongLength() const{
    return songLength;
}

PlaylistNode* PlaylistNode::GetNext() const{
    return nextNodePtr;
}

PlaylistNode::~PlaylistNode() {
    PlaylistNode* current = this;
    while (current != nullptr) {
        PlaylistNode* next = current->nextNodePtr;
        delete current;
        current = next;
    }
}

PlaylistNode& PlaylistNode::operator=(const PlaylistNode& other) {
    if (this != &other) {
        uniqueID = other.uniqueID;
        songName = other.songName;
        artistName = other.artistName;
        songLength = other.songLength;
        // nextNodePtr is not changed because we're only copying content, not list structure
    }
    return *this;
}


//prints all four variables formatted as given using accessor functions from above
void PlaylistNode::PrintPlaylistNode(PlaylistNode *input) {
    cout << "Unique ID: " << input->GetID() << endl;
    cout << "Song Name: " << input->GetSongName() << endl;
    cout << "Artist Name: " << input->GetArtistName() << endl;
    cout << "Song Length (in seconds): " << input->GetSongLength() << endl;
    cout << endl;
}