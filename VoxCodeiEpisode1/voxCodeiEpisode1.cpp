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
//#define DEBUG_ONE_TURN
//#define OUTPUT_GAME_DATA

//const string INPUT_FILE_NAME = "input.txt";
//const string INPUT_FILE_NAME = "input_01_one_moving_node.txt";
//const string INPUT_FILE_NAME = "input_02_2_moving_nodes_2_bombs.txt";
//const string INPUT_FILE_NAME = "input_03_6_moving_nodes_6_bombs.txt";
//const string INPUT_FILE_NAME = "input_04_2_moving_nodes_1_bombs.txt";
//const string INPUT_FILE_NAME = "input_05_9_moving_nodes_9_bombs.txt";
//const string INPUT_FILE_NAME = "input_06_moving_nodes_1_bombs.txt";
//const string INPUT_FILE_NAME = "input_07_indestructible_nodes.txt";
//const string INPUT_FILE_NAME = "input_08_indestructible_nodes_4_bombs.txt";
//const string INPUT_FILE_NAME = "input_09_patience.txt";
const string INPUT_FILE_NAME = "input_10_vandalism.txt";

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
static const int MAX_BOMBS = 10;
static const int MAX_ROUNDS = 19;
static const int ALL_CELLS = MAX_HEIGHT * MAX_WIDTH;
static const int MAX_ACTIONS_COUNT = ALL_CELLS * 4;
static const int MAX_ACTIONS_TO_CHECK = 32; // Limit the checked actions to the size of unsigned int
static const int BOMB_RADIUS = 3;
static const int BOMB_ROUNDS_TO_EXPLODE = 3;
static const int SECOND_TURN = 1;
static const int THIRD_TURN = 2;

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

enum class SimulationType : int {
	S_NODES_DIRECTIONS = 0,
	BEST_ACTIONS,
};

static const Direction directions[] = {
	Direction::UP,
	Direction::DOWN,
	Direction::LEFT,
	Direction::RIGHT
};

/// Reverse direction of the normal direction 
static const Direction reverseDirections[] = {
	Direction::DOWN,
	Direction::UP,
	Direction::RIGHT,
	Direction::LEFT
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
	bool res = lhs.afffectedSNodesCount > rhs.afffectedSNodesCount;

	//if (lhs.afffectedSNodesCount == rhs.afffectedSNodesCount) {
	//	res = lhs.palcementRound < rhs.palcementRound;
	//}

	return res;
}

//*************************************************************************************************************
//*************************************************************************************************************

std::ostream& operator<<(std::ostream& out, const Action& action) {
#ifdef REDIRECT_COUT_TO_FILE
	cout << "\"" << action.col << " " << action.row << "\",";
#else
	out << action.col << " " << action.row;
#endif

	out << endl;
	return out;
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

	int getPossibleDirectionsCount() const {
		return possibleDirectionsCount;
	}

	/// Initialize the node with its correct direction and coordinates
	/// @param[in] rowIdx surveillance node row idx
	/// @param[in] colIdx surveillance node col idx
	/// @param[in] direction the direction of the node, derived from the initial grid
	void init(int rowIdx, int colIdx, Direction direction);

	/// Reset to node to its initial state
	void reset();

	/// Add the given direction to the possible snode directions, the correct one will be chosen from them
	/// @param[in] direction the possible snode direction
	void addPossibleDirection(Direction direction);

	/// Get directionIdxth possible direction
	/// @param[in] directionIdx the index of the wanted possible direction
	/// @return the possible direction
	Direction getPossibleDirection(int directionIdx) const;

	void setFlag(unsigned int flag);
	void unsetFlag(unsigned int flag);
	bool hasFlag(unsigned int flag) const;

private:
	int row; ///< surveillance node row idx
	int col; ///< surveillance node col idx

	Direction direction; ///< movement direction

	/// Initial surveillance node properties, used to reset the node after best turns are gathered
	int initialRow;
	int initialCol;
	Direction initialDirection;

	/// When comparing a snode based on the initial grid it may appear that the snode comes from seceral directions
	/// Use this array to store all posible directions and check all snodes until the turnGrid is matched
	Direction possibleDirections[DIRECTIONS_COUNT];
	int possibleDirectionsCount;

	unsigned int flags; ///< surveillance node state
};

