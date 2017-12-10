#include "DD_AISystem.h"
#include <time.h>
#include <limits>
#include "DD_Terminal.h"

namespace {
// bool once = true;
dd_array<LinePoint> temp_lines;
dd_array<glm::vec2> temp_cost;

float replan_tick = 0.f;
const float tick_interval = 10.f;
}

/**
        Struct contains containers and variables for handling A* simulation data
*/
struct AStarData {
  dd_array<glm::vec3> points = dd_array<glm::vec3>(500);
  dd_array<DD_AINode> curr_nodes = dd_array<DD_AINode>(500);
  dd_array<DD_AINodeQ> p_queue = dd_array<DD_AINodeQ>(500);
  size_t p_q_size = 0;
};

void calculateHeuristics(AStarData& data, AI_Agent* agent);
dd_array<glm::vec3> planA_Star(DD_AIObject* ai_obj, AI_Agent* agent);
void removeFromPriorityQ(const int index, AStarData& data);
void addToPriorityQ(DD_AINode* new_node, const int parent,
                    const float this_cost, const float this_hcost,
                    const dd_array<float> _hcosts, AStarData& data);
size_t addToCLosedQ(const int index, AStarData& data);
void expandNode(const int index, AI_Agent* agent, AStarData& data);
DD_AINode generateLOS(DD_AIObject* ai_obj, AI_Agent* agent, const int index,
                      dd_array<glm::vec3>& points);
void ConnectToGoalNode(DD_AINode& goal, AStarData& data);

DD_Event DD_AISystem::update(DD_Event& event) {
  if (event.m_type.compare("update_AI") == 0) {
    std::string tempstr;

    replan_tick += event.m_time;

    // loop thru AIObjects and execute code
    for (size_t i = 0; i < res_ptr->ai_obj_counter; i++) {
      DD_AIObject* ai = &res_ptr->ai_s[i];
      if (ai->new_prm) {
        // create prm
        ai->new_prm = false;
        generatePRM(ai);
        mapPRMtoLines(ai);
      }
      // render PRM lines
      if (ai->line_mesh.compare("") != 0) {
        DD_LineAgent* _l =
            ResSpace::findDD_LineAgent(res_ptr, ai->line_mesh.c_str());
        if (_l) {
          _l->flag_render = ai->flag_render;
        }
      }
    }
    // loop thru ai agents to update heuristics and crowd algorithms
    //#pragma omp for
    for (int i = 0; i < (int)res_ptr->ai_obj_counter; i++) {
      DD_AIObject* ai = &res_ptr->ai_s[i];
      for (size_t j = 0; j < res_ptr->ai_agent_counter; j++) {
        AI_Agent* agent = &res_ptr->ai_agents[j];
        if (agent->aiobject_ID == ai->m_ID) {
          //*
          if (!agent->plan_now && agent->replan && replan_tick >= 2.f) {
            agent->path_index = 0;
            agent->path = std::move(planA_Star(ai, agent));
            tempstr = "path_" + std::to_string(j);
            createLinePath(agent, tempstr.c_str());
          }
          //*/
          if (agent->plan_now) {  // plan using A*
            agent->path_index = 0;
            agent->path = std::move(planA_Star(ai, agent));
            tempstr = "path_" + std::to_string(j);

            createLinePath(agent, tempstr.c_str());

            agent->plan_now = false;
          }
          // calculate velocity
          if (agent->move_now) moveAlongPath(ai, agent, event.m_time);
        }
      }
    }
    for (size_t i = 0; i < res_ptr->ai_agent_counter; i++) {
      AI_Agent* agent = &res_ptr->ai_agents[i];
      if (agent->move_now) SimulateCrowd(&res_ptr->ai_agents[i], event.m_time);
    }

    if (replan_tick > tick_interval) {
      replan_tick = 0.f;
    }
  }

  return DD_Event();
}

/**
        Uses provided function handler in initializtion method to calculate
        heuristics for A*
        \param data Contains all A* data for current AI_Agent object
        \param agent AI_Agent object containing individual AI agent data
*/
void calculateHeuristics(AStarData& data, AI_Agent* agent) {
  agent->heuristics.resize(data.points.size());
  (*agent->h_func)(agent->current_pos, agent->goal, data.points,
                   agent->heuristics);
}

