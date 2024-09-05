#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

PlayMode::PlayMode() {
	// Read background, tile, and palette assets from binary files
	// Based on code shared by Jude Markabawi (jmarkaba@andrew.cmu.edu) in game1-sprite-based on the class Discord
	std::string bg_filename = "output_of_asset_gen/background.bin";
	std::ifstream bg_file(bg_filename, std::ios_base::binary);
	bg_file.read(reinterpret_cast<char*> (ppu.background.data()), sizeof(ppu.background));

	std::string pt_filename = "output_of_asset_gen/palette_table.bin";
	std::ifstream pt_file(pt_filename, std::ios_base::binary);
	pt_file.read(reinterpret_cast<char*> (ppu.palette_table.data()), sizeof(ppu.palette_table));

	std::string tt_filename = "output_of_asset_gen/tile_table.bin";
	std::ifstream tt_file(tt_filename, std::ios_base::binary);
	tt_file.read(reinterpret_cast<char*> (ppu.tile_table.data()), sizeof(ppu.tile_table));

	// Initialize enemy array
	// TODO: Implement randomness between enemies in movement and firing times?
	for (int i = 0; i < 10; i++) {
		enemies[i].index = int8_t(i * 4 + 6); // Enemy sprite indices start at 6, increment by 4 since they require 4 sprites (3 + shot)
		enemies[i].active = 1;
		enemies[i].position.x = (i % 5) * 40.0f + 16.0f;
		enemies[i].initial_x = int8_t((i % 5) * 40 + 16);
		if (i >= 5) {
			enemies[i].position.y = 232.0f;
		} else {
			enemies[i].position.y = 216.0f;
		}
		enemies[i].time_since_fired = i * 0.8f; // Fire every 10 seconds, with varying initial timing based on position
	}

	// Initialize shot array
	for (int i = 0; i < 10; i++) {
		shots[i].index = int8_t(i * 4 + 9); // Shot indices immediately follow their source enemy indices, starting at 9
		shots[i].active = 0;
		shots[i].position.x = 0.0f;
		shots[i].position.y = 240.0f;
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_ESCAPE) { // Exit with the escape key
			exit(0);
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (game_over) {
		return;
	}

	constexpr float PlayerSpeed = 100.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;

	constexpr float BallSpeed = 100.0f;
	if (ball_up) {
		ball_at.y += BallSpeed * elapsed;
	} else {
		ball_at.y -= BallSpeed * elapsed;
	}

	if (ball_right) {
		ball_at.x += BallSpeed * elapsed;
	} else {
		ball_at.x -= BallSpeed * elapsed;
	}

	// Check for ball collusion with enemies, and move enemies
	uint8_t enemies_exist = 0;
	constexpr float EnemySpeed = 50.0f;
	for (int i = 0; i < 10; i++) {
		if (enemies[i].active) {
			enemies_exist = 1;
			if (enemies[i].moving_right) {
				enemies[i].position.x += EnemySpeed * elapsed;
			} else {
				enemies[i].position.x -= EnemySpeed * elapsed;
			}
		}

		if (ball_up && enemies[i].active && ball_at.y >= enemies[i].position.y && ball_at.x >= (enemies[i].position.x - 8) && ball_at.x < (enemies[i].position.x + 16)) {
			enemies[i].active = false;
			enemies[i].position.x = 0.0f;
			enemies[i].position.y = 240.0f;
			ball_up = ! ball_up;
		}

		// Change direction if gone too far
		if ((enemies[i].active && enemies[i].moving_right && (enemies[i].position.x - enemies[i].initial_x >= 64.0f))
			|| ((! enemies[i].moving_right) && (enemies[i].position.x < enemies[i].initial_x))) {
			enemies[i].moving_right = ! enemies[i].moving_right;
		}

		// Fire shots
		if (enemies[i].active) {
			if (enemies[i].time_since_fired > 10.0f) {
				shots[i].active = true;
				shots[i].position.x = enemies[i].position.x;
				shots[i].position.y = enemies[i].position.y;
				enemies[i].time_since_fired = 0.0f;
			} else {
				enemies[i].time_since_fired += elapsed;
			}
		}
	}

	if (! enemies_exist) {
		std::cout << "You win!" << std::endl;
		game_over = true;
	}

	// Check for ball collision with floor
	if ((! ball_up) && ball_at.y <= 0 && (ball_at.x >= (player_at.x + 24) || ball_at.x < (player_at.x - 16))) {
		std::cout << "Game over!" << std::endl;
		game_over = true;
	}
	
	// Check for ball collision with player or ceiling
	if (((! ball_up) && ball_at.y <= player_at.y && ball_at.x >= (player_at.x - 16) && ball_at.x < (player_at.x + 24))
		|| ball_up && ball_at.y >= 240) {
		ball_up = ! ball_up;
	}

	// Check for ball collision with wall
	if ((ball_right && ball_at.x >= 256) || ((! ball_right) && ball_at.x <= 0)) {
		ball_right = ! ball_right;
	}

	// Move enemy shots and check for shot collision with player
	constexpr float ShotSpeed = 150.0f;
	for (int i = 0; i < 10; i++) {
		if (shots[i].active) {
			shots[i].position.y -= ShotSpeed * elapsed;
		}

		if (shots[i].active && shots[i].position.y <= player_at.y && shots[i].position.x >= (player_at.x - 16) && shots[i].position.x < (player_at.x + 24)) {
			std::cout << "Game over!" << std::endl;
			game_over = true;
		}

		if (shots[i].active && shots[i].position.y <= 0.0f) {
			shots[i].active = false;
			shots[i].position.x = 0.0f;
			shots[i].position.y = 240.0f;
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will just be black
	ppu.background_color = glm::u8vec4(0xff, 0xff, 0xff, 0xff);

	// Background scroll is disabled; comments are kept for my own clarity
	// ppu.background_position.x = int32_t(-0.5f * player_at.x);
	// ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 3;
	ppu.sprites[0].attributes = 0;

	//player left wing
	ppu.sprites[1].x = int8_t(player_at.x - 8);
	ppu.sprites[1].y = int8_t(player_at.y);
	ppu.sprites[1].index = 2;
	ppu.sprites[1].attributes = 0;
	ppu.sprites[2].x = int8_t(player_at.x - 16);
	ppu.sprites[2].y = int8_t(player_at.y);
	ppu.sprites[2].index = 1;
	ppu.sprites[2].attributes = 0;

	//player right wing
	ppu.sprites[3].x = int8_t(player_at.x + 8);
	ppu.sprites[3].y = int8_t(player_at.y);
	ppu.sprites[3].index = 2;
	ppu.sprites[3].attributes = 0;
	ppu.sprites[4].x = int8_t(player_at.x + 16);
	ppu.sprites[4].y = int8_t(player_at.y);
	ppu.sprites[4].index = 4;
	ppu.sprites[4].attributes = 0;

	//ball
	ppu.sprites[5].x = int8_t(ball_at.x);
	ppu.sprites[5].y = int8_t(ball_at.y);
	ppu.sprites[5].index = 5;
	ppu.sprites[5].attributes = 0;

	for (int i = 0; i < 10; i++) {
		//draw enemies
		ppu.sprites[enemies[i].index].x = int8_t(enemies[i].position.x);
		ppu.sprites[enemies[i].index].y = int8_t(enemies[i].position.y);
		ppu.sprites[enemies[i].index].index = 7;
		ppu.sprites[enemies[i].index].attributes = 1;

		//draw enemy wings
		ppu.sprites[enemies[i].index + 1].x = int8_t(enemies[i].position.x - 8);
		ppu.sprites[enemies[i].index + 1].y = int8_t(enemies[i].position.y);
		ppu.sprites[enemies[i].index + 1].index = 6;
		ppu.sprites[enemies[i].index + 1].attributes = 1;
		ppu.sprites[enemies[i].index + 2].x = int8_t(enemies[i].position.x + 8);
		ppu.sprites[enemies[i].index + 2].y = int8_t(enemies[i].position.y);
		ppu.sprites[enemies[i].index + 2].index = 6;
		ppu.sprites[enemies[i].index + 2].attributes = 1;

		//draw shots
		ppu.sprites[shots[i].index].x = int8_t(shots[i].position.x);
		ppu.sprites[shots[i].index].y = int8_t(shots[i].position.y);
		ppu.sprites[shots[i].index].index = 8;
		ppu.sprites[shots[i].index].attributes = 1;
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
