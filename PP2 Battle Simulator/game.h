#pragma once

#include "algorithms.h"

namespace Tmpl8
{
	//forward declarations
	class Tank;
	class Rocket;
	class Smoke;
	class Particle_beam;

	class Game
	{
	public:
		void set_target(Surface* surface) { screen = surface; }
		void init();
		void shutdown();
		void update(float deltaTime);
		void draw();
		void initKD();
		void DrawTankHP();
		void tick(float deltaTime);
		void measure_performance();

		Tank& find_closest_enemy(Tank& current_tank);

		void mouse_up(int button)
		{ /* implement if you want to detect mouse button presses */
		}

		void mouse_down(int button)
		{ /* implement if you want to detect mouse button presses */
		}

		void mouse_move(int x, int y)
		{ /* implement if you want to detect mouse movement */
		}

		void key_up(int key)
		{ /* implement if you want to handle keys */
		}

		void key_down(int key)
		{ /* implement if you want to handle keys */
		}

	private:
		Surface* screen;
		vector<Tank> tanks;
		vector<Tank*> blueTanks;
		vector<Tank*> redTanks;
		vector<Rocket> rockets;
		vector<Smoke> smokes;
		vector<Explosion> explosions;
		vector<Particle_beam> particle_beams;

		KDTree* redteamKD;
		KDTree* blueteamKD;

		Font* frame_count_font;
		long long frame_count = 0;

		bool lock_update = false;

		void updateSmoke();
		void updateParticlebeams();

		void updateExplosions();

		void updateTanks();

		void updateRockets();



	};

}; // namespace Tmpl8