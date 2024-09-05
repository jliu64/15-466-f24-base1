#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//ball position:
	glm::vec2 ball_at = glm::vec2(0.0f);

	//ball velocity:
	uint8_t ball_up = 1;
	uint8_t ball_right = 1;

	struct Enemy {
		uint8_t index = 0; //index in sprites array
		uint8_t active = 1; //flag for active enemy
		glm::vec2 position = glm::vec2(0.0f);
		uint8_t initial_x = 0; //initial x position
		uint8_t moving_right = 1; //flag for moving right
		float time_since_fired = 0.0f; //time elapsed since last shot
	};
	std::array<Enemy, 10> enemies;

	struct Shot {
		uint8_t index = 0; //index in sprites array
		uint8_t active = 1; //flag for active shot from enemy
		glm::vec2 position = glm::vec2(0.0f);
	};
	std::array<Shot, 10> shots;

	uint8_t game_over = 0;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
