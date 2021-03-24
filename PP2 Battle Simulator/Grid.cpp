#include "Grid.h"
#include "defines.h"
#include <iostream>
#include <mutex>

using namespace std;
using namespace Tmpl8;

Grid* Grid::instance = nullptr;
mutex mtx2;

Grid::Grid()
{
    for (auto& x : grid)
        for (auto& y : x) y.reserve(500);
}

Grid::~Grid() = default;

Grid* Grid::Instance()
{
    if (instance == nullptr) instance = new Grid();
    return instance;
}


vector<vec2> Grid::GetNeighbouringCells()
{
    vector<vec2> cells = {
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},
        {1, -1},
        {0, -1},
        {-1, -1},
        {-1, 0},
        {-1, 1} };
    return cells;
}

void Grid::AddTankToGridCell(Tank * tank) { grid[tank->gridCell.x][tank->gridCell.y].emplace_back(tank); }

void Grid::MoveTankToGridCell(Tmpl8::Tank * tank, const vec2&newPos)
{
    //scoped_lock lock(mtx2);
    auto& gridCell = grid[tank->gridCell.x][tank->gridCell.y];
    grid[newPos.x][newPos.y].emplace_back(tank);
    for (int i = 0; i < gridCell.size(); ++i)
    {
        if (gridCell[i] == tank)
        {
            gridCell.erase(gridCell.begin() + i);
            break;
        }
    }
}