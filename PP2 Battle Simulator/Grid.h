#pragma once

#include "precomp.h"
#include "tank.h"
#include <vector>


#define GRID_SIZE 80
#define GRID_OFFSET 8

namespace Tmpl8
{
    class Grid
    {
    public:
        static Grid* Instance();
        ~Grid();
        void AddTankToGridCell(Tmpl8::Tank* tank);
        static vec2<int> GetGridCell(const vec2<>& position);
        void MoveTankToGridCell(Tank* tank, const vec2<int>& newPos);
        static vector<vec2<int>> GetNeighbouringCells();

        vector<Tank*> grid[GRID_SIZE + 1][GRID_SIZE + 1];

    private:
        /* Here will be the instance stored. */
        static Grid* instance;

        /* Private constructor to prevent instancing. */
        Grid();
    };
} // namespace PP2