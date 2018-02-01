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
  glm::mat4 _r;
  bod->bt_bod->getCenterOfMassTransform().getBasis().getOpenGLSubMatrix(
      &_r[0][0]);
  return glm::quat_cast(_r);
}

glm::quat rot_ws(const ddBody* bod) {
  // world transform
  glm::mat4 _r;
  bod->bt_bod->getWorldTransform().getBasis().getOpenGLSubMatrix(&_r[0][0]);
  return glm::quat_cast(_r);
}

glm::vec3 forward_dir(const ddBody* bod) {
  glm::quat q = ddBodyFuncs::rot_ws(bod);
  glm::vec4 _f =
      q * glm::vec4(world_front.x, world_front.y, world_front.z, 1.f);
  return glm::normalize(glm::vec3(_f));
}

void update_velocity(ddBody* bod, const glm::vec3& vel) {
  // bod->bt_bod->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));

  // btMatrix3x3& boxRot = bod->bt_bod->getWorldTransform().getBasis();
  // btVector3 correctedForce = boxRot * btVector3(vel.x, vel.y, vel.z);

  bod->bt_bod->activate(true);
  bod->bt_bod->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
}

void update_pos(ddBody* bod, const glm::vec3& pos) {
  // local rotation
  const btQuaternion q1 = bod->bt_bod->getCenterOfMassTransform().getRotation();

  // set tranform
  btTransform tr;
  tr.setOrigin(btVector3(pos.x, pos.y, pos.z));
  tr.setRotation(q1);
  bod->bt_bod->setWorldTransform(tr);
  bod->bt_bod->getMotionState()->setWorldTransform(tr);

  // clear forces to remove "portal-like" motion
  // bod->bt_bod->clearForces();
  // bod->bt_bod->setLinearVelocity(btVector3(0, 0, 0));
  // bod->bt_bod->setAngularVelocity(btVector3(0, 0, 0));
}

void rotate(ddBody* bod, const float yaw, const float pitch, const float roll) {
  btTransform tr;
  // local rotation
  // const btQuaternion q1 =
  // bod->bt_bod->getCenterOfMassTransform().getRotation();

  // set new transform
  tr.setIdentity();
  const glm::vec3 p1 = ddBodyFuncs::pos_ws(bod);
  tr.setOrigin(btVector3(p1.x, p1.y, p1.z));

  // new rotation
  btQuaternion q2;
  q2.setEuler(yaw, pitch, roll);
  tr.setRotation(q2);

  bod->bt_bod->setWorldTransform(tr);
  // bod->bt_bod->getMotionState()->setWorldTransform(tr);

  // bod->bt_bod->applyTorque(btVector3(torque.x, torque.y, torque.z));
  // bod->bt_bod->setAngularVelocity(btVector3(torque.x, torque.y, torque.z));
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
  const glm::vec3 sc = bod->scale;
  glm::mat4 _s = glm::scale(glm::mat4(), sc);
  glm::mat4 tr;
  bod->bt_bod->getWorldTransform().getOpenGLMatrix(&tr[0][0]);

  return tr * _s;
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
