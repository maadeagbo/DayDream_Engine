#include "DD_MathLib.h"

namespace {
	DD_Resources* res;
}

// Use halton sequence to generate pseudo-random looking distribution
float DD_MathLib::getHaltonValue(int index, const int base)
{
	float _f = 1, result = 0;
	while( index > 0 ) {
		_f = _f / base;
		result = result + _f * (index % base);
		index = floor((float)index / (float)base);
	}
	return result;
}

// ONLY INITIALIZED BY GAME MANAGER OBJECT (DO NOT TOUCH)
void DD_MathLib::setResourceBin(DD_Resources * res_bin)
{
	res = res_bin;
}

// Return boolean checking if ray intersects with a circle
bool DD_MathLib::checkRayCircle(const glm::vec3 ray, const glm::vec3 point,
								const glm::vec3 center, const float r)
{
	glm::vec3 end = point + ray;
	glm::vec3 norm_ray = glm::normalize(ray);
	float _temp = glm::dot(norm_ray, center - point);

	glm::vec3 _p = (_temp * norm_ray) + point; // project circle to line
	// make sure circle center exists in line segment
	float dist_p_center = glm::length(_p - center);
	if( dist_p_center > r ) {
		return false;
	}
	if( _temp < 0.f && glm::length(center - point) > r ) {
		return false; // intersection is behind origin
	}
	if( (glm::dot(-norm_ray, center - end) < 0.f) && glm::length(center - end) > r ) {
		return false; // intersection is behind
	}
	return true;
}

glm::vec4 rayPlaneCheck(const glm::vec3 rayDir, const glm::vec3 rayP,
						const glm::vec3 planeP, const glm::vec3 planeDir)
{
	glm::vec4 intersectVal = glm::vec4(-1.f);

	float denom = glm::dot(rayDir, planeDir);
	if( denom > -0.0001f && denom < 0.0001 ) {	// check if ray is parallel
		return intersectVal;
	}
	// solve for time when ray intersects
	float num = glm::dot(planeP - rayP, planeDir);
	intersectVal.w = num / denom;
	glm::vec3 temp = rayP + rayDir * intersectVal.w;
	intersectVal.x = temp.x;
	intersectVal.y = temp.y;
	intersectVal.z = temp.z;

	return intersectVal;
}

bool DD_MathLib::checkAABBoxPoint(BoundingBox & bbox, glm::vec4 point)
{
	glm::vec3 max = bbox.max;
	glm::vec3 min = bbox.min;
	if( point.x <= max.x && point.y <= max.y && point.z <= max.z &&
	   point.x >= min.x && point.y >= min.y && point.z >= min.z ) {
		return true;
	}
	return false;
}

// If agentID is empty, check all object in scene (can be optimized w/ spatial DS)
raycastBuff DD_MathLib::rayCast(const int mouseX,
								const int mouseY,
								const char * agentID)
{
	raycastBuff rcbuff;
	rcbuff.hit = false;
	// calculate screen space to world space point (normalize ray direction from camera)
	DD_Camera* cam;
	bool cam_exists = ResSpace::GetActiveCamera(res, cam);
	if( !cam_exists ) {
		return rcbuff;
	}
	glm::mat4 view = CamSpace::GetViewMatrix(cam);
	glm::mat4 proj = CamSpace::GetPerspecProjMatrix(cam);
	glm::vec4 viewport = glm::vec4(0.f, 0.f, cam->scr_width, cam->scr_height);
	const int _y = cam->scr_height - mouseY;
	GLfloat _z;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, res->G_BUFFER);
	glReadPixels(mouseX, _y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &_z);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glm::vec3 win_coord = glm::vec3(mouseX, _y - 1, _z);
	glm::vec3 win_coord2 = glm::vec3(mouseX, _y + 1, _z);
	glm::vec3 win_coord3 = glm::vec3(mouseX + 1, _y, _z);
	glm::vec3 win_coord4 = glm::vec3(mouseX - 1, _y, _z);
	glm::vec3 point = glm::unProject(win_coord, view, proj, viewport);
	glm::vec3 point2 = glm::unProject(win_coord2, view, proj, viewport);
	glm::vec3 point3 = glm::unProject(win_coord3, view, proj, viewport);
	glm::vec3 point4 = glm::unProject(win_coord4, view, proj, viewport);

	glm::vec3 avgpoint = (point + point2 + point3 + point4) / 4.f;
	point = avgpoint;

	glm::mat4 trans_mat = glm::mat4();
	if( std::string("").compare(agentID) == 0 ) {	// check againts all agents
		for( size_t i = 0; i < res->m_num_agents; i++ ) {
			DD_Agent* agent = res->agents[i];
			trans_mat = agent->parent_transform * agent->inst_m4x4[0];
			BoundingBox bbox = agent->BBox.transformCorners(trans_mat);

			if( checkAABBoxPoint(bbox, glm::vec4(point, 1.f)) ) {
				rcbuff.hit = true;
				rcbuff.pos = glm::vec4(point, 1.f);
				rcbuff.index = i;
			}
		}
	}
	else {
		DD_Agent* agent = ResSpace::findDD_Agent(res, agentID);
		if( agent ) {
			trans_mat = agent->parent_transform * agent->inst_m4x4[0];
			BoundingBox bbox = agent->BBox.transformCorners(trans_mat);

			if( checkAABBoxPoint(bbox, glm::vec4(point, 1.f)) ) {
				rcbuff.hit = true;
				rcbuff.pos = glm::vec4(point, 1.f);
				rcbuff.index = 0;
			}
		}
	}
	/*
	char char_buff[56];
	snprintf(char_buff, 56, "Hit: x: %f, y: %f, z: %f\n",
		point.x, point.y, point.z);
	DD_Terminal::post(char_buff);
	//*/

	return rcbuff;
}

