#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <set>
#include <queue>
#include <algorithm>
#include <ctime>
#include <deque>
#include <math.h>
#include <climits>
#include <cstring>
#include <fstream>
#include <iterator>

using namespace std;

//#define REDIRECT_CIN_FROM_FILE
//#define REDIRECT_COUT_TO_FILE
//#define OUTPUT_GAME_DATA
//#define DEBUG_ONE_TURN

const string INPUT_FILE_NAME = "input.txt";
const string OUTPUT_FILE_NAME = "output.txt";

const int INVALID_ID = -1;
const int INVALID_NODE_DEPTH = -1;
const int TREE_ROOT_NODE_DEPTH = 1;
const int ZERO_CHAR = '0';
const int DIRECTIONS_COUNT = 8;
const int BYTE_SIZE = 8;
const int PAIR = 2;

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

class Game {
public:
	Game();
	~Game();

	void initGame();
	void gameBegin();
	void gameEnd();
	void gameLoop();
	void getGameInput();
	void getTurnInput();
	void turnBegin();
	void makeTurn();
	void turnEnd();
	void play();

	void debug() const;

private:
	int turnsCount;

	// Game specific members
};

//*************************************************************************************************************
//*************************************************************************************************************

Game::Game() :
	turnsCount(0) 
{

}

//*************************************************************************************************************
//*************************************************************************************************************

Game::~Game() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::initGame() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::gameBegin() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::gameEnd() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::gameLoop() {
	while (true) {
		turnBegin();
		getTurnInput();	
		makeTurn();
		turnEnd();

#ifdef DEBUG_ONE_TURN
		break;
#endif // DEBUG_ONE_TURN
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::getGameInput() {
	int width; // width of the firewall grid
	int height; // height of the firewall grid
	cin >> width >> height; cin.ignore();

#ifdef OUTPUT_GAME_DATA
	cerr << width << " " << height << endl;
#endif // OUTPUT_GAME_DATA

	for (int i = 0; i < height; i++) {
		string mapRow; // one line of the firewall grid
		getline(cin, mapRow);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::getTurnInput() {
	int rounds; // number of rounds left before the end of the game
	int bombs; // number of bombs left
	cin >> rounds >> bombs; cin.ignore();

#ifdef OUTPUT_GAME_DATA
	cerr << rounds << " " << bombs << endl;
#endif // OUTPUT_GAME_DATA
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::turnBegin() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::makeTurn() {
	cout << "0 1" << endl;
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::turnEnd() {
	++turnsCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::play() {
	initGame();
	getGameInput();
	gameBegin();
	gameLoop();
	gameEnd();
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::debug() const {
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

#ifdef TESTS
#include "debug.h"
#endif // TESTS

int main(int argc, char** argv) {
#ifdef TESTS
	doctest::Context context;
	int res = context.run();
#else

#ifdef REDIRECT_CIN_FROM_FILE
	ifstream in(INPUT_FILE_NAME);
	streambuf *cinbuf = cin.rdbuf();
	cin.rdbuf(in.rdbuf());
#endif // REDIRECT_CIN_FROM_FILE

#ifdef REDIRECT_COUT_TO_FILE
	ofstream out(OUTPUT_FILE_NAME);
	streambuf *coutbuf = cout.rdbuf();
	cout.rdbuf(out.rdbuf());
#endif // REDIRECT_COUT_TO_FILE

	Game game;
	game.play();
#endif // TESTS

	return 0;
}

//#include <iostream>
//#include <string>
//#include <vector>
//#include <algorithm>
//
//using namespace std;
//
//int main() {
//	int width; // width of the firewall grid
//	int height; // height of the firewall grid
//	cin >> width >> height; cin.ignore();
//	cerr << width << " " << height << endl;
//	for (int i = 0; i < height; i++) {
//		string mapRow; // one line of the firewall grid
//		getline(cin, mapRow);
//	}
//
//	// game loop
//	while (1) {
//		int rounds; // number of rounds left before the end of the game
//		int bombs; // number of bombs left
//		cin >> rounds >> bombs; cin.ignore();
//		cerr << rounds << " " << bombs << endl;
//
//		// Write an action using cout. DON'T FORGET THE "<< endl"
//		// To debug: cerr << "Debug messages..." << endl;
//
//		cout << "0 1" << endl;
//	}
//}