void removeFromPriorityQ(const int index, AStarData& data) {
  if (index < 0) {
    return;
  }
  for (size_t i = index; i < data.p_q_size; i++) {
    data.p_queue[i] = std::move(data.p_queue[i + 1]);
  }
  data.p_q_size -= 1;
}

/**
        Verifies if a node is a duplicate and if not, adds it to the frontier
   (in
        sorted order) for possible expansion
        \param new_node Node to be added to priority queue
        \param parent Parent node of new_node
        \param this_cost Total cost of path to new_node
        \param this_hcost Heuristic cost of new_node to goal node
        \param _hcosts Container for all heuristic costs to goal node
        \param data Container for current A* plan
*/
void addToPriorityQ(DD_AINode* new_node, const int parent,
                    const float this_cost, const float this_hcost,
                    const dd_array<float> _hcosts, AStarData& data) {
  if (data.p_q_size == 0) {
    new_node->cost_to_current = this_cost;
    new_node->parent_index = (int)parent;
    data.p_queue[0].set(new_node);
    data.p_q_size += 1;
  } else {
    // sort by score
    float _g = this_cost + new_node->cost_to_current;
    float total = _g + this_hcost;

    // check if node already exists in tree (if new_node has smaller cost,
    // remove older version of node, set updated costs, resort queue)
    bool duplicate = false;
    size_t counter = 0;
    while (!duplicate && counter < data.p_q_size) {
      int saved_index = data.p_queue[counter].index;
      if (saved_index == new_node->index) {
        duplicate = true;
      }
      counter += 1;
    }
    if (duplicate) {
      float old_cost = data.p_queue[counter - 1].cost_to_current;
      if (old_cost > _g) {
        removeFromPriorityQ((int)counter - 1, data);  // remove then replace
      } else {
        return;  // older node has a lower G cost so skip new_node
      }
    }

    // update score and place node in sorted order (bottom --> up loop)
    new_node->parent_index = (int)parent;
    new_node->cost_to_current = _g;
    bool placed_node = false;
    for (int i = ((int)data.p_q_size - 1); i >= 0 && !placed_node; i--) {
      // printf("Index = %zd\n", i);
      DD_AINode* p_node = &data.curr_nodes[data.p_queue[i].index];
      size_t index = p_node->index;
      float f_cost = _hcosts[index] + data.p_queue[i].cost_to_current;
      // printf("node G = %f\n", _g);
      // printf("node F = %f\n", total);
      // printf("F cost = %f\n", f_cost);
      if (total < f_cost) {
        // move node being checked against lower on queue
        data.p_queue[i + 1] = std::move(data.p_queue[i]);
        data.p_queue[i].set(new_node);
      } else {
        // place current node in correct spot
        data.p_queue[i + 1].set(new_node);
        placed_node = true;
      }
    }
    data.p_q_size += 1;
  }
}

/**
        Adds top node to closed list (list represented as boolean flag)
        \param index Current node's location in node container
        \param data Contains all A* data for current AI_Agent object
*/
size_t addToCLosedQ(const int index, AStarData& data) {
  data.curr_nodes[index].closed = true;
  removeFromPriorityQ(0, data);
  return data.curr_nodes[index].index;
}

/**
        Expands unopened node, provided by index, and adds to priority queue.
   Checks
        for duplicates
        \param index Location of node to be expanded in node container
        \param agent AI_Agent object containing individual AI agent data
        \param data Contains all A* data for current AI_Agent object
*/
void expandNode(const int index, AI_Agent* agent, AStarData& data) {
  DD_AINode* old_node = &data.curr_nodes[index];
  // expand all node connections and add to frontier
  for (size_t i = 0; i < old_node->connections.size(); i++) {
    size_t cost_index = (size_t)old_node->costs[i].x;
    DD_AINode* new_node = &data.curr_nodes[cost_index];
    if (new_node->closed) {
      continue;  // skip current node (already explored)
    }
    new_node->cost_to_current = old_node->cost_to_current;

    addToPriorityQ(new_node, index, old_node->costs[i].y,
                   agent->heuristics[cost_index], agent->heuristics, data);
  }
}

