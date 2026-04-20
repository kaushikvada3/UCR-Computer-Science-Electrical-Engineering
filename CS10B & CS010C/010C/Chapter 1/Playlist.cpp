#include <iostream> 
#include "Playlist.h"
using namespace std;

//constructors
//default constructor -- initialize all variables with default values
PlaylistNode::PlaylistNode():uniqueID("none"), songName("none"), artistName("none"), songLength(0), nextNodePtr(0){}

//constructor with 4 parameters initialization
PlaylistNode::PlaylistNode(string inputID, string inputSongName, string inputArtistName, int inputSongLength)
:uniqueID(inputID), songName(inputSongName), artistName(inputArtistName), songLength(inputSongLength), nextNodePtr(0){}

//insert after takes a parameter, what is before what is being inserted
void PlaylistNode::InsertAfter(PlaylistNode *input){
    PlaylistNode *temp = nextNodePtr;
    nextNodePtr = input;
    input->SetNext(temp);
}

//member function to set the next node in linked list
void PlaylistNode::SetNext(PlaylistNode *input)
{
    nextNodePtr = input; // Assigns the input pointer to `nextNodePtr`
}

// Getter function to retrieve the unique ID of the song
string PlaylistNode::GetID() const
{
    return uniqueID; // Returns the unique ID for the specific playlist node
}

// Getter function to retrieve the name of the song
string PlaylistNode::GetSongName() const
{
    return songName; // Returns the name of the song stored in this node
}

// Getter function to get the artist name of the song
string PlaylistNode::GetArtistName() const
{
    return artistName; // Returns the artist's name associated with the song in this node
}

// Getter function to get the length of the song in seconds
int PlaylistNode::GetSongLength() const
{
    return songLength; // Returns the length of the song (in seconds)
}

// Getter function to retrieve the pointer to the next node in the list
PlaylistNode *PlaylistNode::GetNext() const
{
    return nextNodePtr; // Returns a pointer to the next node in the playlist linked list
}

// Destructor for the `PlaylistNode` class
PlaylistNode::~PlaylistNode()
{
    PlaylistNode *current = this; // Starts with the current node
    while (current != nullptr)
    {                                              // Traverses the list and deletes all nodes starting from `this`
        PlaylistNode *next = current->nextNodePtr; // Temporary pointer to the next node
        delete current;                            // Deletes the current node
        current = next;                            // Moves to the next node
    }
}

// Copy assignment operator for deep copying of a playlist node
PlaylistNode &PlaylistNode::operator=(const PlaylistNode &other)
{
    if (this != &other)
    {                                  // Checks to see if it was self assgined
        uniqueID = other.uniqueID;     // Copies the unique ID 
        songName = other.songName;     // Copies the song name
        artistName = other.artistName; // Copies the artist name
        songLength = other.songLength; // Copies the song length
    }
    return *this;                      // Returns a reference to this node
}

// Member function to print details of a node in a formatted manner
void PlaylistNode::PrintPlaylistNode(PlaylistNode *input)
{
    cout << "Unique ID: " << input->GetID() << endl;                        // Prints the unique ID of the song
    cout << "Song Name: " << input->GetSongName() << endl;                  // Prints the song's name
    cout << "Artist Name: " << input->GetArtistName() << endl;              // Prints the artist's name
    cout << "Song Length (in seconds): " << input->GetSongLength() << endl; // Prints the length of the song in seconds
    cout << endl;                                                           // Adds a newline for better readability
}
