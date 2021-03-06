#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "Sound.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("phone-bank.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
	});

Load< std::map<std::string, Sound::Sample>> sound_samples(LoadTagDefault, []() -> std::map<std::string, Sound::Sample> const* {
	auto map_p = new std::map<std::string, Sound::Sample>();
	std::string base_dir = data_path("musics");

#if defined(_WIN32)
	for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
		std::string path_string = entry.path().string();
		std::string file_name = entry.path().filename().string();
#else
	struct dirent* entry;
	DIR* dp;

	dp = opendir(&base_dir[0]);
	if (dp == nullptr) {
		std::cout << "Cannot open " << base_dir << "\n";
		throw std::runtime_error("Cannot open dir");
	}
	while ((entry = readdir(dp))) {
		std::string path_string = base_dir + "/" + std::string(entry->d_name);
		std::string file_name = std::string(entry->d_name);
#endif
		size_t start = 0;
		size_t end = file_name.find(".opus");

		if (end != std::string::npos) {
			map_p->emplace(file_name.substr(start, end), Sound::Sample(path_string));
		}
	}
	return map_p;
	});

Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("phone-bank.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		Mesh const& mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable& drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		});
	});

WalkMesh const* walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const* {
	WalkMeshes* ret = new WalkMeshes(data_path("phone-bank.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
	});

void PlayMode::spawn_goods() {
	Mesh const& goods_mesh = phonebank_meshes->lookup("Goods." + std::to_string(((color + 1)/10) %10) + std::to_string(((color + 1) / 100) % 10) +  std::to_string((color + 1)));
	goods_drawable->pipeline.type = goods_mesh.type;
	goods_drawable->pipeline.start = goods_mesh.start;
	goods_drawable->pipeline.count = goods_mesh.count;
	goods_drawable->transform->position = spawn_position;
	is_picked_up = false;
}
void PlayMode::switch_foot() {
	Sound::play((*sound_samples).at("walk"));
	if (is_prev_left && !is_picked_up ) {
		Mesh const& player_mesh = phonebank_meshes->lookup("Player_left");
		player_drawable->pipeline.type = player_mesh.type;
		player_drawable->pipeline.start = player_mesh.start;
		player_drawable->pipeline.count = player_mesh.count;
		player_drawable->transform = player.transform;
	}
	else if (!is_prev_left && !is_picked_up) {
		Mesh const& player_mesh = phonebank_meshes->lookup("Player_right");
		player_drawable->pipeline.type = player_mesh.type;
		player_drawable->pipeline.start = player_mesh.start;
		player_drawable->pipeline.count = player_mesh.count;
		player_drawable->transform = player.transform;
	}
	else if (is_prev_left && is_picked_up) {
		Mesh const& player_mesh = phonebank_meshes->lookup("Player_left_hand_up");
		player_drawable->pipeline.type = player_mesh.type;
		player_drawable->pipeline.start = player_mesh.start;
		player_drawable->pipeline.count = player_mesh.count;
		player_drawable->transform = player.transform;
	}
	else if (!is_prev_left && is_picked_up) {
		Mesh const& player_mesh = phonebank_meshes->lookup("Player_right_hand_up");
		player_drawable->pipeline.type = player_mesh.type;
		player_drawable->pipeline.start = player_mesh.start;
		player_drawable->pipeline.count = player_mesh.count;
		player_drawable->transform = player.transform;
	}
}

PlayMode::PlayMode() : scene(*phonebank_scene) {
	std::string const destination_prefix = "Destination";
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player_right") {
			player.transform = &transform;
			player_start_position = glm::vec3(player.transform->position.x, player.transform->position.y, player.transform->position.z);
			player_start_rotation = glm::quat(player.transform->rotation.x, player.transform->rotation.y, player.transform->rotation.z, player.transform->rotation.w);
		}
		if (transform.name.find(destination_prefix) == 0) {
			destinations.push_back(&transform);
		}
		if (transform.name == "Pointer") {
			pointer_transform = &transform;
		}
	}
	for (auto& transform : scene.transforms) {
		if (transform.name.find(destination_prefix) == 0) {
			destinations[std::stoi(transform.name.substr(transform.name.length() - 3)) - 1] = &transform;
		}
	}
	for (player_drawable = scene.drawables.begin(); player_drawable != scene.drawables.end(); player_drawable++) {
		if (player_drawable->transform->name == "Player_left") {
			break;
		}
	}
	scene.drawables.erase(player_drawable);
	for (player_drawable = scene.drawables.begin(); player_drawable != scene.drawables.end(); player_drawable++) {
		if (player_drawable->transform->name == "Player_right") {
			break;
		}
	}
	scene.drawables.erase(player_drawable);
	for (player_drawable = scene.drawables.begin(); player_drawable != scene.drawables.end(); player_drawable++) {
		if (player_drawable->transform->name == "Player_left_hand_up") {
			break;
		}
	}
	scene.drawables.erase(player_drawable);
	for (player_drawable = scene.drawables.begin(); player_drawable != scene.drawables.end(); player_drawable++) {
		if (player_drawable->transform->name == "Player_right_hand_up") {
			break;
		}
	}
	std::string const goods_prefix = "Goods";
	std::list<Scene::Drawable>::iterator it;
	for (it = scene.drawables.begin(); it != scene.drawables.end(); it++) {
		if (it->transform->name == "Goods.001") {
			goods_drawable = it;
			break;
		}
	}
	std::cout << goods_drawable->transform->name << std::endl;
	goods_drawable->transform->scale *= 4.0f;
	bool found = true;
	while (found) {
		found = false;
		for (it = scene.drawables.begin(); it != scene.drawables.end(); it++) {
			if (it->transform->name.find(goods_prefix) == 0 && it->transform->name != "Goods.001") {
				scene.drawables.erase(it);
				found = true;
				break;
			}
		}
	}
	
	if (player.transform == nullptr) throw std::runtime_error("player not found.");
	if (goods_drawable == scene.drawables.end()) throw std::runtime_error("goods not found.");
	if (pointer_transform == nullptr) throw std::runtime_error("pointer not found.");
	if (destinations.size() == 0) throw std::runtime_error("destinations not found.");

	//create a player transform:
	//scene.transforms.emplace_back();
	//player.transform = &scene.transforms.back();

	// Set pointer on player's head
	pointer_transform->position = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 5.0f, 0.0f) + player.transform->position;
	goods_drawable->transform->position = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 5.0f, 0.0f) + player.transform->position;


	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;
	player.camera->transform->parent = player.transform;


	//player's eyes are 1.8 units above the ground:
	//player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);

	player.camera->transform->position = glm::vec3(0.0f, -5.0f, 4.0f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	player.camera->transform->rotation = glm::angleAxis(glm::radians(75.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(player.transform->position);
	bgm_loop = Sound::loop((*sound_samples).at("bit_bit_loop"));
	bgm_loop->set_volume(0.3f);
	switch_foot();
}

PlayMode::~PlayMode() {
}
void PlayMode::reset() {
	gameover = false;
	player.transform->position = player_start_position;
	player.transform->rotation = player_start_rotation;
	player.at = walkmesh->nearest_walk_point(player.transform->position);

	time = max_time;
	score = 0;
	curr_moved = false;
	is_delivering = false;
	gameover = false;
	is_picked_up = true;
	switch_foot();

}
bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {
	if (gameover) {
		if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_SPACE) {
			reset();
		}
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a && right.pressed == false) {
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d && left.pressed == false) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		/*else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}*/
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			if (is_picked_up && !is_delivering && glm::distance(player.transform->position, destinations[color]->position) < valid_distance) {
				deliver_up_vec = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
				goods_drawable->transform->parent = nullptr;
				goods_drawable->transform->position = 2.0f * deliver_up_vec + destinations[color]->position;
				is_delivering = true;
				is_picked_up = false;

				Sound::play((*sound_samples).at("confirm"));
				score++;
				switch_foot();
			}
			if (!is_picked_up && !is_delivering && glm::distance(player.transform->position, spawn_position) < valid_distance) {
				is_picked_up = true;
				Sound::play((*sound_samples).at("select"));
				switch_foot();
			}
		}

	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			curr_moved = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			curr_moved = false;

			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 up = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, up) * player.transform->rotation;

			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (time <= 0.0f) {
		gameover = true;
		time = 0.0f;
		return;
	}
	time -= elapsed;
	//update pointer
	pointer_transform->position = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 4.8f, 0.0f) + player.transform->position;

	if (is_picked_up) {
		pointer_transform->rotation = glm::quatLookAt(-glm::normalize(destinations[color]->position - pointer_transform->position), glm::vec3(0, 0, 1));
	}
	else {
		pointer_transform->rotation = glm::quatLookAt(-glm::normalize(spawn_position - pointer_transform->position), glm::vec3(0, 0, 1));
	}
	//player walking:
	{
		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !curr_moved) {
			curr_moved = true;
			if (is_prev_left) {
				move.y = -1.0f;
			}
			else {
				move.y = 1.0f;
			}
			is_prev_left = !is_prev_left;
			switch_foot();

		}
		if (right.pressed && !curr_moved) {
			curr_moved = true;
			if (!is_prev_left) {
				move.y = -1.0f;
			}
			else {
				move.y = 1.0f;
			}
			is_prev_left = !is_prev_left;
			switch_foot();

		}
		//if (left.pressed && !right.pressed) move.x = -1.0f;
		//if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y = -1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				//rotate step to follow surface:
				remain = rotation * remain;
			}
			else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const& a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const& b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const& c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b - a);
				glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				}
				else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player.transform->position = walkmesh->to_world_point(player.at);

		{ //update player's rotation to respect local (smooth) up-vector:

			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
		}

		/*
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
		*/
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	if (is_delivering) {
		goods_drawable->transform->position -= elapsed * deliver_up_vec * 2.0f;
		if (glm::distance(goods_drawable->transform->position, destinations[color]->position) < 0.1f) {
			is_delivering = false;
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> color_rand(0, (int)destinations.size() - 1);
			color = color_rand(mt);
			spawn_goods();
		}
	}
	else if (is_picked_up) {
		goods_drawable->transform->position = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 3.5f, 0.0f) + player.transform->position;
	}

}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);

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
		std::string text_to_draw;
		if (!gameover) {
			text_to_draw = "Score: " + std::to_string(score) + "    Time: " + std::to_string((int)time) + "." + std::to_string((int)(time * 10) % 10);
		}
		if (gameover) {
			text_to_draw = "Final Score: " + std::to_string(score) + "    Game Over, Press Space to restart.";
		}
		lines.draw_text(text_to_draw,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text_to_draw,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
