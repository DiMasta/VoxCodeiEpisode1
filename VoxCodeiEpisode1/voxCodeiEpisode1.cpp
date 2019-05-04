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
//#define DEBUG_ONE_TURN
//#define OUTPUT_GAME_DATA

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
static const int ALL_CELLS = MAX_HEIGHT * MAX_WIDTH;
static const int MAX_ACTIONS_COUNT = ALL_CELLS * 4;
static const int BOMB_RADIUS = 3;
static const int BOMB_ROUNDS_TO_EXPLODE = 3;
static const int SECOND_TURN = 1;

/// Flags
static const unsigned int SOLUTION_FOUND_FLAG =		0b1000'0000'0000'0000'0000'0000'0000'0000;
static const unsigned int DESTROYED_FLAG =			0b0100'0000'0000'0000'0000'0000'0000'0000;

typedef unsigned char Cell;

static const Cell EMPTY = '.';
static const Cell WALL = '#';
static const Cell S_NODE_GOOD_FOR_BOMB		= 0b1000'0000; // The cell is surveillance node, but if the node is destroyed a bomb may be placed there to destroy other surveillance nodes
static const Cell SURVEILLANCE_NODE			= 0b0100'0000; // '@' as the input, This flag may not be used, since there is counter for nodes in cell
static const Cell BOMB_FLAG					= 0b0010'0000; // The cell is bomb
static const Cell EMPTY_FLAG				= 0b0001'0000; // The cell is empty
static const Cell S_NODES_IN_CELL_MASK		= 0b0000'0011; // Store how many surveillance nodes are in cell, in the last 2 bits of the cell


enum class Direction : int {
	INVALID = -1,
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
	Action() : row(INVALID_IDX), col(INVALID_IDX), afffectedSNodesCount(0), palcementRound(INVALID_IDX) {}
	Action(int row, int col, int afffectedSNodesCount, int palcementRound) :
		row(row), col(col), afffectedSNodesCount(afffectedSNodesCount), palcementRound(palcementRound) {}

	/// Set the default values
	void init();

	int row; ///< where to place the bomb
	int col; ///< where to place the bomb
	int afffectedSNodesCount; ///< How many surveillance nodes are affected for this cell
	int palcementRound; ///< The round when the bomb should be placed
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

/// Represents surveillance node
class SNode {
public:
	/// Adding default constructor, because there will be big array of SNode default initialized
	SNode();

	void setRow(int row) { this->row = row; }
	void setCol(int col) { this->col = col; }
	void setDirection(Direction direction) { this->direction = direction; }

	int getRow() const {
		return row;
	}

	int getCol() const {
		return col;
	}

	Direction getDirection() const {
		return direction;
	}

	/// Initialize the node with its correct direction and coordinates
	/// @param[in] rowIdx surveillance node row idx
	/// @param[in] colIdx surveillance node col idx
	/// @param[in] direction the direction of the node, derived from the initial grid
	void init(int rowIdx, int colIdx, Direction direction);

	void setFlag(unsigned int flag);
	void unsetFlag(unsigned int flag);
	bool hasFlag(unsigned int flag) const;

private:
	int row; ///< surveillance node row idx
	int col; ///< surveillance node col idx

	Direction direction; ///< movement direction

