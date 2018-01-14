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

glm::vec3 pos_ws(const ddBody* bod) {
  // world position
  btTransform tr = bod->bt_bod->getWorldTransform();
  btVector3 ws = tr.getOrigin();
  return glm::vec3(ws.x(), ws.y(), ws.z());
}

glm::quat rot(const ddBody* bod) {
  // local transform
  btTransform tr = bod->bt_bod->getCenterOfMassTransform();
  btQuaternion q = tr.getRotation();
  return glm::quat(q.x(), q.y(), q.z(), q.w());
}

glm::quat rot_ws(const ddBody* bod) {
  // world transform
  btTransform tr = bod->bt_bod->getWorldTransform();
  btQuaternion q = tr.getRotation();
  return glm::quat(q.x(), q.y(), q.z(), q.w());
}

glm::vec3 forward_dir(const ddBody* bod, const glm::quat q) {
  glm::vec4 _f =
      q * glm::vec4(world_front.x, world_front.y, world_front.z, 1.f);
  return glm::normalize(glm::vec3(_f));
}

void update_pos(ddBody* bod, const glm::vec3& pos) {
  // local rotation
  const glm::quat q = ddBodyFuncs::rot(bod);

  // set tranform
  btTransform tr;
  tr.setOrigin(btVector3(pos.x, pos.y, pos.z));
  tr.setRotation(btQuaternion(q.x, q.y, q.z, q.z));
  bod->bt_bod->setWorldTransform(tr);
}

void rotate(ddBody* bod, const glm::vec3& _euler) {
  btTransform tr;
  // local rotation
  const btQuaternion q1 = bod->bt_bod->getCenterOfMassTransform().getRotation();
  // new rotation
  btQuaternion q2((btScalar)_euler.y, (btScalar)_euler.z, (btScalar)_euler.x);
  q2 *= q1;
  // local translation
  const glm::vec3 p1 = ddBodyFuncs::pos(bod);

  // set new transform
  tr.setIdentity();
  tr.setOrigin(btVector3(p1.x, p1.y, p1.z));
  tr.setRotation(q2);
  bod->bt_bod->setWorldTransform(tr);
}

void update_scale(ddBody* bod, const glm::vec3& _scale) {
  bod->scale = _scale;
  // set scale in collision shape for physics
  bod->bt_bod->getCollisionShape()->setLocalScaling(
      btVector3((btScalar)_scale.x, (btScalar)_scale.y, (btScalar)_scale.z));
  // btCollisionShape::setLocalScaling()
  // btCollisionWorld::updateSingleAABB( rigidbody )
}

glm::mat4 get_model_mat(ddBody* bod) {
  const glm::vec3 pos = ddBodyFuncs::pos_ws(bod);
  const glm::quat q = ddBodyFuncs::rot_ws(bod);
  const glm::vec3 sc = bod->scale;

  glm::mat4 _t = glm::translate(glm::mat4(), pos);
  glm::mat4 _r = glm::mat4_cast(q);
  glm::mat4 _s = glm::scale(glm::mat4(), sc);

  return _t * _r * _s;
}

AABB get_aabb(ddBody* bod) {
  AABB bbox;

  btVector3 min, max;
  bod->bt_bod->getAabb(min, max);
  bbox.min = glm::vec3(min.x(), min.y(), min.z());
  bbox.max = glm::vec3(max.x(), max.y(), max.z());

  return bbox;
}

}  // namespace ddBodyFuncs
