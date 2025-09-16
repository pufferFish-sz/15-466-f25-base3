#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Rhythm.hpp"

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
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform* ground = nullptr;
	Scene::Transform* duck = nullptr;
	Scene::Transform* left_foot = nullptr;
	Scene::Transform* right_foot = nullptr;

	glm::quat right_foot_base_rotation;
	glm::quat left_foot_base_rotation;
	bool left_foot_step = false;

	void duck_take_step(float dt);
	// distance that duckie steps each step
	float step_distance = 0.5f;

	std::shared_ptr<Sound::PlayingSample> duck_music_loop;
	Rhythm rhythm;

	//car honk sound:
	//std::shared_ptr< Sound::PlayingSample > honk_oneshot;
	
	//camera:
	Scene::Camera *camera = nullptr;

private:
	float tilt_time = 0.0f;
	float tilt_duration = rhythm.seconds_per_beat();

};
