#include "ReconstructScene.h"
#include <experimental/filesystem>

#include <fstream>
#include <sstream>

namespace {
	const size_t char_buff_size = 512, max_frames = 120;
	char char_buff[char_buff_size];
	ControllerDP* myControl;
	DD_Light* light;
	DD_Camera* cam;
	DD_Agent *balls;//, *ground;
	DD_Texture2D *tex01, *tex02;
	const std::string proj_loc = "D:/Docs/DayDream_Repo/Projects/DepthProjection/";
	const std::string pic_loc = "D:/Docs/kinect_pics/61036/";
	std::string folder_c = pic_loc + "smile_c/";
	std::string folder_d = pic_loc + "smile_d/";
	std::string nose_file = pic_loc + "nose_center.csv";
	std::string start_file = "00-53-831";
	size_t total_frames = 0;
	std::string color_pics[max_frames] ,depth_pics[max_frames];

	bool render_particle = false, process_frame = false;
	glm::vec4 bin[512 * 424];
	glm::vec4 color_bin[512 * 424];
	glm::vec4 rendered_p_bin[512 * 424];
	glm::vec4 rendered_c_bin[512 * 424];

	size_t indices[512 * 424], num_particles;
	int current_pic_index = 0;
	const float fps30 = 1.0 / 30.0;
	const float fps25 = 1.0 / 25.0;
	const float fps20 = 1.0 / 20.0;
	const float fps10 = 1.0 / 10.0;
	float fpsTracker = 0.f, run_time = 0.f;
	bool new_frame = true, pause = false;

	unsigned char *images_d_RAM[max_frames] , *images_c_RAM[max_frames];
	glm::ivec2 nose_bin[max_frames];
}

void ripDepth(glm::ivec2 *md, size_t idx, std::string input) {
	std::string val1, val2;
	size_t last_idx = input.find_last_of(',');
	if (last_idx != std::string::npos) {
		val2 = input.substr(last_idx + 1);
		val1 = input.substr(0, last_idx);
		size_t cut_off = val1.find_last_of(',');
		val1 = val1.substr(cut_off + 1, last_idx);
	}
	md[idx].x = (int)(atof(val1.c_str()) + 0.5f);
	md[idx].y = (int)(atof(val2.c_str()) + 0.5f);
}

void readMeshData(const char* _file, glm::ivec2 *md, const char* start) {
	std::ifstream file(_file);
	size_t idx = 0;
	bool start_rip = false;

	if (file.good()) {
		std::stringstream analysisBuf;
		analysisBuf << file.rdbuf();
		file.close();
		std::string line, subline;
		std::getline(analysisBuf, line); // remove header

		while (std::getline(analysisBuf, line) && idx < max_frames) {
			size_t indexEnd = line.find_first_of(',');
			std::string s_index = line.substr(0, indexEnd);

			if (s_index.compare(start) == 0) { start_rip = true; }

			if (start_rip) {
				std::string s_remain = line.substr(indexEnd + 1);
				ripDepth(nose_bin, idx, s_remain);
				snprintf(char_buff, char_buff_size, "[%s]: %d, %d\n",
					s_index.c_str(), nose_bin[idx].x, nose_bin[idx].y);
				DD_Terminal::post(char_buff);
				idx += 1;
			}
		}
	}
}

ReconstructScene::~ReconstructScene() {
	for (size_t i = 0; i < total_frames; i++) {
		SOIL_free_image_data(images_d_RAM[i]);
		SOIL_free_image_data(images_c_RAM[i]);
	}
}