glm::vec3 DD_MathLib::rayCastW(const int mouseX, const int mouseY)
{
	// calculate screen space to world space point (normalize ray direction from camera)
	DD_Camera* cam;
	bool cam_exists = ResSpace::GetActiveCamera(res, cam);
	if( !cam_exists ) {
		return glm::vec3();
	}
	glm::mat4 view = CamSpace::GetViewMatrix(cam);
	glm::mat4 proj = CamSpace::GetPerspecProjMatrix(cam);
	glm::vec4 viewport = glm::vec4(0.f, 0.f, cam->scr_width, cam->scr_height);
	const int _y = cam->scr_height - mouseY;
	GLfloat _z;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, res->G_BUFFER);
	glReadPixels(mouseX, _y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &_z);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glm::vec3 win_coord = glm::vec3(mouseX, _y - 1, _z);
	glm::vec3 win_coord2 = glm::vec3(mouseX, _y + 1, _z);
	glm::vec3 win_coord3 = glm::vec3(mouseX + 1, _y, _z);
	glm::vec3 win_coord4 = glm::vec3(mouseX - 1, _y, _z);
	glm::vec3 point = glm::unProject(win_coord, view, proj, viewport);
	glm::vec3 point2 = glm::unProject(win_coord2, view, proj, viewport);
	glm::vec3 point3 = glm::unProject(win_coord3, view, proj, viewport);
	glm::vec3 point4 = glm::unProject(win_coord4, view, proj, viewport);

	glm::vec3 avgpoint = (point + point2 + point3 + point4) / 4.f;

	return avgpoint;
}

void DD_MathLib::randomSampleRect(const glm::vec3 center, glm::vec3 min,
								  glm::vec3 max, dd_array<glm::vec3>& points)
{
	float range_x = (max.x - min.x);
	float range_z = (max.z - min.z);
	for( size_t i = 0; i < points.size(); i++ ) {
		float x_pos = (getHaltonValue(i, 2) * range_x) + min.x;
		float z_pos = (getHaltonValue(i, 3) * range_z) + min.z;
		float y_pos = center.y;
		points[i] = glm::vec3(x_pos, y_pos, z_pos);
		//printf("x: %f, y: %f, z: %f\n", x_pos, y_pos, z_pos);
	}
}

// Calculate an return all points within "Line of Sight"
size_t DD_MathLib::calculateLOSLines(const size_t index,
									 const dd_array<AI_Obstacle>& obst,
									 const dd_array<glm::vec3> other,
									 const dd_array<LinePoint>& container,
									 const dd_array<glm::vec2>& cost_bin,
									 const float agent_radius,
									 const float search_radius)
{
	glm::vec3 point = other[index];
	//printf("x: %f, y: %f, z: %f\n", point.x, point.y, point.z);
	size_t count = 0;

	for( size_t i = 0; i < other.size() && count < container.size(); i++ ) {
		if( index == i ) {
			continue;
		}
		// create line connecting points
		glm::vec3 ray = other[i] - point;
		float ray_length = glm::length(ray);
		if( search_radius > 0.f && ray_length > search_radius ) {
			continue;
		}
		// check for ray circle intersection
		dd_array<bool> hit = dd_array<bool>(obst.size());
		for( size_t j = 0; j < obst.size(); j++ ) {
			glm::vec3 pos = glm::vec3(obst[j].transform[3]);
			pos.y = 0.f;
			if( obst[j].type == 0 ) {
				// circles
				hit[j] = checkRayCircle(
					ray, point, pos, obst[j].radius + agent_radius);
			}
			else {
				// boxes
			}
		}
		size_t counter = 0;
		bool no_intersect = true;
		while( counter < obst.size() && no_intersect ) {
			if( hit[counter] == true ) {
				no_intersect = false;
			}
			counter += 1;
		}
		if( no_intersect ) {
			// save lines and information
			container[count] = {
				glm::vec4(point.x, point.y + 1.f, point.z, 1.f),
				glm::vec4(other[i].x, other[i].y + 1.f, other[i].z, 1.f)
			};
			cost_bin[count].x = (float)i;
			cost_bin[count].y = glm::length(ray);
			count += 1;
		}
	}
	return count;
}
