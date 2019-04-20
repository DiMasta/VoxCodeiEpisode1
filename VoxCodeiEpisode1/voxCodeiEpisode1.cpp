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

const string WAIT = "WAIT";
const string EMPTY_STRING = "";

static const int INVALID_ID = -1;
static const int INVALID_IDX = -1;
static const int INVALID_NODE_DEPTH = -1;
static const int TREE_ROOT_NODE_DEPTH = 1;
static const int ZERO_CHAR = '0';
static const int DIRECTIONS_COUNT = 4;  // UP, DOWN, LEFT, RIGHT
static const int BYTE_SIZE = 8;
static const int PAIR = 2;

static const int MAX_HEIGHT = 19;
static const int MAX_WIDTH = 19;
static const int MAX_BOMBS = 9;
static const int MAX_ROUNDS = 19;
static const int MAX_ACTIONS = MAX_ROUNDS * 2; // There are may be more than 19 actions to perform, could be more than needed
static const int BOMB_RADIUS = 3;
static const int BOMB_ROUNDS_TO_EXPLODE = 3;
static const unsigned int SOLUTION_FOUND_FLAG = 0b1000'0000'0000'0000'0000'0000'0000'0000;

typedef unsigned char Cell;

static const Cell EMPTY = '.';
static const Cell WALL = '#';
static const Cell S_NODE_GOOD_FOR_BOMB		= 0b1000'0000; // The cell is surveillance node, but if the node is destroyed a bomb may be placed there to destroy other surveillance nodes
static const Cell SURVEILLANCE_NODE			= 0b0100'0000; // '@' as the input
static const Cell BOMB_FLAG					= 0b0010'0000; // The cell is bomb
static const Cell EMPTY_FLAG				= 0b0001'0000; // The cell is bomb

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

/// Represents a turn action, where(row, col) to place a bomb
/// (-1, -1) represents "WAIT" action, needed when a bomb must be placed on not yet destroyed sureveillance node
struct Action {
	/// By default set the invalid action, that means "WAIT"
	Action() : row(INVALID_IDX), col(INVALID_IDX), afffectedSNodesCount(0) {}
	Action(int row, int col, int afffectedSNodesCount) : row(row), col(col), afffectedSNodesCount(afffectedSNodesCount) {}

	/// Set the default values
	void init();

	int row; ///< where to place the bomb
	int col; ///< where to place the bomb

	/// How many surveillance nodes are affected for this cell
	int afffectedSNodesCount;
};

//*************************************************************************************************************
//*************************************************************************************************************

void Action::init() {
	row = INVALID_IDX;
	col = INVALID_IDX;
	afffectedSNodesCount = 0;
}

//*************************************************************************************************************
//*************************************************************************************************************