/**
        Generates PRM line of sight lines for node placement on map. Used mostly
        to place nodes on prm map in real-time for plan navigation
        \param ai_obj A* object containing initialization data
        \param agent AI_Agent object containing individual AI agent data
        \param index Location of node in container having PRM connections
   created
        \param points Container of PRM node locations
*/
DD_AINode generateLOS(DD_AIObject* ai_obj, AI_Agent* agent, const int index,
                      dd_array<glm::vec3>& points) {
  size_t numL = DD_MathLib::calculateLOSLines(
      index, ai_obj->obstacles, points, temp_lines, temp_cost,
      ai_obj->agent_radius, ai_obj->prm_search_radius);

  DD_AINode _node = DD_AINode();
  _node.index = index;
  _node.connections.resize(numL);
  _node.connections = temp_lines;
  _node.costs.resize(numL);
  _node.costs = temp_cost;

  return _node;
}

// conect nodes to goal (per agent)
/**
        Places goal node in PRM by linking PRM nodes to goal node. Number of
   links
        limited (set in initialization data)
        \param goal Node of goal position
        \param data Contains all A* data for current AI_Agent object
*/
void ConnectToGoalNode(DD_AINode& goal, AStarData& data) {
  for (size_t i = 0; i < goal.costs.size(); i++) {
    const size_t n_index = (size_t)goal.costs[i].x;
    DD_AINode& _node = data.curr_nodes[n_index];
    // resize
    const size_t new_size = _node.costs.size() + 1;
    dd_array<glm::vec2> t_costs = std::move(_node.costs);
    dd_array<LinePoint> t_conn = std::move(_node.connections);
    _node.costs.resize(new_size);
    _node.connections.resize(new_size);
    _node.costs = t_costs;
    _node.connections = t_conn;

    // add connections and costs to goal
    _node.costs[new_size - 1].x = (float)goal.index;
    _node.costs[new_size - 1].y = goal.costs[i].y;
    _node.connections[new_size - 1].pos01 =
        glm::vec4(data.points[n_index], 1.f);
    _node.connections[new_size - 1].pos02 =
        glm::vec4(data.points[goal.index], 1.f);
  }
}