void ReconstructScene::Init() {
	// get center point
	glm::mat3 invdepthK = glm::mat3(
		0.002989879854668, 0.0, 0.0,
		0.0, 0.002968574668559, 0.0,
		-0.752066006979576, -0.621590740421894, 1.0);
	glm::vec3 center = (invdepthK * glm::vec3(321.6797, 247, 1.0)) * 926.f;

	readMeshData(nose_file.c_str(), nose_bin, start_file.c_str());

	// add mouse controller to level
	myControl = new ControllerDP("avatar");
	AddAgent(myControl);
	myControl->clickedSpace = false;
	myControl->UpdatePosition(glm::vec3(center));
	myControl->initRot(glm::radians(0.f), glm::radians(180.f));

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "shadow");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	light->m_flagShadow = false;

	// add generic camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 10000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&ReconstructScene::basePost, this,
		std::placeholders::_1);
	AddCallback("post", a);
	AddCallback("compute_done", a);

	// add agent model
	balls = ResSpace::getNewDD_Agent(res_ptr, "agent");
	balls->AddModel("sphere_prim", 0.f, 10000.f);
	balls->UpdatePosition(center);
	balls->UpdateScale(glm::vec3(0.1f));
	//balls->UpdateRotation(glm::rotate(glm::quat(), glm::radians(90.f),
		//glm::vec3(1.f, 0.f, 0.f)));
	//balls->flag_render = false;

	// change agent material
	DD_Material* mat = ResSpace::getNewDD_Material(res_ptr, "my_mat_01");
	mat->m_base_color = glm::vec4(1.f, 0.1f, 0.1f, 1.f);
	int m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_01");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	balls->SetMaterial(m_idx);

	// create textures
	size_t d_frames = GetPNGFiles(folder_d.c_str());
	size_t c_frames = GetPNGFiles(folder_c.c_str());
	total_frames = (d_frames < c_frames) ? d_frames : c_frames;

	tex01 = ResSpace::getNewDD_Texture2D(res_ptr, "depth0");
	tex01->Internal_Format = GL_RGBA16F;
	tex01->path = folder_d + depth_pics[0];
	tex02 = ResSpace::getNewDD_Texture2D(res_ptr, "color0");
	tex02->Internal_Format = GL_RGBA16F;
	tex02->path = folder_c + color_pics[0];

	int w, h;
	for (size_t i = 0; i < total_frames; i++) {
		std::string fpath = folder_d + depth_pics[i];
		images_d_RAM[i] = SOIL_load_image(fpath.c_str(), &w, &h, 0,
			SOIL_LOAD_RGBA);
	}
	for (size_t i = 0; i < total_frames; i++) {
		std::string fpath = folder_c + color_pics[i];
		images_c_RAM[i] = SOIL_load_image(fpath.c_str(), &w, &h, 0,
			SOIL_LOAD_RGBA);
	}
}

// Setup imgui interface
void ReconstructScene::setInterface(const float dt) {
	fpsTracker += dt;
	if (fpsTracker > fps20) {
		if (current_pic_index < ((int)total_frames - 1) && !pause) {
			run_time += fpsTracker;
			current_pic_index += 1;
			new_frame = true;
		}
		fpsTracker = 0.f;
	}

	const float debugY = m_screenH - 50.f;
	const float debugW = m_screenW - 10.f;
	ImGui::SetNextWindowPos(ImVec2(0.f, debugY));
	ImGui::SetNextWindowSize(ImVec2(debugW, 20.f));

	ImGuiWindowFlags toolbar_flags = 0;
	toolbar_flags |= ImGuiWindowFlags_NoTitleBar;
	toolbar_flags |= ImGuiWindowFlags_NoResize;
	toolbar_flags |= ImGuiWindowFlags_NoMove;
	ImGui::Begin("Depth Color Reconstruction", nullptr, toolbar_flags);

	ImGui::Text("Depth Reconstruction \t");
	ImGui::SameLine();

	// Pause button
	if (pause) { if (ImGui::Button(" >  ")) { pause = false; } }
	else { if (ImGui::Button(" || ")) { pause = true; } }

	// Restart button
	ImGui::SameLine();
	if (ImGui::Button(" |<< ")) {
		current_pic_index = 0.f;
		run_time = 0.f;
	}

	// slider
	ImGui::SameLine();
	snprintf(char_buff, char_buff_size, "%.4f\n", run_time);
	if (ImGui::SliderInt("", &current_pic_index, 0, total_frames - 1)) {
		new_frame = true;
	}

	ImGui::End();
}

