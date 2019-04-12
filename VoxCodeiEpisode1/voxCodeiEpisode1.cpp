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

#define REDIRECT_CIN_FROM_FILE
#define REDIRECT_COUT_TO_FILE
#define OUTPUT_GAME_DATA
#define DEBUG_ONE_TURN

const string INPUT_FILE_NAME = "input.txt";
const string OUTPUT_FILE_NAME = "output.txt";

static const int INVALID_ID = -1;
static const int INVALID_NODE_DEPTH = -1;
static const int TREE_ROOT_NODE_DEPTH = 1;
static const int ZERO_CHAR = '0';
static const int DIRECTIONS_COUNT = 4;  // UP, DOWN, LEFT, RIGHT
static const int BYTE_SIZE = 8;
static const int PAIR = 2;

static const int MAX_HEIGHT = 19;
static const int MAX_WIDTH = 19;
static const int BOMB_RADIUS = 3;

static const char EMPTY = '.';
static const char WALL = '#';
static const char SURVEILLANCE_NODE = '@';

enum class Direction : int {
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3,
};

static const Direction directions[] = {
	Direction::UP,
	Direction::DOWN,
	Direction::LEFT,
	Direction::RIGHT
};

static const int MOVE_IN_ROWS[DIRECTIONS_COUNT] = { -1, 1, 0, 0 };
static const int MOVE_IN_COLS[DIRECTIONS_COUNT] = { 0, 0, -1, 1 };

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

class Grid {
public:
	void setHeight(int height) { this->height = height; }
	void setWidth(int width) { this->width = width; }

	int getHeight() const {
		return height;
	}

	int getWidth() const {
		return width;
	}

	/// Set cell entry, without checking for valid coordinates, which may be a mistake
	/// @param[in] rowIdx the index of the row, where the cell will be set
	/// @param[in] colIdx the index of the column, where the cell will be set
	/// @param[in] cell the value for the cell, that will be set
	void setCell(int rowIdx, int colIdx, char cell);

	/// Get cell, without checking for valid coordinates, which may be a mistake
	/// @param[in] rowIdx the index of the row, for the cell
	/// @param[in] colIdx the index of the column, for the cell
	/// @return the cell value
	char getCell(int rowIdx, int colIdx) const;

	/// Iterate the whole grid and set for each cell how many surveillance nodes will be destroyed if a bomb is set there
	/// This hides the risk when a bomb is detonated it changes the cell evaluation, but I think for starting marking
	/// of the best cells to check is OK
	void evaluateGridCells();

	/// Count how many surveillance nodes will be affected if a bomb is placed in the cell with the given coordinates
	/// @param[in] rowIdx the index of the row, for the cell to check
	/// @param[in] colIdx the index of the column, for the cell to check
	/// @return surveillance nodes in range count
	int countSurveillanceNodesInRange(int rowIdx, int colIdx) const;

	/// Count how many surveillance nodes will be affected, in certain direction,
	/// if a bomb is placed in the cell with the given coordinates
	/// @param[in] rowIdx the index of the row, for the cell to check
	/// @param[in] colIdx the index of the column, for the cell to check
	/// @param[in] direction the direction for which to check
	/// @return surveillance nodes in range count
	int countSurveillanceNodesInRangeForDirection(int rowIdx, int colIdx, Direction direction) const;

private:
	char grid[MAX_HEIGHT][MAX_WIDTH];

	int height;
	int width;
};

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::setCell(int rowIdx, int colIdx, char cell) {
	grid[rowIdx][colIdx] = cell;
}

//*************************************************************************************************************
//*************************************************************************************************************

char Grid::getCell(int rowIdx, int colIdx) const {
	return grid[rowIdx][colIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::evaluateGridCells() {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			char& cell = grid[rowIdx][colIdx];

			if (EMPTY == cell) {
				const int surveillanceNodesInRange = countSurveillanceNodesInRange(rowIdx, colIdx);

				// If the nodes in range are 0 do not set the char to 0, because it is NULL and will terminate the row
				if (surveillanceNodesInRange) {
					cell = static_cast<int>(surveillanceNodesInRange);
				}
			}
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::countSurveillanceNodesInRange(int rowIdx, int colIdx) const {
	int affectedNodesCount = 0;

	for (const Direction direction : directions) {
		affectedNodesCount += countSurveillanceNodesInRangeForDirection(rowIdx, colIdx, direction);
	}

	return affectedNodesCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::countSurveillanceNodesInRangeForDirection(int rowIdx, int colIdx, Direction direction) const {
	int affectedNodesCount = 0;

	for (int range = 0; range < BOMB_RADIUS; ++range) {
		rowIdx += MOVE_IN_ROWS[static_cast<int>(direction)];
		colIdx += MOVE_IN_COLS[static_cast<int>(direction)];

		if (rowIdx >= 0 && rowIdx < height && colIdx >= 0 && colIdx < width) {
			const char cell = getCell(rowIdx, colIdx);

			if (WALL == cell) {
				break;
			}

			if (SURVEILLANCE_NODE == cell) {
				++affectedNodesCount;
			}

			// Do not considering other bombs or empty cells
		}
		else {
			break;
		}
	}

	return affectedNodesCount;
}

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

	/// The game grid
	Grid firewallGrid;
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
	firewallGrid.evaluateGridCells();
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
	firewallGrid.setWidth(width);
	firewallGrid.setHeight(height);

#ifdef OUTPUT_GAME_DATA
	cerr << width << " " << height << endl;
#endif // OUTPUT_GAME_DATA

	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		string row; // one line of the firewall grid
		getline(cin, row);

#ifdef OUTPUT_GAME_DATA
		cerr << row << endl;
#endif // OUTPUT_GAME_DATA

		for (int colIdx = 0; colIdx < width; ++colIdx) {
			const char cell = row[colIdx];
			firewallGrid.setCell(rowIdx, colIdx, cell);
		}
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