/**
        Main function that runs A* planning loop.
        \param ai_obj A* object containing initialization data
        \param agent Contains all A* data for current AI_Agent object
*/
dd_array<glm::vec3> planA_Star(DD_AIObject* ai_obj, AI_Agent* agent) {
  AStarData data = AStarData();
  // reset
  size_t total_nodes = ai_obj->num_nodes + 2;
  size_t start_index = total_nodes - 2, end_index = total_nodes - 1;
  data.p_q_size = 0;
  data.points = ai_obj->plots;
  data.curr_nodes = ai_obj->aiobj_nodes;

  // create nodes for start and end
  data.points[start_index] = agent->current_pos;
  data.curr_nodes[start_index] =
      generateLOS(ai_obj, agent, (int)start_index, data.points);
  data.points[end_index] = agent->goal;
  data.curr_nodes[end_index] =
      generateLOS(ai_obj, agent, (int)end_index, data.points);

  if (agent->calc_heuristics) {  // calculate heuristics
    calculateHeuristics(data, agent);
    agent->calc_heuristics = false;
  }
  ConnectToGoalNode(data.curr_nodes[end_index], data);

  for (size_t i = 0; i < total_nodes; i++) {
    data.curr_nodes[i].closed = false;
    data.curr_nodes[i].cost_to_current = 0.f;
    data.curr_nodes[i].parent_index = -1;
  }
  DD_AINode* start_n = &data.curr_nodes[start_index];
  size_t last_node = 0, path_length = 0;

  addToPriorityQ(start_n, -1, 0, 0, agent->heuristics, data);
  // expand highest node in queue until end node is at the top
  while (data.p_queue[0].index != (int)end_index) {
    // send current node to the closed list
    last_node = addToCLosedQ(data.p_queue[0].index, data);

    expandNode((int)last_node, agent, data);
  }

  dd_array<size_t> path = dd_array<size_t>(total_nodes);
  // reconstruct optimal path
  int next_node = (int)end_index;
  while (next_node != (int)start_index) {
    // printf("Previous node: %d\n", next_node);
    path[path_length] = next_node;
    path_length += 1;
    next_node = data.curr_nodes[next_node].parent_index;
  }
  dd_array<glm::vec3> new_path = dd_array<glm::vec3>(path_length);
  for (size_t i = 1; i < path_length; i++) {
    // ofset to get points after 1st
    new_path[i - 1] = data.points[path[path_length - i]];
  }
  new_path[path_length - 1] = agent->goal;

  return new_path;
}
/**
        Creates PRM map using Halton sequence
        \param ai_obj A* object containing initialization data
*/
void DD_AISystem::generatePRM(DD_AIObject* ai_obj) {
  // randomly plot points on screen
  if (ai_obj->new_nodes) {
    ai_obj->new_nodes = false;

    ai_obj->plots.resize(ai_obj->num_nodes);
    temp_lines.resize(ai_obj->num_connections);
    temp_cost.resize(ai_obj->num_connections);
    ai_obj->aiobj_nodes.resize(ai_obj->num_nodes);

    dd_array<glm::vec3> colors = dd_array<glm::vec3>(ai_obj->num_nodes);
    dd_array<glm::mat4> temp_mats = dd_array<glm::mat4>(ai_obj->num_nodes);

    DD_MathLib::randomSampleRect(ai_obj->ground_p, ai_obj->ground_pmin,
                                 ai_obj->ground_pmax, ai_obj->plots);

    // set up instances of node mesh representation
    if (ai_obj->node_mesh.compare("") != 0) {
      DD_Agent* bulb =
          ResSpace::findDD_Agent(res_ptr, ai_obj->node_mesh.c_str());
      if (bulb) {
        for (size_t i = 0; i < ai_obj->plots.size(); i++) {
          temp_mats[i] = glm::translate(glm::mat4(), ai_obj->plots[i]);
          temp_mats[i] *= glm::scale(glm::mat4(), bulb->size());
          colors[i] = glm::vec3(1.f, 1.f, 1.f);
        }
        // load up colors and markers
        bulb->SetInstances(temp_mats);
        bulb->SetColorInstances(colors);
      }
    }
  }
#pragma omp for
  for (int i = 0; i < (int)ai_obj->num_nodes; i++) {
    size_t numL = DD_MathLib::calculateLOSLines(
        i, ai_obj->obstacles, ai_obj->plots, temp_lines, temp_cost,
        ai_obj->agent_radius, ai_obj->prm_search_radius);

    // load nodes
    ai_obj->aiobj_nodes[i].index = i;
    ai_obj->aiobj_nodes[i].connections.resize(numL);
    ai_obj->aiobj_nodes[i].connections = temp_lines;
    ai_obj->aiobj_nodes[i].costs.resize(numL);
    ai_obj->aiobj_nodes[i].costs = temp_cost;
  }
}

/**
        Create line render of the paths for generated PRM
        \param ai_obj A* object containing initialization data
*/
void DD_AISystem::mapPRMtoLines(DD_AIObject* ai_obj) {
  if (ai_obj->line_mesh.compare("") != 0) {
    DD_LineAgent* l_a =
        ResSpace::findDD_LineAgent(res_ptr, ai_obj->line_mesh.c_str());
    if (!l_a) {
      return;  // end of function
    }
    // clean out line bin and set size constraints
    l_a->FlushLines();
    l_a->lines.resize(ai_obj->num_nodes * ai_obj->num_connections);

    size_t running_index = 0;
    for (size_t i = 0; i < ai_obj->num_nodes; i++) {
      size_t numL = ai_obj->aiobj_nodes[i].connections.size();
      // fill up line array w/ line segments
      for (size_t j = 0; j < numL; j++) {
        l_a->lines[running_index + j] = ai_obj->aiobj_nodes[i].connections[j];
      }
      running_index += numL;
    }
    dd_array<LinePoint> temp = dd_array<LinePoint>(running_index);
    temp = l_a->lines;
    l_a->lines.resize(running_index);
    l_a->lines = std::move(temp);
  }
}

