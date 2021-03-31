#pragma once

#include "defines.h"
#include "tank.h"
#include <vector>
#include "template.h"

namespace Tmpl8
{
	class Grid
	{
	public:
		static Grid* Instance();
		~Grid();
		void AddTankToGridCell(Tank* tank);
		static vec2 GetGridCell(const vec2& position);
		void MoveTankToGridCell(Tank* tank, const vec2& newPos);
		static std::vector<vec2> GetNeighbouringCells();

		std::vector<Tank*> grid[GRID_SIZE + 1][GRID_SIZE + 1];

	private:
		/* Here will be the instance stored. */
		static Grid* instance;

		/* Private constructor to prevent instancing. */
		Grid();
	};
}