//*************************************************************************************************************
//*************************************************************************************************************

SNode::SNode() :
	row(INVALID_IDX),
	col(INVALID_IDX),
	direction(Direction::INVALID),
	flags(0),
	possibleDirectionsCount(0)
{
	for (Direction& direction : possibleDirections) {
		direction = Direction::INVALID;
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::init(int rowIdx, int colIdx, Direction direction) {
	row = rowIdx;
	col = colIdx;
	this->direction = direction;

	initialRow = rowIdx;
	initialCol = colIdx;
	initialDirection = direction;
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::reset() {
	row = initialRow;
	col = initialCol;
	direction = initialDirection;
	flags = 0;
}

//*************************************************************************************************************
//*************************************************************************************************************

void SNode::addPossibleDirection(Direction direction) {
	possibleDirections[possibleDirectionsCount++] = direction;
}

//*************************************************************************************************************
//*************************************************************************************************************

Direction SNode::getPossibleDirection(int directionIdx) const {
	return possibleDirections[directionIdx];
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

class Bomb {
public:
	/// Initialize bombs variables
	/// @param[in] row bomb row idx
	/// @param[in] col bombcol idx
	/// @param[in] roundsLeft how many rounds before exploding
	void init(int row, int col, int roundsLeft);

	/// Update the timer of the bomb, decrease it
	/// @return true if the bomb explodes
	bool update();

	/// Simulate the bomb explotion in the simulation grid
	/// @param[in/out] simulationGrid reference to the simulation grid
	/// @param[in] gridHeight the height of the game grid
	/// @param[in] gridWidth the width of the game grid
	/// @return how many nodes are destroyed from this explosion
	int explode(
		Cell (&simulationGrid)[MAX_HEIGHT][MAX_WIDTH],
		int gridHeight,
		int gridWidth,
		SNode (&sNodes)[ALL_CELLS],
		int sNodesCount,
		Bomb (&bombs)[MAX_BOMBS],
		int bombsCount
	) const;

	/// Destroy surveillnace node in the given cell, flag it as destroyed
	/// @param[in/out] cell the cell in which the surveillance node is placed
	/// @param[in] rowRange row of the bombs hit
	/// @param[in] colRange col of the bombs hit
	/// @param[in/out] sNodes the surveillnace nodes array to be updated with the destroyed node
	/// @param[in] sNodesCount the count of the surveillnace nodes
	int destroySNode(
		Cell& cell,
		int rowRange,
		int colRange,
		SNode(&sNodes)[ALL_CELLS],
		int sNodesCount
	) const;

private:
	int row; ///< bomb row idx
	int col; ///< bomb col idx

	/// Starting at 3 and each round decreased, when 0 the bomb explodes
	int roundsLeft;
};

//*************************************************************************************************************
//*************************************************************************************************************

void Bomb::init(int row, int col, int roundsLeft) {
	this->row = row;
	this->col = col;
	this->roundsLeft = roundsLeft;
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
	SNode (&sNodes)[ALL_CELLS],
	int sNodesCount,
	Bomb (&bombs)[MAX_BOMBS],
	int bombsCount
) const {
	// First count the surveillance nodes in the cell where the bomb is placed
	int destroyedNodes = destroySNode(simulationGrid[row][col], row, col, sNodes, sNodesCount);

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
				destroyedNodes += destroySNode(cell, rowRange, colRange, sNodes, sNodesCount);
			}
			else if (BOMB_FLAG & cell) {
				for (int bombIdx = 0; bombIdx < bombsCount; ++bombIdx) {
					Bomb& bomb = bombs[bombIdx];
					if (bomb.row == rowRange && bomb.col == colRange) {
						bomb.roundsLeft = 1;
					}
				}

				break; // Do not continue this bomb's explotion in current direction, activated bomb will have similar range
			}
		}
	}

	simulationGrid[row][col] &= ~BOMB_FLAG; // Clear the bomb from the simulation grid

	return destroyedNodes;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Bomb::destroySNode(
	Cell& cell,
	int rowRange,
	int colRange,
	SNode(&sNodes)[ALL_CELLS],
	int sNodesCount
) const {
	// Get count of surveillance nodes in the cell
	int destroyedNodes = static_cast<int>(cell & S_NODES_IN_CELL_MASK);

	// Mark Surveillance node as destroyed
	for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
		SNode& sNode = sNodes[sNodeIdx];
		if (sNode.getRow() == rowRange && sNode.getCol() == colRange) {
			sNode.setFlag(DESTROYED_FLAG);
		}
	}

	// Clear cell after the explosion
	cell = EMPTY_FLAG;

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
	/// @param[out] affectsMovingNode true if moving node is affected
	/// @return surveillance nodes in range count
	int countSurveillanceNodesInRange(int rowIdx, int colIdx, bool& affectsMovingNode) const;

	/// Count how many surveillance nodes will be affected, in certain direction,
	/// if a bomb is placed in the cell with the given coordinates
	/// @param[in] rowIdx the index of the row, for the cell to check
	/// @param[in] colIdx the index of the column, for the cell to check
	/// @param[in] direction the direction for which to check
	/// @param[out] affectsMovingNode true if moving node is affected
	/// @return surveillance nodes in range count
	int countSurveillanceNodesInRangeForDirection(int rowIdx, int colIdx, Direction direction, bool& affectsMovingNode) const;

	/// Store the given action, in which cell to place a bomb
	/// @param[in] rowIdx the index of the row, to place a bomb
	/// @param[in] colIdx the index of the column, to place a bomb
	/// @param[in] afffectedSNodesCount how many sureveillance nodes are affected from this cell
	/// @param[in] palcementRound the round when the bomb should be placed
	/// @return the new action index
	int addAction(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound);

	/// Check if action with the given properties is already added, update its placement round if needed
	/// @param[in] rowIdx the index of the row, to place a bomb
	/// @param[in] colIdx the index of the column, to place a bomb
	/// @param[in] afffectedSNodesCount how many sureveillance nodes are affected from this cell
	/// @param[in] palcementRound the round when the bomb should be placed
	/// @return the new action index
	int actionAlreadyAdded(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound);

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

	/// Reset firewall grid based on the given simulation type
	/// @param[in] simType type of the simulation which is performed
	void resetForSimulation(SimulationType simType);

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
	/// @param[in] tunrIdx the current turn index
	void createSNode(int rowIdx, int colIdx, int turnIdx);

	/// Move all nodes in their directions round by round and echa round track the best empty cells
	/// If an empty cell, which reaches all nodes, is found, break the simulation and use that cell in the that round
	/// @param[in] simulationStartRound how many turns are alreadu passed
	void simulateAllRounds(int simulationStartRound);

	/// Move nodes on the given grid with given directions or nodes own directions
	/// @param[in] directions the overriding directions to use if they are equal to the size of the sureveillance nodes
	/// @param[in/out] gridToUse the grid on which the movement will be performed
	void moveSNodes(const vector<Direction>& directions, Cell(*gridToUse)[MAX_WIDTH]);

	/// First unset the current cell to be surveillance node, then set the new cell
	/// @param[in] sNodeIdx the index of the node to move
	/// @param[in] directions the overriding directions to use if they are equal to the size of the sureveillance nodes
	/// @param[in/out] gridToUse the grid on which the movement will be performed
	void moveSNode(int sNodeIdx, const vector<Direction>& directions, Cell(*gridToUse)[MAX_WIDTH]);

	/// Extract the infromation for the count of the surveillance nodes in the given cell
	/// @param[in] cell the cell to check
	/// @return the count of surveilllance nodes in the cell
	int getCellSNodesCount(const Cell& cell) const;

	/// Use all possible directions for nodes to extract the correct ones
	/// Using recurtion gather all possible combination from the possible directions of all nodes
	/// Move all sureveillance nodes in reverse one turn and compare if the simulation grid is identical with the initialgrid
	/// @param[in] depth the current depth of the search tree
	/// @param[in] directionsToTest combination of directions for the surveillance nodes, new one added in each level of the search tree
	void claculateSNodesMovementDirections(int depth, const vector<Direction>& directionsToTest);

	/// Check the simulation of movement made the same simulation grid as the inital grid
	bool checkNodesInitialPositions() const;

	/// Using the initial surveillance nodes' positions and the second turn positions get the possible directions for nodes
	void fillPossibleSNodesDirections();

	/// Use the calculated directions for the surveillnace nodes and init them for the current turn
	void initNodesForTurn();

private:

	/// All nodes scatered across the grid
	SNode sNodes[ALL_CELLS];

	/// All possible actions for the grid, including placing bombs on nodes (after thery are destroyed)
	Action actions[MAX_ACTIONS_COUNT];

	/// The sequence of the best actions
	int actionsBestSequence[ALL_CELLS];

	/// The initial grid, used to derive the moving directions of the nodes
	Cell initialGrid[MAX_HEIGHT][MAX_WIDTH];

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

	/// Marks if we found the correct directions for the movement of the surveillnace nodes
	bool nodesMovementCalculated;

	/// There is a static node in the level
	bool staticNode;
};

//*************************************************************************************************************
//*************************************************************************************************************

Grid::Grid() :
	sNodesCount(0),
	actionsCount(0),
	solutionActionsCount(0),
	bombsCount(0),
	solutionFound(false),
	nodesMovementCalculated(false),
	staticNode(false)
{
	for (int& bestAction : actionsBestSequence) {
		bestAction = INVALID_IDX;
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::createCell(int rowIdx, int colIdx, int turnIdx, Cell cell) {
	if (SURVEILLANCE_NODE == cell) {
		cell |= 1; // One node in cell
	}
	else if (EMPTY == cell) {
		cell = EMPTY_FLAG; // Using flag, beacuse '.' uses more bits in the char
	}

	if (0 == turnIdx) {
		if (SURVEILLANCE_NODE & cell) {
			createSNode(rowIdx, colIdx, turnIdx);
		}
		initialGrid[rowIdx][colIdx] = cell;
	}

	// Every turn fill the turn grid
	turnGrid[rowIdx][colIdx] = cell;
}

//*************************************************************************************************************
//*************************************************************************************************************

Cell Grid::getCell(int rowIdx, int colIdx) const {
	return simulationGrid[rowIdx][colIdx];
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::evaluateGridCells(int simulationStartRound) {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			const Cell& cell = simulationGrid[rowIdx][colIdx];

			// If there are not static nodes, surveillance nodes could be evaluated
			const bool noStaticNodes = cell != WALL && !staticNode;
			if ((cell & EMPTY_FLAG) || noStaticNodes) {
				// Add action if a moving node is targetted by the potential bomb
				bool affectsMovingNode = false;

				// This the count of nodes if the bomb explodes, so the placement of the bomb should be 2 turns before this
				const int surveillanceNodesInRange = countSurveillanceNodesInRange(rowIdx, colIdx, affectsMovingNode);

				// If the nodes in range are 0 do not set the char to 0, because it is NULL and will terminate the row
				if (surveillanceNodesInRange) {
					const int placementRound = simulationStartRound - (BOMB_ROUNDS_TO_EXPLODE - 1);
					int actionIdx = INVALID_IDX;

					// Ignore cells where only one node will be destroyed, not sure if this is right
					//if (surveillanceNodesInRange > 1 || 1 == sNodesCount) {
					if ((surveillanceNodesInRange > 2 && affectsMovingNode) || surveillanceNodesInRange == sNodesCount) {
						actionIdx = addAction(rowIdx, colIdx, surveillanceNodesInRange, placementRound);
					}

					// Only one action is needed to destroy all surveillance nodes
					if (surveillanceNodesInRange == sNodesCount) {
						solutionFound = true;
						actionsBestSequence[placementRound] = actionIdx;
						solutionActionsCount = placementRound + 1;
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

int Grid::countSurveillanceNodesInRange(int rowIdx, int colIdx, bool& affectsMovingNode) const {
	// If the current cell contains surveillance count them
	int affectedNodesCount = getCellSNodesCount(getCell(rowIdx, colIdx));

	for (const Direction direction : directions) {
		affectedNodesCount += countSurveillanceNodesInRangeForDirection(rowIdx, colIdx, direction, affectsMovingNode);
	}

	return affectedNodesCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::countSurveillanceNodesInRangeForDirection(int rowIdx, int colIdx, Direction direction, bool& affectsMovingNode) const {
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

			for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
				const SNode& sNode = sNodes[sNodeIdx];
				if (rowIdx == sNode.getRow() && colIdx == sNode.getCol() && Direction::INVALID != sNode.getDirection()) {
					affectsMovingNode = true;
					break;
				}
			}
		}
		else {
			break;
		}
	}

	return affectedNodesCount;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::addAction(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound) {
	int actionIdx = actionAlreadyAdded(rowIdx, colIdx, afffectedSNodesCount, palcementRound);

	if (actionsCount < MAX_ACTIONS_COUNT && INVALID_IDX == actionIdx) {
		actionIdx = actionsCount;
		actions[actionsCount++] = Action(rowIdx, colIdx, afffectedSNodesCount, palcementRound);
	}

	return actionIdx;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::actionAlreadyAdded(int rowIdx, int colIdx, int afffectedSNodesCount, int palcementRound) {
	int idx = INVALID_IDX;

	for (int actionIdx = 0; actionIdx < actionsCount; ++actionIdx) {
		Action& action = actions[actionIdx];

		if ((action.row == rowIdx && action.col == colIdx) || action.palcementRound == palcementRound) {
			idx = actionIdx;

			if (afffectedSNodesCount > action.afffectedSNodesCount) {
				action.afffectedSNodesCount = afffectedSNodesCount;
				action.palcementRound = palcementRound;
			}

			break;
		}
	}

	return idx;
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
	if (1 == actionsCount - bombsLeft) {
		vector<int> actionsToPerform{};
		for (int skipActionIdx = 0; skipActionIdx < actionsCount; ++skipActionIdx) {
			for (int useActionIdx = 0; useActionIdx < actionsCount; ++useActionIdx) {
				if (skipActionIdx != useActionIdx) {
					actionsToPerform.push_back(useActionIdx);
				}
			}

			if (!solutionFound) {
				simulate(turnIdx, actionsToPerform);
				actionsToPerform.clear();
			}
			else {
				break;
			}
		}
	}
	else {
		vector<int> actionsToPerform;
		actionsToPerform.reserve(MAX_ACTIONS_TO_CHECK);

		recursiveDFSActions(turnIdx, 0, 0, actionsToPerform);
	}
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
	resetForSimulation(SimulationType::BEST_ACTIONS);

	int surveillanceNodesDestroyed = 0;

	for (int roundIdx = turnIdx; roundIdx < roundsLeft; ++roundIdx) {
		int actionIdxToCheck = INVALID_IDX;
		for (int actionIdx : actionsToPerform) {
			if (roundIdx == actions[actionIdx].palcementRound) {
				actionIdxToCheck = actionIdx;
				break;
			}
		}

		surveillanceNodesDestroyed += bombsTick();

		if (INVALID_IDX != actionIdxToCheck) {
			const Action& actionToPerform = actions[actionIdxToCheck];

			if (couldPlaceBomb(actionToPerform)) {
				placeBomb(actionToPerform);
				actionsBestSequence[actionToPerform.palcementRound] = actionIdxToCheck;
			}
		}

		++solutionActionsCount;

		vector<Direction> directions(0);
		moveSNodes(directions, simulationGrid);

		if (surveillanceNodesDestroyed == sNodesCount) {
			solutionFound = true;
			break;
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::resetForSimulation(SimulationType simType) {
	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			Cell cellToReset = turnGrid[rowIdx][colIdx];

			if (SimulationType::S_NODES_DIRECTIONS == simType) {
				cellToReset = initialGrid[rowIdx][colIdx];
			}

			simulationGrid[rowIdx][colIdx] = cellToReset;
		}
	}

	for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
		sNodes[sNodeIdx].reset();
	}

	for (int bestActionIdx = 0; bestActionIdx < solutionActionsCount; ++bestActionIdx) {
		actionsBestSequence[bestActionIdx] = INVALID_IDX;
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
	
	bombs[bombsCount++].init(action.row, action.col, BOMB_ROUNDS_TO_EXPLODE);

	cell |= BOMB_FLAG;
}

//*************************************************************************************************************
//*************************************************************************************************************

int Grid::bombsTick() {
	int surveillanceNodesDestroyed = 0;

	// Go through all placed bombs, update their timers
	// If a bomb triggers another bomb the newly triggered one is added after the current, so the loop will work
	for (int bombIdx = 0; bombIdx < bombsCount; ++bombIdx) {
		if (bombs[bombIdx].update()) {
			surveillanceNodesDestroyed += bombs[bombIdx].explode(
				simulationGrid, height, width, sNodes, sNodesCount, bombs, bombsCount);
		}
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

void Grid::createSNode(int rowIdx, int colIdx, int turnIdx) {
	sNodes[sNodesCount++].init(rowIdx, colIdx, Direction::INVALID);
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::simulateAllRounds(int simulationStartRound) {
	resetForSimulation(SimulationType::BEST_ACTIONS);

	for (int roundIdx = simulationStartRound; roundIdx < roundsLeft; ++roundIdx) {
		vector<Direction> directions(0);
		moveSNodes(directions, simulationGrid);

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

void Grid::moveSNodes(const vector<Direction>& directions, Cell(*gridToUse)[MAX_WIDTH]) {
	for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
		moveSNode(sNodeIdx, directions, gridToUse);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::moveSNode(int sNodeIdx, const vector<Direction>& directions, Cell(*gridToUse)[MAX_WIDTH]) {
	SNode& sNode = sNodes[sNodeIdx];
	Direction sNodeDirection = sNode.getDirection();

	if (static_cast<int>(directions.size()) == sNodesCount) {
		sNodeDirection = directions[sNodeIdx];
	}

	if (Direction::INVALID != sNodeDirection && !sNode.hasFlag(DESTROYED_FLAG)) {
		const int sNodeRow = sNode.getRow();
		const int sNodeCol = sNode.getCol();

		Cell& currentCell = gridToUse[sNodeRow][sNodeCol];
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
			const Cell& cell = gridToUse[newRow][newCol];
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

		Cell& newCell = gridToUse[newRow][newCol];
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

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::claculateSNodesMovementDirections(int depth, const vector<Direction>& directionsToTest) {
	if (nodesMovementCalculated || depth > sNodesCount) {
		return;
	}

	if (depth == sNodesCount) {
		resetForSimulation(SimulationType::S_NODES_DIRECTIONS);
		moveSNodes(directionsToTest, simulationGrid);
		moveSNodes(directionsToTest, simulationGrid);
		nodesMovementCalculated = checkNodesInitialPositions();

		if (nodesMovementCalculated) {
			initNodesForTurn();
		}

		return;
	}

	const SNode& sNodeToCheck = sNodes[depth];

	for (int possDirIdx = 0; possDirIdx < sNodeToCheck.getPossibleDirectionsCount(); ++possDirIdx) {
		if (nodesMovementCalculated) {
			break;
		}

		Direction possibleDirection = sNodeToCheck.getPossibleDirection(possDirIdx);

		vector<Direction> newPossibleDirections = directionsToTest;
		newPossibleDirections.push_back(possibleDirection);

		claculateSNodesMovementDirections(depth + 1, newPossibleDirections);
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

bool Grid::checkNodesInitialPositions() const {
	bool sNodesPositionsCorrect = true;

	for (int rowIdx = 0; rowIdx < height; ++rowIdx) {
		for (int colIdx = 0; colIdx < width; ++colIdx) {
			const Cell turnCell = turnGrid[rowIdx][colIdx];
			const Cell simulationCell = simulationGrid[rowIdx][colIdx];

			const bool initialCellSNode = SURVEILLANCE_NODE & turnCell;
			const bool simulationCellSNode = SURVEILLANCE_NODE & simulationCell;

			if (initialCellSNode != simulationCellSNode) {
				sNodesPositionsCorrect = false;
				break;
			}
		}

		if (!sNodesPositionsCorrect) {
			break;
		}
	}

	return sNodesPositionsCorrect;
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::fillPossibleSNodesDirections() {
	for (int sNodeIdx = 0; sNodeIdx < sNodesCount; ++sNodeIdx) {
		SNode& sNode = sNodes[sNodeIdx];
		const int sNodeRow = sNode.getRow();
		const int sNodeCol = sNode.getCol();

		if (SURVEILLANCE_NODE & turnGrid[sNodeRow][sNodeCol]) {
			sNode.init(sNodeRow, sNodeCol, Direction::INVALID);
			sNode.addPossibleDirection(Direction::INVALID); // Add only one possible direction, its needed when determining all directions
		}
		else {
			for (const Direction direction : directions) {
				int rowNeighbour = sNodeRow;
				int colNeighbour = sNodeCol;

				rowNeighbour += MOVE_IN_ROWS[static_cast<int>(direction)];
				colNeighbour += MOVE_IN_COLS[static_cast<int>(direction)];

				if (rowNeighbour < 0 || rowNeighbour >= height || colNeighbour < 0 || colNeighbour >= width) {
					continue;
				}

				if (SURVEILLANCE_NODE & turnGrid[rowNeighbour][colNeighbour]) {
					const int rowDiff = sNodeRow - rowNeighbour;
					const int colDiff = sNodeCol - colNeighbour;

					sNode.addPossibleDirection(direction);
				}
			}

			if (1 == sNode.getPossibleDirectionsCount()) {
				sNode.init(sNodeRow, sNodeCol, sNode.getPossibleDirection(0));
			}
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Grid::initNodesForTurn() {
	for (int nodeIdx = 0; nodeIdx < sNodesCount; ++nodeIdx) {
		SNode& sNode = sNodes[nodeIdx];

		// SNode is moved for the current turn to get its right direction, so use this position
		sNode.init(sNode.getRow(), sNode.getCol(), sNode.getDirection());

		if (Direction::INVALID == sNode.getDirection()) {
			staticNode = true;
		}
	}
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

		turnBegin();
		makeTurn();
		turnEnd();

		if (1 == firewallGrid.getRoundsLeft()) {
			break;
		}

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

	if (SECOND_TURN == turnsCount) {
		firewallGrid.fillPossibleSNodesDirections();
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::turnBegin() {
	if (turnsCount == THIRD_TURN) {
		vector<Direction> directions;
		firewallGrid.claculateSNodesMovementDirections(0, directions);

		// The nodes movement is calculated, proceed with simulation
		// The goal of the simulation is to find empty cell with the most nodes in range for bombs
		// May be every time when updating node mark all empty cells which have access to it
		// And at the end of each round simulation perform only one 2d traversal of the grid to find the best empty cells
		firewallGrid.simulateAllRounds(turnsCount);

		if (!firewallGrid.getSolutionFound()) {
			firewallGrid.sortActions();

			// Test dfs with unlimitted time, maybe it's not nice place to use dfs here
			firewallGrid.dfsActions(turnsCount);
		}
	}
}

//*************************************************************************************************************
//*************************************************************************************************************

void Game::makeTurn() {
	bool wait = false;

	if (firewallGrid.getSolutionFound()) {
		const int solutionActionIdx = firewallGrid.getSolutionActionIdx(turnsCount);

		if (INVALID_IDX == solutionActionIdx || solutionActionIdx < 0 || solutionActionIdx >= firewallGrid.getActionsCount()) {
			wait = true;
		}
		else {
			const Action& action = firewallGrid.getAction(solutionActionIdx);
			cout << action;
		}
	}
	else {
		wait = true;
	}

	if (wait) {
#ifdef REDIRECT_COUT_TO_FILE
		cout << "\"" << WAIT << "\"," << endl;
#else
		cout << WAIT << endl;
#endif
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
