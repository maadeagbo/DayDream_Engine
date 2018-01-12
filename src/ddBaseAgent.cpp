#include "ddBaseAgent.h"

ddAgent::ddAgent() {}

ddAgent::~ddAgent() {}

namespace {
	DD_FuncBuff fb;
}

namespace ddBodyFuncs {

glm::vec3 pos(const ddBody* bod) {
  // local position
  btVector3 center = bod->bt_bod->getCenterOfMassPosition();
  return glm::vec3(center.x(), center.y(), center.z());
}

glm::vec3 pos_ws(const ddBody * bod) {
	// world position
	btVector3 ws = bod->bt_bod->getWorldTransform().getOrigin();
	return glm::vec3(ws.x(), ws.y(), ws.z());
}

glm::quat rot(const ddBody* bod) {
  // local transform
  btTransform tr = bod->bt_bod->getCenterOfMassTransform();
  btQuaternion q = tr.getRotation();
  return glm::quat(q.x(), q.y(), q.z(), q.w());
}

glm::quat rot_ws(const ddBody * bod) {
	// world transform
	btTransform tr = bod->bt_bod->getWorldTransform();
	btQuaternion q = tr.getRotation();
	return glm::quat(q.x(), q.y(), q.z(), q.w());
}

glm::vec3 forward_dir(const ddBody * bod, const glm::quat q) {
	return glm::normalize(q * world_front);
}

void update_pos(ddBody * bod, const glm::vec3 & pos) {
	// local rotation
	const glm::quat q = ddBodyFuncs::rot(bod);

	// set tranform
	btTransform tr;
	tr.setOrigin(btVector3(pos.x, pos.y, pos.z));
	tr.setRotation(btQuaternion(q.x, q.y, q.z, q.z));
	bod->bt_bod->setCenterOfMassTransform(tr);
}

void rotate(ddBody* bod, const glm::vec3& _euler) {
  // local rotation
  const glm::quat l_rot = ddBodyFuncs::rot(bod);
  const btQuaternion q1 = btQuaternion((btScalar)l_rot.x, (btScalar)l_rot.y,
                                       (btScalar)l_rot.z, (btScalar)l_rot.w);
  // new rotation
  const btQuaternion q2((btScalar)_euler.y, (btScalar)_euler.x,
                        (btScalar)_euler.z);
	// local translation
	const glm::vec3 p1 = ddBodyFuncs::pos(bod);
	
	// set transform
	btTransform tr;
	tr.setOrigin(btVector3(p1.x, p1.y, p1.z));
	tr.setRotation(q1 * q2);
	bod->bt_bod->setCenterOfMassTransform(tr);
}

glm::mat4 get_model_mat(ddBody * bod) {
	const glm::vec3 pos = ddBodyFuncs::pos_ws(bod);
	const glm::quat q = ddBodyFuncs::rot_ws(bod);
	const glm::vec3 sc = bod->scale;

	glm::mat4 _t = glm::translate(glm::mat4(), pos);
	glm::mat4 _r = glm::mat4_cast(q);
	glm::mat4 _s = glm::scale(glm::mat4(), sc);

	return _t * _r * _s;
}

AABB get_aabb(ddBody * bod) {
	AABB bbox;
	
	btVector3 min, max;
	bod->bt_bod->getAabb(min, max);
	bbox.min = glm::vec3(min.x(), min.y(), min.z());
	bbox.max = glm::vec3(max.x(), max.y(), max.z());
	
	return bbox;
}

}  // namespace ddBodyFuncs
