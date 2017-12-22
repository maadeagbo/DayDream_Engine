#pragma once

#include <DD_Types.h>

#define MAX_JOINTS ((uint8_t)-1)

/// \brief Skeletal bone base structure
struct DD_Joint {
  glm::mat4 m_invBP;   //< inverse bind pose matrix
  glm::mat4 m_pDelta;  //< transform from parent to this joint
  cbuff<64> m_ID;
  u8 m_parent;
};

/// \brief Simple Skeleton container
struct DD_Skeleton {
	/// \brief Engine identifier assigned at initialization
	size_t id;

  cbuff<64> m_ID;
  glm::mat4 m_globalMat;
  dd_array<DD_Joint> m_bones;
};

/// \brief Joint transformations (only concered w/ skeletons w/ joint rotations)
struct DD_JointPose {
  glm::quat m_rot = glm::quat();
  glm::vec3 m_trans = glm::vec3();
  // float       m_scale;
};

/// \brief Skeleton pose (ID and a list of joint rotations)
struct DD_SkeletonPose {
	/// \brief Engine identifier assigned at initialization
	size_t id;
	/// \brief DD_Skeleton identifier
	size_t skeleton_id;

  cbuff<64> m_skeletonID;
  glm::mat4 m_globalMat;
  dd_array<DD_JointPose> m_localPose;
  dd_array<glm::mat4> m_globalPose;
  dd_array<glm::mat4> m_invBPs;
};