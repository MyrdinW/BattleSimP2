#include "precomp.h"
#include "Grid.h"
#include "defines.h"
#include <iostream>
#include <mutex>

using namespace std;
using namespace Tmpl8;

Grid* Grid::instance = nullptr;
mutex gridlock;

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

#define CLAP_POS(_IN_) clamp((0.05f * _IN_) * (GRID_SIZE / 100.f), -(float)GRID_OFFSET, (float)GRID_SIZE - GRID_OFFSET) + GRID_OFFSET
vec2 Grid::GetGridCell(const vec2& position)
{
    return vec2(CLAP_POS(position.x), CLAP_POS(position.y));
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

void Grid::AddTankToGridCell(Tank * tank) { grid[(int)tank->gridCell.x][(int)tank->gridCell.y].emplace_back(tank); }

void Grid::MoveTankToGridCell(Tmpl8::Tank * tank, const vec2&newPos)
{
    scoped_lock lock(gridlock);
    auto& gridCell = grid[(int)tank->gridCell.x][(int)tank->gridCell.y];
    grid[(int)newPos.x][(int)newPos.y].emplace_back(tank);
    for (int i = 0; i < gridCell.size(); ++i)
    {
        if (gridCell[i] == tank)
        {
            gridCell.erase(gridCell.begin() + i);
            break;
        }
    }
}