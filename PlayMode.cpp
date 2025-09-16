#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>

GLuint duck_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > duck_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("duck.pnct"));
	duck_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	
	return ret;
});

Load< Scene > duck_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("duck.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = duck_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = duck_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > duck_music_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("duck_init.wav"));
});


PlayMode::PlayMode() : scene(*duck_scene), rhythm(
	/*bpm*/ 120.0f,
	/*pattern*/ std::vector<Rhythm::Measure>{
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
		Rhythm::Measure{ true, false, false },
},
     /*hit window*/ 0.12f

) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		//std::cout << transform.name << std::endl;
		if (transform.name == "Ground") ground = &transform;
		else if (transform.name == "Duck") duck = &transform;
		else if (transform.name == "LeftFoot") left_foot = &transform;
		else if (transform.name == "RightFoot") right_foot = &transform;
	}
	if (duck == nullptr) throw std::runtime_error("Duck body not found.");
	if (left_foot == nullptr) throw std::runtime_error("Left foot not found.");
	if (right_foot == nullptr) throw std::runtime_error("Right foot not found.");

	right_foot_base_rotation = right_foot->rotation;
	left_foot_base_rotation = left_foot->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	rhythm.reset();
	rhythm.start();
	duck_music_loop = Sound::loop(*duck_music_sample, 0.6f, 0.0f);
	
	{ // check sample
		const float spb = rhythm.seconds_per_beat();
		const float loop_s = rhythm.loop_duration_sec();
		std::cout << "[Start] BPM=" << rhythm.get_bpm()
			<< " SPB=" << spb << "s"
			<< " LoopLen=" << loop_s << "s"
			<< " PatternMeasures=" << rhythm.pattern.size()
			<< "\n";
		const float music_seconds = duck_music_sample->data.size() / 48000.0f;
		std::cout << "[Start] Music duration (file) is around " << music_seconds << "s\n";
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} 
		else if (evt.key.key == SDLK_SPACE) {
			if (!space.pressed) {
				space.downs += 1;
				space.pressed = true;
				std::cout << "space is pressed" << std::endl;
			}
			
			return true;
			//if (honk_oneshot) honk_oneshot->stop();
			//honk_oneshot = Sound::play_3D(*honk_sample, 0.3f, glm::vec3(4.6f, -7.8f, 6.9f)); //hardcoded position of front of car, from blender
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::duck_take_step(float dt) {
	if (tilt_time <= 0.0f) return;

	tilt_time = std::max(0.0f, tilt_time - dt);
	float t = 1.0f - (tilt_time / tilt_duration);

	float lift = std::sin(glm::pi<float>() * t);
	float angle = glm::radians(30.0f) * lift;

	float forward = -step_distance * (dt / tilt_duration);

	duck->position += glm::vec3(0.0f, forward, 0.0f);
	//std::cout << "duck position is " << duck->position.y << std::endl;

	if (left_foot_step) {
		left_foot->rotation = left_foot_base_rotation * glm::angleAxis(angle, glm::vec3(-1, 0, 0));
		right_foot->rotation = right_foot_base_rotation;
	}
	else {
		right_foot->rotation = right_foot_base_rotation * glm::angleAxis(angle, glm::vec3(-1, 0, 0));
		//std::cout << "duck rfoot rotation is " << right_foot->rotation.y << std::endl;
		left_foot->rotation = left_foot_base_rotation;
	}

	if (tilt_time <= 0.0f) {
		left_foot->rotation = left_foot_base_rotation;
		right_foot->rotation = right_foot_base_rotation;
		left_foot_step = !left_foot_step;
	}
}

void PlayMode::update(float elapsed) {
	rhythm.update(elapsed);
	uint8_t presses = space.downs;
	//std::cout << "num presses: " << int(presses) << std::endl;
	space.downs = 0;

	for (uint8_t i = 0; i < presses; ++i) {
		Rhythm::HitResult hr = rhythm.register_tap();
		std::cout << "measure " << hr.measure_idx << std::endl;

		if (tilt_time <= 0.0f) {
			tilt_time = tilt_duration;
		}

	}
	duck_take_step(elapsed);

	if (rhythm.all_strong_beats_hit()) {
		std::cout << "[PlayMode] finished perfect!!!" << std::endl;
	}

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Tap space to make Duckie step on the strong beats",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Tap space to make Duckie step on the strong beats",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