bool biggestAction(const Action& lhs, const Action& rhs) {
	return lhs.afffectedSNodesCount > rhs.afffectedSNodesCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

std::ostream& operator<<(std::ostream& out, const Action& action) {
	if (INVALID_IDX == action.row || INVALID_IDX == action.col) {
		out << WAIT;
	}
	else {
		out << action.col << " " << action.row;
	}

	out << endl;
	return out;
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

class Bomb {
public:
	/// Initialize bombs variables
	/// @param[in] row bomb row idx
	/// @param[in] col bombcol idx
	void init(int row, int col);

	/// Update the timer of the bomb, decrease it
	/// @return true if the bomb explodes
	bool update();

	/// Simulate the bomb explotion in the simulation grid
	/// @param[in/out] simulationGrid reference to the simulation grid
	/// @param[in] gridHeight the height of the game grid
	/// @param[in] gridWidth the width of the game grid
	/// @param[out] explodedBombs if other bomb is triggered add it for explotion
	/// @param[out] explodedBombsCount if other bomb is triggered increase the triggered bombs
	/// @return how many nodes are destroyed from this explosion
	int explode(Cell (&simulationGrid)[MAX_HEIGHT][MAX_WIDTH],
		int gridHeight,
		int gridWidth, 
		Bomb (&explodedBombs)[MAX_BOMBS],
		int& explodedBombsCount
	) const;

private:
	int row; ///< bomb row idx
	int col; ///< bomb col idx

	/// Starting at 3 and each round decreased, when 0 the bomb explodes
	int roundsLeft;
};

//*************************************************************************************************************
//*************************************************************************************************************

void Bomb::init(int row, int col) {
	this->row = row;
	this->col = col;

	roundsLeft = BOMB_ROUNDS_TO_EXPLODE;
}

//*************************************************************************************************************
//*************************************************************************************************************

bool Bomb::update() {
	--roundsLeft;
	return 0 == roundsLeft;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Bomb::explode(
	Cell (&simulationGrid)[MAX_HEIGHT][MAX_WIDTH],
	int gridHeight,
	int gridWidth,
	Bomb (&explodedBombs)[MAX_BOMBS],
	int& explodedBombsCount
) const {
	int destroyedNodes = 0;

	for (const Direction direction : directions) {
		int rowRange = row;
		int colRange = col;

		// Aplly bomb range
		for (int range = 0; range < BOMB_RADIUS; ++range) {
			rowRange += MOVE_IN_ROWS[static_cast<int>(direction)];
			colRange += MOVE_IN_COLS[static_cast<int>(direction)];

			if (rowRange < 0 || rowRange >= gridHeight || colRange < 0 || colRange >= gridWidth) {
				break; // Continue with next direction
			}

			Cell& cell = simulationGrid[rowRange][colRange];

			if (WALL == cell) {
				break; // Continue with next direction
			}
			else if (SURVEILLANCE_NODE & cell) {
				++destroyedNodes;
				cell &= ~SURVEILLANCE_NODE; // destroy node
			}
			else if (BOMB_FLAG & cell) {
				explodedBombs[explodedBombsCount++].init(rowRange, colRange); // Init new bomb, no matter the timer, it will be activated anyway
				break; // Do not continue this bomb's explotion, activated bomb will have similar range
			}
		}
	}

	simulationGrid[row][col] &= ~BOMB_FLAG; // Clear the bomb from the simulation grid

	return destroyedNodes;
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

class Grid {
public:
	Grid();

	void setHeight(int height) { this->height = height; }
	void setWidth(int width) { this->width = width; }
	void setRoundsLeft(int roundsLeft) { this->roundsLeft = roundsLeft; }
	void setBombsLeft(int bombsLeft) { this->bombsLeft = bombsLeft; }

	int getHeight() const {
		return height;
	}

	int getWidth() const {
		return width;
	}

	int getRoundsLeft() const {
		return roundsLeft;
	}

	int getBombsLeft() const {
		return bombsLeft;
	}

	int setSNodesCount() const {
		return sNodesCount;
	}

	int getActionsCount() {
		return actionsCount;
	}

	bool getSolutionFound() {
		return solutionFound;
	}

	int getSolutionActionsCount() {
		return solutionActionsCount;
	}

	/// Set cell entry, without checking for valid coordinates, which may be a mistake
	/// @param[in] rowIdx the index of the row, where the cell will be set
	/// @param[in] colIdx the index of the column, where the cell will be set
	/// @param[in] cell the value for the cell, that will be set
	void setCell(int rowIdx, int colIdx, Cell cell);

	/// Get cell, without checking for valid coordinates, which may be a mistake
	/// @param[in] rowIdx the index of the row, for the cell
	/// @param[in] colIdx the index of the column, for the cell
	/// @return the cell value
	Cell getCell(int rowIdx, int colIdx) const;

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

	/// Store the given action, in which cell to place a bomb
	/// @param[in] rowIdx the index of the row, to place a bomb
	/// @param[in] colIdx the index of the column, to place a bomb
	/// @param[in] afffectedSNodesCount how many sureveillance nodes are affected from this cell
	void addAction(int rowIdx, int colIdx, int afffectedSNodesCount);

	/// Ovewrite the actionIdx-th action, in which cell to place a bomb
	/// @param[in] actionIdx action's index
	/// @param[in] rowIdx the index of the row, to place a bomb
	/// @param[in] colIdx the index of the column, to place a bomb
	/// @param[in] afffectedSNodesCount how many sureveillance nodes are affected from this cell
	void setAction(int actionIdx, int rowIdx, int colIdx, int afffectedSNodesCount);

	/// Get action with given index
	/// @param[in] actionIdx action's index
	/// @return the action
	const Action& getAction(int actionIdx) const;

	/// Sort all possible action based on how many surveillance nodes are affected by each
	void sortActions();

	/// Perform actions using DFS, check all possible actions combination until solution is found or max depth is reached
	/// when max depth(leave) is reached simulate all gathered actions
	/// @param[in] turnsCount the current turn index
	void dfsActions(int turnsCount);

	/// Perform recursion to generate all actions combinations, at the bottom of the recursion simulate the action sequance
	/// @param[in] recursionFlags shows which actions are played so far and if a solution is found
	/// @param[in] depth the current depth of the recursion
	/// @param[in] actionsToPerform sequence of actions to be tested
	void recursiveDFSActions(unsigned int recursionFlags, int depth, const vector<int>& actionsToPerform);

	/// Simulate the given sequence of actions, place and activate bombs taking the turns count in account
	/// @param[in] actionsToPerform which action to take in certain order
	void simulate(const vector<int>& actionsToPerform);

	/// Reset firewall grid to its original state from the beginning of the turn
	void resetForSimulation();

	/// Check if given action places the bomb on empty cell or surveillance node cell
	/// @param[in] action the action to perform
	/// @return true if the action places bomb on empty cell false otherwise
	bool couldPlaceBomb(const Action& action) const;

	/// Place bomb according to the given action, placing the cell to place the bomb is guartied to be empty
	/// @param[in] action where to place the bomb
	void placeBomb(const Action& action);

	/// Update placed bombs, decrease their timers, if bomb reached 0 timer, explode
	/// @return how many surveillance nodes are destroyed in the simulated turn
	int bombsTick();

	/// Get the index of the solution action for the current turn
	/// @param[in] turnIdx the turn being maked
	/// @return the index of the action for the current turn
	int getSolutionActionIdx(int turnIdx) const;

private:
	/// All possible actions for the grid, including placing bombs on nodes (after thery are destroyed)
	Action actions[MAX_ACTIONS];

	/// The sequence of the best actions
	int actionsBestSequence[MAX_ACTIONS];

	/// The original firewall grid for the game
	Cell grid[MAX_HEIGHT][MAX_WIDTH];

	/// Grid used to simulate actions, bombs placing and activation
	Cell simulationGrid[MAX_HEIGHT][MAX_WIDTH];

	/// Bombs spread throug the simulation grid
	Bomb bombs[MAX_BOMBS];

	int height; ///< Of the firewall grid
	int width; ///< Of the firewall grid

	/// Number of rounds left before the end of the game
	int roundsLeft;

	/// Number of bombs left to be placed
	int bombsLeft;

	/// The count of all surveillance nodes, if they could be destroyed with one bomb, no need of DFS
	int sNodesCount;

	/// All possible places for bombs
	int actionsCount;

	/// Count of best solutions, reset each turn
	int solutionActionsCount;

	/// How many bombs are placed, reset each simulation
	int bombsCount;

	/// Marks if we found set actions, which are solving the puzzle
	bool solutionFound;
};

//*************************************************************************************************************
//*************************************************************************************************************

Grid::Grid() :
	sNodesCount(0),
	actionsCount(0),
	solutionActionsCount(0),
	bombsCount(0),
	solutionFound(false)
{
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::setCell(int rowIdx, int colIdx, Cell cell) {
	if (SURVEILLANCE_NODE == cell) {
		++sNodesCount;
	}
	else if (EMPTY == cell) {
		cell = EMPTY_FLAG;
	}

	grid[rowIdx][colIdx] = cell;
}

//*************************************************************************************************************
//*************************************************************************************************************

Cell Grid::getCell(int rowIdx, int colIdx) const {
	return grid[rowIdx][colIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::evaluateGridCells() {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			Cell& cell = grid[rowIdx][colIdx];
			const bool cellIsEmpty = EMPTY_FLAG == cell;
			const bool cellIsSNode = SURVEILLANCE_NODE == cell;

			if (cellIsEmpty || cellIsSNode) {
				const int surveillanceNodesInRange = countSurveillanceNodesInRange(rowIdx, colIdx);

				// If the nodes in range are 0 do not set the char to 0, because it is NULL and will terminate the row
				if (surveillanceNodesInRange) {
					//cell = static_cast<Cell>(surveillanceNodesInRange);

					// Ignore cells where only one node will be destroyed, not sure if this is right
					if (surveillanceNodesInRange > 1) {
						addAction(rowIdx, colIdx, surveillanceNodesInRange);

						if (cellIsSNode) {
							cell |= S_NODE_GOOD_FOR_BOMB;
						}
					}

					// Only one action is needed to destroy all surveillance nodes
					if (surveillanceNodesInRange == sNodesCount && cellIsEmpty) {
						solutionFound = true;
						setAction(0, rowIdx, colIdx, surveillanceNodesInRange); // Overwrite first action
						actionsBestSequence[solutionActionsCount++] = 0;
						break;
					}
				}
			}
		}

		if (solutionFound) {
			break;
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
			const Cell cell = getCell(rowIdx, colIdx);

			if (WALL == cell) {
				break;
			}

			if ((SURVEILLANCE_NODE == cell) || (cell & S_NODE_GOOD_FOR_BOMB)) {
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

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::addAction(int rowIdx, int colIdx, int afffectedSNodesCount) {
	actions[actionsCount++] = Action(rowIdx, colIdx, afffectedSNodesCount);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::setAction(int actionIdx, int rowIdx, int colIdx, int afffectedSNodesCount) {
	actions[actionIdx] = Action(rowIdx, colIdx, afffectedSNodesCount);
}

//*************************************************************************************************************
//*************************************************************************************************************

const Action& Grid::getAction(int actionIdx) const {
	return actions[actionIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::sortActions() {
	sort(begin(actions), end(actions), biggestAction);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::dfsActions(int turnsCount) {
	vector<int> actionsToPerform;
	actionsToPerform.reserve(actionsCount);

	recursiveDFSActions(0, 0, actionsToPerform);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::recursiveDFSActions(unsigned int recursionFlags, int depth, const vector<int>& actionsToPerform) {
	if (solutionFound) {
		return;
	}

	// Each level of the tree represent placing a bomb, no need more levels than bomb
	// Rounds left will be taken in account when simualtion the action sequence
	if (bombsLeft == depth) {
		simulate(actionsToPerform);
		return;
	}
	else if (bombsLeft < depth) {
		return;
	}

	for (int actionIdx = 0; actionIdx < actionsCount; ++actionIdx) {
		const unsigned int actionBit = 1 << actionIdx;
		if (!(actionBit & recursionFlags)) {
			const unsigned int newFlags = recursionFlags | actionBit;
			vector<int> newActions = actionsToPerform;
			newActions.push_back(actionIdx);

			recursiveDFSActions(newFlags, depth + 1, newActions);
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::simulate(const vector<int>& actionsToPerform) {
	resetForSimulation();

	int surveillanceNodesDestroyed = 0;

	int actionToPerformIdx = 0;
	for (int roundIdx = 0; roundIdx < roundsLeft; ++roundIdx) {
		if (actionToPerformIdx < static_cast<int>(actionsToPerform.size())) {
			const int actionIdx = actionsToPerform[actionToPerformIdx];
			const Action& actionToPerform = actions[actionIdx];

			if (couldPlaceBomb(actionToPerform)) {
				placeBomb(actionToPerform);

				actionsBestSequence[solutionActionsCount++] = actionToPerformIdx;
				++actionToPerformIdx;
			}
			else {
				actionsBestSequence[solutionActionsCount++] = INVALID_ID; // Wait action
			}
		}

		surveillanceNodesDestroyed += bombsTick();
	}

	if (surveillanceNodesDestroyed == sNodesCount) {
		solutionFound = true;
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::resetForSimulation() {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			simulationGrid[rowIdx][colIdx] = grid[rowIdx][colIdx];
		}
	}

	solutionActionsCount = 0;
	bombsCount = 0;
}

//*************************************************************************************************************
//*************************************************************************************************************

bool Grid::couldPlaceBomb(const Action& action) const {
	const Cell cell = simulationGrid[action.row][action.col];

	return !(cell & S_NODE_GOOD_FOR_BOMB);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::placeBomb(const Action& action) {
	Cell& cell = simulationGrid[action.row][action.col];
	
	bombs[bombsCount++].init(action.row, action.col);

	cell |= BOMB_FLAG;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::bombsTick() {
	Bomb explodedBombs[MAX_BOMBS]; // Bombs indecies which are exploding this turn
	int explodedBombsCount = 0;

	// Go through all placed bombs, update their timers
	for (int bombIdx = 0; bombIdx < bombsCount; ++bombIdx) {
		if (bombs[bombIdx].update()) {
			explodedBombs[explodedBombsCount++] = bombs[bombIdx];
		}
	}

	int surveillanceNodesDestroyed = 0;

	// Explode bombs with 0 timers, explode bombs which are triggered by other bombs
	for (int bombIdx = 0; bombIdx < explodedBombsCount; ++bombIdx) {
		surveillanceNodesDestroyed += explodedBombs[bombIdx].explode(simulationGrid, height, width, explodedBombs, explodedBombsCount);
	}

	return surveillanceNodesDestroyed;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::getSolutionActionIdx(int turnIdx) const {
	return actionsBestSequence[turnIdx];
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
	firewallGrid.sortActions();
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::gameEnd() {
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::gameLoop() {
	while (true) {
		getTurnInput();

		if (0 == firewallGrid.getRoundsLeft()) {
			break;
		}

		turnBegin();
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
			const Cell cell = row[colIdx];
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
	firewallGrid.setRoundsLeft(rounds);
	firewallGrid.setBombsLeft(bombs);

#ifdef OUTPUT_GAME_DATA
	cerr << rounds << " " << bombs << endl;
#endif // OUTPUT_GAME_DATA
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::turnBegin() {
	if (0 == turnsCount) {
		firewallGrid.dfsActions(turnsCount);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::makeTurn() {
	if (firewallGrid.getSolutionFound() && turnsCount < firewallGrid.getSolutionActionsCount()) {
		cout << firewallGrid.getAction(firewallGrid.getSolutionActionIdx(turnsCount));
	}
	else {
		cout << WAIT << endl;
	}
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