	unsigned int flags; ///< surveillance node state
};

//*************************************************************************************************************
//*************************************************************************************************************

SNode::SNode() :
	row(INVALID_IDX),
	col(INVALID_IDX),
	direction(Direction::INVALID),
	flags(0)
{
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::init(int rowIdx, int colIdx, Direction direction) {
	row = rowIdx;
	col = colIdx;
	this->direction = direction;
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::setFlag(unsigned int flag) {
	flags |= flag;
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::unsetFlag(unsigned int flag) {
	flags &= ~flag;
}

//*************************************************************************************************************
//*************************************************************************************************************

bool SNode::hasFlag(unsigned int flag) const {
	return flags & flag;
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
	/// @param[in] tunrIdx the current turn index
	/// @param[in] cell the value for the cell, that will be set
	/// @param[in] firstTurn true if the initial grid is filled
	void createCell(int rowIdx, int colIdx, int turnIdx, Cell cell);

	/// Get cell, without checking for valid coordinates, which may be a mistake
	/// @param[in] rowIdx the index of the row, for the cell
	/// @param[in] colIdx the index of the column, for the cell
	/// @return the cell value
	Cell getCell(int rowIdx, int colIdx) const;

	/// Iterate the whole grid and set for each cell how many surveillance nodes will be destroyed if a bomb is set there
	/// This hides the risk when a bomb is detonated it changes the cell evaluation, but I think for starting marking
	/// of the best cells to check is OK
	/// @param[in] simulationStartRound which turn is checked now
	void evaluateGridCells(int simulationStartRound);

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
	/// @param[in] palcementRound the round when the bomb should be placed
	void addAction(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound);

	/// Ovewrite the actionIdx-th action, in which cell to place a bomb
	/// @param[in] actionIdx action's index
	/// @param[in] rowIdx the index of the row, to place a bomb
	/// @param[in] colIdx the index of the column, to place a bomb
	/// @param[in] afffectedSNodesCount how many sureveillance nodes are affected from this cell
	/// @param[in] palcementRound the round when the bomb should be placed
	void setAction(int actionIdx, int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound);

	/// Get action with given index
	/// @param[in] actionIdx action's index
	/// @return the action
	const Action& getAction(int actionIdx) const;

	/// Sort all possible action based on how many surveillance nodes are affected by each
	void sortActions();

	/// Perform actions using DFS, check all possible actions combination until solution is found or max depth is reached
	/// when max depth(leave) is reached simulate all gathered actions
	/// @param[in] tunrIdx the current turn index
	void dfsActions(int turnIdx);

	/// Perform recursion to generate all actions combinations, at the bottom of the recursion simulate the action sequance
	/// @param[in] tunrIdx the current turn index
	/// @param[in] recursionFlags shows which actions are played so far and if a solution is found
	/// @param[in] depth the current depth of the recursion
	/// @param[in] actionsToPerform sequence of actions to be tested
	void recursiveDFSActions(int turnIdx, unsigned int recursionFlags, int depth, const vector<int>& actionsToPerform);

	/// Simulate the given sequence of actions, place and activate bombs taking the turns count in account
	/// @param[in] tunrIdx the current turn index
	/// @param[in] actionsToPerform which action to take in certain order
	void simulate(int turnIdx, const vector<int>& actionsToPerform);

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

	/// Create surveillance node based on the initial positions of all nodes, to derive their movement directions
	/// @param[in] rowIdx the current row index of the surveillance node
	/// @param[in] colIdx the current column index of the surveillance node
	void createdSNode(int rowIdx, int colIdx);

	/// Move all nodes in their directions round by round and echa round track the best empty cells
	/// If an empty cell, which reaches all nodes, is found, break the simulation and use that cell in the that round
	/// @param[in] simulationStartRound how many turns are alreadu passed
	void simulateAllRounds(int simulationStartRound);

	/// Move nodes for the given round
	void moveSNodes();

	/// First unset the current cell to be surveillance node, then set the new cell
	void moveSNode(int sNodeIdx);

	/// Extract the infromation for the count of the surveillance nodes in the given cell
	/// @param[in] cell the cell to check
	/// @return the count of surveilllance nodes in the cell
	int getCellSNodesCount(const Cell& cell) const;

private:
	/// All nodes scatered across the grid
	SNode sNodes[ALL_CELLS];

	/// All possible actions for the grid, including placing bombs on nodes (after thery are destroyed)
	Action actions[MAX_ACTIONS_COUNT];

	/// The sequence of the best actions
	int actionsBestSequence[ALL_CELLS];

	/// The initial grid, used to derive the moving directions of the nodes
	Cell initialGrid[MAX_HEIGHT][MAX_WIDTH];

	/// The original firewall grid for the game
	Cell grid[MAX_HEIGHT][MAX_WIDTH];

	/// Grid used to simulate actions, bombs placing and activation
	Cell simulationGrid[MAX_HEIGHT][MAX_WIDTH];

	/// The input grid for each turn
	Cell turnGrid[MAX_HEIGHT][MAX_WIDTH];

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

void Grid::createCell(int rowIdx, int colIdx, int turnIdx, Cell cell) {
	if (0 == turnIdx) {
		initialGrid[rowIdx][colIdx] = cell;
	}

	if (SECOND_TURN == turnIdx) {
		if (SURVEILLANCE_NODE == cell) {
			// Create snode, based on the intial grid positions of nodes
			createdSNode(rowIdx, colIdx);
		}
		else if (EMPTY == cell) {
			cell = EMPTY_FLAG; // Using flag, beacuse '.' uses more bits in the char
		}

		grid[rowIdx][colIdx] = cell;
	}

	// Every turn fill the turn grid
	turnGrid[rowIdx][colIdx] = cell;
}

//*************************************************************************************************************
//*************************************************************************************************************

Cell Grid::getCell(int rowIdx, int colIdx) const {
	return grid[rowIdx][colIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::evaluateGridCells(int simulationStartRound) {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			const Cell& cell = grid[rowIdx][colIdx];

			// Consider empty and surveillance node as possible places for bombs
			// Now snodes moves so in previous turns bomb may be placed on that cell(if it is empty on that turn)
			if (WALL != cell) {
				// This the count of nodes if the bomb explodes, so the placement of the bomb should be 2 turns before this
				const int surveillanceNodesInRange = countSurveillanceNodesInRange(rowIdx, colIdx);

				// If the nodes in range are 0 do not set the char to 0, because it is NULL and will terminate the row
				if (surveillanceNodesInRange) {
					const int placementRound = simulationStartRound - (BOMB_ROUNDS_TO_EXPLODE - 1);

					// Ignore cells where only one node will be destroyed, not sure if this is right
					if (surveillanceNodesInRange > 1) {
						addAction(rowIdx, colIdx, surveillanceNodesInRange, placementRound);
					}

					// Only one action is needed to destroy all surveillance nodes
					if (surveillanceNodesInRange == sNodesCount) {
						solutionFound = true;
						setAction(0, rowIdx, colIdx, surveillanceNodesInRange, placementRound); // Overwrite first action
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
	// If the current cell contains surveillance count them
	int affectedNodesCount = getCellSNodesCount(getCell(rowIdx, colIdx));

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

			affectedNodesCount += getCellSNodesCount(cell);
		}
		else {
			break;
		}
	}

	return affectedNodesCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::addAction(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound) {
	actions[actionsCount++] = Action(rowIdx, colIdx, afffectedSNodesCount, palcementRound);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::setAction(int actionIdx, int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound) {
	actions[actionIdx] = Action(rowIdx, colIdx, afffectedSNodesCount, palcementRound);
}

//*************************************************************************************************************
//*************************************************************************************************************

const Action& Grid::getAction(int actionIdx) const {
	return actions[actionIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::sortActions() {
	sort(actions, actions + actionsCount, biggestAction);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::dfsActions(int turnIdx) {
	vector<int> actionsToPerform;
	actionsToPerform.reserve(actionsCount);

	recursiveDFSActions(turnIdx, 0, 0, actionsToPerform);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::recursiveDFSActions(int turnIdx, unsigned int recursionFlags, int depth, const vector<int>& actionsToPerform) {
	if (solutionFound) {
		return;
	}

	// Each level of the tree represent placing a bomb, no need more levels than bomb
	// Rounds left will be taken in account when simualtion the action sequence
	if (bombsLeft == depth) {
		simulate(turnIdx, actionsToPerform);
		return;
	}
	else if (bombsLeft < depth) {
		return;
	}

	for (int actionIdx = 0; actionIdx < actionsCount; ++actionIdx) {
		if (solutionFound) {
			break;
		}

		const unsigned int actionBit = 1 << actionIdx;
		if (!(actionBit & recursionFlags)) {
			const unsigned int newFlags = recursionFlags | actionBit;
			vector<int> newActions = actionsToPerform;
			newActions.push_back(actionIdx);

			recursiveDFSActions(turnIdx, newFlags, depth + 1, newActions);
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

// TODO: use bit encoding for actions indecies
void Grid::simulate(int turnIdx, const vector<int>& actionsToPerform) {
	resetForSimulation();

	int surveillanceNodesDestroyed = 0;
	int actionIdxToCheck = 0; // Update only after a bomb is placed

	for (int roundIdx = turnIdx; roundIdx < roundsLeft; ++roundIdx) {
		// TODO: move nodes each simulation round

		if (actionIdxToCheck < static_cast<int>(actionsToPerform.size())) {
			const int actionIdx = actionsToPerform[actionIdxToCheck];
			const Action& actionToPerform = actions[actionIdx];

			if (couldPlaceBomb(actionToPerform)) {
				placeBomb(actionToPerform);

				actionsBestSequence[solutionActionsCount++] = actionIdx;
				// Action performed, move to next action
				++actionIdxToCheck;
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
			simulationGrid[rowIdx][colIdx] = turnGrid[rowIdx][colIdx];
		}
	}

	solutionActionsCount = 0;
	bombsCount = 0;
}

//*************************************************************************************************************
//*************************************************************************************************************

bool Grid::couldPlaceBomb(const Action& action) const {
	const Cell cell = simulationGrid[action.row][action.col];

	return !(cell & SURVEILLANCE_NODE);
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

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::createdSNode(int rowIdx, int colIdx) {
	Direction nodeDirection = Direction::INVALID;

	// Check all four neighbour cells to find from where the snode comes
	for (const Direction direction : directions) {
		int rowNeighbour = rowIdx;
		int colNeighbour = colIdx;

		rowNeighbour += MOVE_IN_ROWS[static_cast<int>(direction)];
		colNeighbour += MOVE_IN_COLS[static_cast<int>(direction)];

		if (rowNeighbour < 0 || rowNeighbour >= height || colNeighbour < 0 || colNeighbour >= width) {
			continue;
		}

		const Cell& cell = initialGrid[rowNeighbour][colNeighbour];
		if (SURVEILLANCE_NODE == cell) {
			const int rowDiff = rowIdx - rowNeighbour;
			const int colDiff = colIdx - colNeighbour;

			if (rowDiff > 0) {
				nodeDirection = Direction::DOWN;
			}
			else if (rowDiff < 0) {
				nodeDirection = Direction::UP;
			}
			else if (colDiff > 0) {
				nodeDirection = Direction::RIGHT;
			}
			else if (colDiff < 0) {
				nodeDirection = Direction::LEFT;
			}
			break;
		}
	}

	sNodes[sNodesCount++].init(rowIdx, colIdx, nodeDirection);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::simulateAllRounds(int simulationStartRound) {
	for (int roundIdx = simulationStartRound; roundIdx < roundsLeft; ++roundIdx) {
		moveSNodes();

		// First two turns just move the nodes, so when the bomb is placed it affetct in right turn
		if (roundIdx >= simulationStartRound + (BOMB_ROUNDS_TO_EXPLODE - 1)) {
			evaluateGridCells(roundIdx);
		}

		if (solutionFound) {
			break;
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::moveSNodes() {
	for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
		moveSNode(sNodeIdx);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::moveSNode(int sNodeIdx) {
	SNode& sNode = sNodes[sNodeIdx];
	Direction sNodeDirection = sNode.getDirection();

	if (Direction::INVALID != sNodeDirection) {
		const int sNodeRow = sNode.getRow();
		const int sNodeCol = sNode.getCol();

		Cell& currentCell = grid[sNodeRow][sNodeCol];
		Cell sNodesInCell = S_NODES_IN_CELL_MASK & currentCell;
		
		if (sNodesInCell > 1) {
			currentCell = SURVEILLANCE_NODE;
			currentCell |= (--sNodesInCell);
		}
		else {
			currentCell = EMPTY_FLAG;
		}

		int newRow = sNodeRow + MOVE_IN_ROWS[static_cast<int>(sNodeDirection)];
		int newCol = sNodeCol + MOVE_IN_COLS[static_cast<int>(sNodeDirection)];

		bool changeDirection = false;
		if (newRow < 0 || newRow >= height || newCol < 0 || newCol >= width) {
			changeDirection = true;
		}

		if (!changeDirection) {
			const Cell& cell = grid[newRow][newCol];
			changeDirection = cell == WALL;
		}

		if (changeDirection) {
			switch (sNodeDirection) {
				case Direction::UP: {
					sNodeDirection = Direction::DOWN;
					break;
				}
				case Direction::DOWN: {
					sNodeDirection = Direction::UP;
					break;
				}
				case Direction::LEFT: {
					sNodeDirection = Direction::RIGHT;
					break;
				}
				case Direction::RIGHT: {
					sNodeDirection = Direction::LEFT;
					break;
				}
			}

			newRow = sNodeRow + MOVE_IN_ROWS[static_cast<int>(sNodeDirection)];
			newCol = sNodeCol + MOVE_IN_COLS[static_cast<int>(sNodeDirection)];
		}

		sNode.setRow(newRow);
		sNode.setCol(newCol);
		sNode.setDirection(sNodeDirection);

		Cell& newCell = grid[newRow][newCol];
		Cell sNodesInNewCell = S_NODES_IN_CELL_MASK & newCell;
		newCell = SURVEILLANCE_NODE;
		newCell |= ++sNodesInNewCell;
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::getCellSNodesCount(const Cell& cell) const {
	int sNodesCount = 0;

	if (SURVEILLANCE_NODE & cell) {
		sNodesCount = static_cast<int>(cell & S_NODES_IN_CELL_MASK);
	}

	return sNodesCount;
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

	/// Index of the action to perform, when the round for the action is reached
	int actionIdx;
};

//*************************************************************************************************************
//*************************************************************************************************************

Game::Game() :
	turnsCount(0),
	actionIdx(0)
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

	for (int rowIdx = 0; rowIdx < firewallGrid.getHeight(); ++rowIdx) {
		string row; // one line of the firewall grid
		getline(cin, row);

#ifdef OUTPUT_GAME_DATA
		cerr << row << endl;
#endif // OUTPUT_GAME_DATA

		// First and second turn create additioanl grids to find the movement of the nodes
		for (int colIdx = 0; colIdx < firewallGrid.getWidth(); ++colIdx) {
			const Cell cell = row[colIdx];
			firewallGrid.createCell(rowIdx, colIdx, turnsCount, cell);
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::turnBegin() {
	if (turnsCount == SECOND_TURN) {
		// The nodes movement is calculated, proceed with simulation
		// The goal of the simulation is to find empty cell with the most nodes in range for bombs
		// May be every time when updating node mark all empty cells which have access to it
		// And at the end of each round simulation perform only one 2d traversal of the grid to find the best empty cells
		firewallGrid.simulateAllRounds(turnsCount);
		firewallGrid.sortActions();

		// Test dfs with unlimitted time, maybe it's not nice place to use dfs here
		firewallGrid.dfsActions(turnsCount);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::makeTurn() {
	if (firewallGrid.getSolutionFound()) {
		const Action& action = firewallGrid.getAction(actionIdx);
		if (action.palcementRound == turnsCount) {
			cout << action;
			++actionIdx;
		}
		else {
			cout << WAIT << endl;
		}
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
