#include "precomp.h" // include (only) this in every .cpp file
#include "defines.h"





//Global performance timer
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* background_img = new Surface("assets/Background_Grass.png");
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite background(background_img, 1);
static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(14, 18);
const static vec2 rocket_size(25, 24);

const static float tank_radius = 8.5f; // was 8.5
const static float rocket_radius = 10.f;

vector<int> redHealthBars = {};
vector<int> blueHealthBars = {};

int thread_amount = thread::hardware_concurrency();
ThreadPool thread_pool(thread_amount);
mutex tankVectorLock;



// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::init()
{

	auto instance = Grid::Instance();

	frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

	tanks.reserve(NUM_TANKS_BLUE + NUM_TANKS_RED);

	uint rows = (uint)sqrt(NUM_TANKS_BLUE + NUM_TANKS_RED);
	uint max_rows = 12;

	float start_blue_x = tank_size.x + 10.0f;
	float start_blue_y = tank_size.y + 80.0f;

	float start_red_x = 980.0f;
	float start_red_y = 100.0f;

	float spacing = 15.0f;

	//Spawn blue tanks
	for (int i = 0; i < NUM_TANKS_BLUE; i++)
	{
		tanks.emplace_back(Tank(start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing), BLUE, &tank_blue, &smoke, 1200, 600, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
	}
	//Spawn red tanks
	for (int i = 0; i < NUM_TANKS_RED; i++)
	{
		tanks.emplace_back(Tank(start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing), RED, &tank_red, &smoke, 80, 80, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
	}

	particle_beams.emplace_back(Particle_beam(vec2(SCRWIDTH / 2, SCRHEIGHT / 2), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
	particle_beams.emplace_back(Particle_beam(vec2(80, 80), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
	particle_beams.emplace_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));

	for (auto& tank : tanks)
	{
		instance->AddTankToGridCell(&tank);
		if (tank.allignment == RED)
			redTanks.emplace_back(&tank);
		else
			blueTanks.emplace_back(&tank);
	}
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown()
{
}

// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank& Game::find_closest_enemy(Tank& current_tank)
{
	float closest_distance = numeric_limits<float>::infinity();
	int closest_index = 0;

	for (int i = 0; i < tanks.size(); i++)
	{
		if (tanks.at(i).allignment != current_tank.allignment && tanks.at(i).active)
		{
			float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
			if (sqr_dist < closest_distance)
			{
				closest_distance = sqr_dist;
				closest_index = i;
			}
		}
	}

	return tanks.at(closest_index);
}

// -----------------------------------------------------------
// Update the game state:
// Move all objects
// Update sprite frames
// Collision detection
// Targeting etc..
// -----------------------------------------------------------
void Game::update(float deltaTime)
{
	if (frame_count % 200 == 0)
	{
		initKD();
	}


	updateSmoke();

	updateParticlebeams();

	updateExplosions();
	//Remove explosions with remove erase idiom
	explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) { return explosion.done(); }), explosions.end());

	updateRockets();
	//Remove exploded rockets with remove erase idiom
	rockets.erase(std::remove_if(rockets.begin(), rockets.end(), [](const Rocket& rocket) { return !rocket.active; }), rockets.end());



	updateTanks();


	redHealthBars = CountSort(redTanks);

	blueHealthBars = CountSort(blueTanks);


}

void Game::initKD()
{
	redteamKD = new KDTree(redTanks);
	blueteamKD = new KDTree(blueTanks);

}
void Game::updateParticlebeams()
{
	//Update particle beams
	for (Particle_beam& particle_beam : particle_beams)
	{
		particle_beam.tick(tanks);

		//Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding box)
		for (Tank& tank : tanks)
		{
			if (tank.active && particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
			{
				if (tank.hit(particle_beam.damage))
				{
					smokes.emplace_back(Smoke(smoke, tank.position - vec2(0, 48)));
				}
			}
		}
	}
}
void Game::updateSmoke()
{
	//Update smoke plumes
	for (Smoke& smoke : smokes)
	{
		smoke.tick();
	}
}

void Game::updateTanks()
{

	int tanksperthread = tanks.size() / thread_amount;
	int remain = tanks.size();
	int start = 0;
	int end = start + tanksperthread;
	int remaining = tanks.size() % thread_amount;
	int currently_remaining = remaining;




	for (int j = 1; j < thread_amount; j++)
	{
		//als er tanks overblijven na het delen.
		if (currently_remaining > 0)
		{
			end++;
			currently_remaining--;
		}

		std::future<void> fut = thread_pool.enqueue([&, start, end] {
			for (int i = start; i < end; i++)
			{
				Tank& tank = tanks.at(i);
				if (!tank.active) continue;

				for (const auto& cell : Grid::GetNeighbouringCells())
				{
					int x = tank.gridCell.x + cell.x;
					int y = tank.gridCell.y + cell.y;
					if (x < 0 || y < 0 || x > GRID_SIZE || y > GRID_SIZE) continue;

					for (auto& oTank : Grid::Instance()->grid[x][y])
					{
						if (&tank == oTank) continue;

						vec2 dir = tank.get_position() - oTank->get_position();

						float colSquaredLen =
							(tank.get_collision_radius() * tank.get_collision_radius()) +
							(oTank->get_collision_radius() * oTank->get_collision_radius());

						if (dir.sqr_length() < colSquaredLen) tank.push(dir.normalized(), 1.f);
					}
				}

				//Check if inside particle beam
				for (Particle_beam& particle_beam : particle_beams)
				{
					if (particle_beam.rectangle.intersects_circle(tank.get_position(),
						tank.get_collision_radius()))
					{
						if (tank.hit(particle_beam.damage))
						{
							smokes.emplace_back(smoke, tank.position - vec2(0, 48));
						}
					}
				}

				//Shoot at closest target if reloaded
				if (!tank.rocket_reloaded()) continue;

				Tank* target = tank.allignment == RED ? blueteamKD->find_closest_enemy(&tank) : redteamKD->find_closest_enemy(&tank);
				scoped_lock lock(tankVectorLock);
				rockets.emplace_back(tank.position, (target->position - tank.position).normalized() * 3, rocket_radius, tank.allignment, ((tank.allignment == RED) ? &rocket_red : &rocket_blue));
				tank.reload_rocket();
			}

			});

		start = end;

		if (end / j != tanksperthread) {
			end += tanksperthread + currently_remaining;
		}
		else {
			end += tanksperthread;
		}


	}




	for (int k = 0; k < tanks.size(); k++)
	{
		Tank& tank = tanks.at(k);
		if (!tank.active) continue;

		//Move tanks according to speed and nudges (see above) also reload
		tank.tick();

	}

}


void Game::updateRockets()
{

	for (int i = 0; i < rockets.size(); i++) {
		Rocket& nRocket = rockets[i];
		nRocket.tick();

		if (nRocket.position.x < -250 || nRocket.position.y < -250 || nRocket.position.x > 1750 || nRocket.position.y > 1750)
		{
			nRocket.active = false;
			continue;
		}

		for (const auto& cell : Grid::GetNeighbouringCells())
		{
			vec2 rocketGridCell = Grid::GetGridCell(nRocket.position);
			int x = rocketGridCell.x + cell.x;
			int y = rocketGridCell.y + cell.y;
			if (x < 0 || y < 0 || x > GRID_SIZE || y > GRID_SIZE) continue;

			for (auto& tank : Grid::Instance()->grid[x][y])
			{
				if (tank->active && (tank->allignment != nRocket.allignment) &&
					nRocket.intersects(tank->position, tank->collision_radius))
				{
					scoped_lock lock(tankVectorLock);
					explosions.emplace_back(&explosion, tank->position);

					if (tank->hit(ROCKET_HIT_VALUE))
					{
						smokes.emplace_back(smoke, tank->position - vec2(0, 48));
					}

					nRocket.active = false;
					break;
				}
			}
		}
	}



}

void Game::updateExplosions()
{
	//Update explosion sprites and remove when done with remove erase idiom
	for (Explosion& explosion : explosions)
	{
		explosion.tick();
	}
}

void Game::draw()
{
	// clear the graphics window
	screen->clear(0);

	//Draw background
	background.draw(screen, 0, 0);

	//Draw sprites
	for (int i = 0; i < NUM_TANKS_BLUE + NUM_TANKS_RED; i++)
	{
		tanks.at(i).draw(screen);

		vec2 tank_pos = tanks.at(i).get_position();
		// tread marks
		if ((tank_pos.x >= 0) && (tank_pos.x < SCRWIDTH) && (tank_pos.y >= 0) && (tank_pos.y < SCRHEIGHT))
			background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH] = sub_blend(background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH], 0x808080);
	}

	for (Rocket& rocket : rockets)
	{
		rocket.draw(screen);
	}

	for (Smoke& smoke : smokes)
	{
		smoke.draw(screen);
	}

	for (Particle_beam& particle_beam : particle_beams)
	{
		particle_beam.draw(screen);
	}

	for (Explosion& explosion : explosions)
	{
		explosion.draw(screen);
	}

	DrawTankHP();

}

void Game::DrawTankHP()
{

	for (int i = 0; i < NUM_TANKS_BLUE; i++)
	{
		int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
		int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
		int health_bar_start_y = 0;
		int health_bar_end_y = HEALTH_BAR_HEIGHT;

		screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
		screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - (blueHealthBars[i] / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
	}

	for (int j = 0; j < NUM_TANKS_RED; j++)
	{
		int health_bar_start_x = j * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
		int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
		int health_bar_start_y = (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
		int health_bar_end_y = SCRHEIGHT - 1;

		screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
		screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - (redHealthBars[j] / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
	}
}


// -----------------------------------------------------------
// When we reach MAX_FRAMES print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance()
{
	char buffer[128];
	if (frame_count >= MAX_FRAMES)
	{
		if (!lock_update)
		{
			duration = perf_timer.elapsed();
			cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
			lock_update = true;
		}

		frame_count--;
	}

	if (lock_update)
	{
		screen->bar(420, 170, 870, 430, 0x030000);
		int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
		sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
		frame_count_font->centre(screen, buffer, 200);
		sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
		frame_count_font->centre(screen, buffer, 340);
	}
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime)
{
	if (!lock_update)
	{
		update(deltaTime);
	}
	draw();

	measure_performance();

	// print something in the graphics window
	//screen->Print("hello world", 2, 2, 0xffffff);

	// print something to the text window
	//cout << "This goes to the console window." << std::endl;

	//Print frame count
	frame_count++;
	string frame_count_string = "FRAME: " + std::to_string(frame_count);
	frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}