/**
        Creates line render of the paths for agent's A* path
        \param agent Contains all A* data for current AI_Agent object
        \param lineID Descriptor used to find Line object attahced to agent
*/
void DD_AISystem::createLinePath(AI_Agent* agent, const char* lineID) {
  const size_t numLines = agent->path.size();
  DD_LineAgent* _l;
  if (agent->line_ID.compare(lineID) == 0) {
    _l = ResSpace::findDD_LineAgent(res_ptr, lineID);
  } else {
    _l = ResSpace::getNewDD_LineAgent(res_ptr, lineID);
    agent->line_ID = lineID;
  }
  _l->FlushLines();
  _l->color = glm::vec4(0.f, 0.7f, 0.f, 1.f);
  _l->lines.resize(numLines);

  for (size_t i = 0; i < numLines; i++) {
    if (i == 0) {
      glm::vec4 a =
          glm::vec4(agent->current_pos.x, 5.f, agent->current_pos.z, 0.f);
      glm::vec4 b = glm::vec4(agent->path[0].x, 5.f, agent->path[0].z, 0.f);
      _l->lines[0].pos01 = a;
      _l->lines[0].pos02 = b;
    } else {
      glm::vec4 a =
          glm::vec4(agent->path[i - 1].x, 5.f, agent->path[i - 1].z, 0.f);
      glm::vec4 b = glm::vec4(agent->path[i].x, 5.f, agent->path[i].z, 0.f);
      _l->lines[i].pos01 = a;
      _l->lines[i].pos02 = b;
    }
  }
  _l->m_buffer.resize(numLines * 2);
  for (size_t j = 0; j < _l->lines.size(); j++) {
    _l->m_buffer[j * 2] = _l->lines[j].pos01;
    _l->m_buffer[j * 2 + 1] = _l->lines[j].pos02;
  }
}

/**
        Advances agent along planned path using path smoothing
        \param ai_obj A* object containing initialization data
        \param agent Contains all A* data for current AI_Agent object
        \param dt Time step interval
*/
void DD_AISystem::moveAlongPath(DD_AIObject* ai_obj, AI_Agent* agent,
                                const float dt) {
  if (agent->path.size() == 1) {
    glm::vec3 pos = agent->current_pos;
    glm::vec3 ray = agent->goal - pos;
    if (glm::length(ray) <= 10.f) {
      agent->goal_vel = glm::vec3(0.f);
      agent->curr_vel = glm::vec3(0.f);
      return;
    }
    ray.y = 0.f;
    ray = glm::normalize(ray);
    agent->goal_vel = ray * agent->max_speed;
  } else {
    bool no_intersect = true;
    glm::vec3 pos = agent->current_pos;
    int index = (int)agent->path_index;
    size_t num_obstacles = ai_obj->obstacles.size();
    for (int i = index; i < (int)agent->path.size() && no_intersect; i++) {
      glm::vec3 ray = agent->path[i] - pos;

      for (size_t j = 0; j < num_obstacles && no_intersect; j++) {
        glm::vec3 p = glm::vec3(ai_obj->obstacles[j].transform[3]);
        p.y = 0.f;
        const float r = ai_obj->obstacles[j].radius + ai_obj->agent_radius;
        if (DD_MathLib::checkRayCircle(ray, glm::vec3(pos.x, 0.f, pos.z), p,
                                       r)) {
          no_intersect = false;
          index = i - 1;
        } else {
          index = i;
        }
      }
    }
    index = (index < 0) ? 0 : index;
    // plot course
    agent->path_index =
        (index < (int)agent->path_index) ? agent->path_index : index;
    glm::vec3 ray = agent->path[agent->path_index] - pos;
    if (glm::length(ray) <= 10.f) {
      agent->goal_vel = glm::vec3(0.f);
      agent->curr_vel = glm::vec3(0.f);
      return;
    }
    ray.y = 0.f;
    ray = glm::normalize(ray);
    agent->goal_vel = ray * agent->max_speed;
  }
}