// Uses experimental filesystem library to loop thru directory and find files
size_t ReconstructScene::GetPNGFiles(const char * direc) {
	namespace fs = std::experimental::filesystem;
	size_t num_files = 0;
	for (auto & p : fs::directory_iterator(direc)) {

		std::string fname = p.path().string();
		size_t ftype = fname.length() - 4;
		if (fname.substr(ftype) == ".jpg" || fname.substr(ftype) == ".png") {
			size_t sub_index = fname.find_last_of("\\") + 1;

			if (num_files >= max_frames) {
				return num_files;								// limiter
			}

			if (fname.find("c_[") != std::string::npos) {
				color_pics[num_files] = fname.substr(sub_index);
				num_files += 1;
			}
			else if (fname.find("d_[") != std::string::npos) {
				depth_pics[num_files] = fname.substr(sub_index);
				num_files += 1;
			}
		}
	}
	return num_files;
}

void ReconstructScene::CreateTexForFrame(const size_t index) {
	tex01->Refill(images_d_RAM[index]);
	tex02->Refill(images_c_RAM[index]);
}

DD_Event ReconstructScene::basePost(DD_Event& event) {
	if (event.m_type.compare("post_compute") == 0) {

		// imgui
		setInterface(event.m_time);

		if (new_frame) {
			DD_Event e = DD_Event();
			e.m_type = "compute_texA";

			CreateTexForFrame(current_pic_index);

			shaderDataA* _data = new shaderDataA();
			_data->byte_size01 = sizeof(glm::vec4) * 512 * 424;
			//_data->byte_size02 = sizeof(glm::vec4) * 512 * 424;
			_data->data_info[0] = 4;
			_data->data_info[1] = 4;
			_data->data_01 = bin;
			//_data->data_02 = color_bin;
			_data->sender = "me1";
			_data->shader_ID = "kinectDepthCompute";
			_data->texture_ID = "depth0";

			snprintf(char_buff, char_buff_size, "Nose input: %d, %d",
				nose_bin[current_pic_index].x, nose_bin[current_pic_index].y);
			//DD_Terminal::post(char_buff);

			_data->uniform_01 = glm::vec2(nose_bin[current_pic_index]);

			e.m_message = _data;
			new_frame = false;
			process_frame = true;
			return e;
		}
	}
	if (event.m_type.compare("post") == 0) {

		if (process_frame) {
			num_particles = 0;
			glm::vec3 calc_nose_pos;
			glm::vec3 zero = glm::vec3();
			for (size_t j = 0; j < 512; j++) {
				for (size_t i = 0; i < 424; i++) {
					glm::vec3 temp = glm::vec3(bin[i * 512 + j]);
					if ((i * 512 + j) == 0) {
						balls->UpdatePosition(temp);

						snprintf(char_buff, char_buff_size,
							"Nose output: %.3f, %.3f, %.3f",
							temp.x, temp.y, temp.z);
						//DD_Terminal::post(char_buff);

						temp = zero;
					}
					if (temp != zero && temp.z < 1400.0) {
						indices[num_particles] = i * 512 + j;
						num_particles += 1;
					}
				}
			}
			for (size_t i = 0; i < num_particles; i++) {
				rendered_p_bin[i] = bin[indices[i]];
				rendered_p_bin[i].w = 1.f;
				//rendered_c_bin[i] = color_bin[indices[i]];
			}
			for (size_t i = 0; i < 100; i++) {
				/*
				snprintf(char_buff, char_buff_size, "uv--> %.4f, %.4f, %.4f, %.4f\n",
					rendered_c_bin[i].x, rendered_c_bin[i].y, rendered_c_bin[i].z, rendered_c_bin[i].w);
				DD_Terminal::post(char_buff);
				//*/
			}
			render_particle = true;
			process_frame = false;
		}

		if (render_particle) {
			// send to particle engine for render
			DD_Event e = DD_Event();
			e.m_type = "particle_jobA";

			shaderDataA* _data = new shaderDataA();
			_data->byte_size01 = sizeof(glm::vec4) * num_particles;
			_data->data_info[0] = num_particles;

			_data->data_01 = rendered_p_bin;

			_data->sender = "me1";
			_data->shader_ID = "kinectParticle";
			_data->texture_ID = "color0";

			e.m_message = _data;
			return e;
		}

		return DD_Event();
	}
	return DD_Event();
}
