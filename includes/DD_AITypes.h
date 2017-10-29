#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_AITypes:
*		- structs for ai system
*
*	TODO:	==
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"
#include "DD_MeshTypes.h"

typedef void(*heuristic_func)(const glm::vec3, const glm::vec3,
							  const dd_array<glm::vec3>&, dd_array<float>&);

enum AI_ALGORITHMS
{
	A_STAR,
	DIJKSTRA,
	NUM_ALGOS
};

struct DD_AINode
{
	glm::vec3 color;
	int index = -1, parent_index = -1;
	bool closed = false;
	dd_array<LinePoint> connections;
	dd_array<glm::vec2> costs;
	float cost_to_current = 0.f;
};

struct DD_AINodeQ
{
	int index = -1, parent_index = -1;
	float cost_to_current = 0.f;
	void set(const DD_AINode* n)
	{
		index = n->index;
		parent_index = n->parent_index;
		cost_to_current = n->cost_to_current;
	}
};

struct AI_Obstacle
{
	glm::mat4 transform;
	glm::vec3 aabb_min, aabb_max;
	float radius = 0;
	size_t type = 0; // 0 for circle, 1 for box

	inline void init(const glm::mat4 _transform, const BoundingBox bbox,
					 const size_t type = 0)
	{
		transform = _transform;
		aabb_min = bbox.min;
		aabb_max = bbox.max;
		if( type == 0 ) {
			//glm::vec4 v = _transform[3];
			radius = std::abs(aabb_max.x);
		}
	}
};

struct AI_Agent
{
	std::string m_ID = "", aiobject_ID = "", line_ID = "";
	size_t path_index = 0;
	glm::vec3 origin, goal, goal_vel, current_pos, curr_vel;
	float radius, sense_n_horizon[2], max_speed;
	bool plan_now = false, calc_heuristics = true, move_now = false, replan = false;
	dd_array<float> heuristics;
	dd_array<glm::vec3> path;
	heuristic_func h_func;
	inline void init(const glm::vec3 start, const glm::vec3 end, const float _radius,
					 const float speed, const char* ai_object_ID, heuristic_func func_pointer)
	{
		aiobject_ID = ai_object_ID;
		origin = start;
		goal = end;
		current_pos = start;
		radius = _radius;
		h_func = func_pointer;
		max_speed = speed;
		path.resize(1);
		path[0] = end;
		sense_n_horizon[0] = 500.f;
		sense_n_horizon[1] = 15.f;
		curr_vel = glm::vec3(0.f);
	}
};

struct DD_AIObject
{
	AI_ALGORITHMS algo_type;
	size_t num_nodes = 10, num_connections = 5;
	dd_array<AI_Obstacle> obstacles;
	dd_array<glm::vec3> plots;
	dd_array<size_t> planned_path;
	float prm_search_radius = -1.f, agent_radius = 0;
	bool new_prm = true, new_nodes = true, flag_render = true;

	std::string m_ID = "", node_mesh = "", line_mesh = "";
	glm::vec3 ground_p, ground_pmax, ground_pmin;

	dd_array<DD_AINode> aiobj_nodes;

	inline void init(const AI_ALGORITHMS algo, const glm::vec3 ground_pos,
					 const glm::vec3 ground_max_corner, const glm::vec3 ground_min_corner,
					 const size_t total_nodes = 10, const size_t lines_per_node = 5,
					 const float max_agent_radius = 0, const char* node_mesh_ID = "",
					 const char* line_mesh_ID = "")
	{
		algo_type = algo;
		ground_p = ground_pos;
		ground_pmax = ground_max_corner;
		ground_pmin = ground_min_corner;
		num_nodes = total_nodes;
		num_connections = lines_per_node;
		node_mesh = node_mesh_ID;
		line_mesh = line_mesh_ID;
		agent_radius = max_agent_radius;
	}
};