/**
        Compute force from all obstacles (agents and stationary targets) in path
        \param agent Contains all A* data for current AI_Agent object
        \todo Add stationary targets to algo for ttc
*/
glm::vec3 DD_AISystem::ObstacleAvoidance(AI_Agent* agent) {
  srand((unsigned)time(0));
  glm::vec3 F = glm::vec3(0.f);
  DD_AIObject* ai =
      ResSpace::findDD_AIObject(res_ptr, agent->aiobject_ID.c_str());
  const size_t num_obst = ai->obstacles.size();
  const size_t r = (size_t)agent->radius;

  // works w/ circles
  for (size_t i = 0; i < num_obst; i++) {
    AI_Obstacle* obst = &ai->obstacles[i];
    glm::vec3 ray = agent->current_pos - glm::vec3(obst->transform[3]);
    ray.y = 0;
    float dist = glm::length(ray) - (r + obst->radius);

    if (dist < 10.f && dist > 0.f) {
      ray = glm::normalize(ray);
      float magn = 100000.f / (dist);
      if (magn > 5000.f) {
        magn = 5000.f;
      }

      F += ray * magn;
    }
  }
  return F;
}

/**
        Compute force between agent A and B using psuedo-ttc model
        \param agent Contains all A* data for current AI_Agent object
*/
float DD_AISystem::timeToCollision(AI_Agent* agentA, AI_Agent* agentB) {
  float r = agentA->radius + agentB->radius;
  glm::vec3 w = agentB->current_pos - agentA->current_pos;
  float c = glm::dot(w, w) - r * r;
  if (c < 0) {
    return 0.f;
  }  // agents are colliding

  glm::vec3 v = agentA->curr_vel - agentB->curr_vel;
  float a = glm::dot(v, v);
  float b = glm::dot(w, v);
  float discrim = b * b - a * c;
  if (discrim <= 0) {
    return std::numeric_limits<float>::infinity();
  }

  float tau = (b - sqrtf(discrim)) / a;
  if (tau < 0) {
    return std::numeric_limits<float>::infinity();
  }

  return tau;
}

/**
        Simulates crowd of agents using psuedo-ttc model and obstacle avoidance
        \param agent Contains all A* data for current AI_Agent object
        \param dt Time step interval
*/
void DD_AISystem::SimulateCrowd(AI_Agent* agent, const float dt) {
  const float k_term = 2;
  if (res_ptr->ai_agent_counter == 1) {
    glm::vec3 obst_vel = ObstacleAvoidance(agent) * dt;
    agent->curr_vel = agent->goal_vel + obst_vel;
    return;
  }
  dd_array<size_t> bin = dd_array<size_t>(res_ptr->ai_agent_counter);
  size_t neighbors = 0;
  // find neighbors
  for (size_t i = 0; i < bin.size(); i++) {
    AI_Agent* other = &res_ptr->ai_agents[i];
    if (other->m_ID == agent->m_ID) {
      continue;
    }  // skip myself

    float dist = glm::distance(other->current_pos, agent->current_pos);
    if (dist > agent->sense_n_horizon[0]) {
      continue;
    }  // skip far away agents

    bin[neighbors] = i;
    neighbors += 1;
  }
  // calc goal force
  glm::vec3 F = (agent->goal_vel - agent->curr_vel) / k_term;

  // calculate collision
  for (size_t i = 0; i < neighbors; i++) {
    AI_Agent* other = &res_ptr->ai_agents[bin[i]];
    float ttc = timeToCollision(agent, other);

    glm::vec3 direc = agent->current_pos + agent->curr_vel * dt -
                      other->current_pos - other->curr_vel * dt;
    direc = glm::normalize(direc);
    float magn = 0;
    if (ttc >= 0.f && ttc <= agent->sense_n_horizon[1]) {
      magn = 100.f / (sqrtf(ttc) + 0.001f);
    }
    if (magn > 10000.f) {
      magn = 10000.f;
    }

    F += direc * magn;
  }
  F += ObstacleAvoidance(agent);

  glm::vec3 vel = (F * dt);
  agent->curr_vel += vel;
}
