#pragma once

#include <DD_Types.h>

#define MAX_JOINTS ((uint8_t)-1)

/// \brief Skeletal bone base structure
struct DD_Joint {
	/// \brief inverse bind pose matrix
  glm::mat4 inv_bp;   
	/// \brief transform from parent to this joint
  glm::mat4 p_delta;
  cbuff<64> id;
  u8 parent;
};

/// \brief Simple Skeleton container
struct DD_Skeleton {
  /// \brief Engine identifier assigned at initialization
  size_t id;
	/// \brief global model matrix from file
  glm::mat4 global_mat;
  dd_array<DD_Joint> bones;
};

/// \brief Joint transformations (only concered w/ skeletons w/ joint rotations)
struct DD_JointPose {
  glm::quat rot = glm::quat();
  glm::vec3 trans = glm::vec3();
  // float       m_scale;
};

/// \brief Skeleton pose (ID and a list of joint rotations)
struct DD_SkeletonPose {
  /// \brief Engine identifier assigned at initialization
  size_t id;
	/// \brief skeleton id
	size_t sk_id;
	/// \brief modifiable global matrix to apply to skeleton
  glm::mat4 global_mat;
	/// \brief scratch buffer for local pose data
  dd_array<DD_JointPose> local_pose;
	/// \brief per-frame calculated global pose data
	dd_array<DD_JointPose> global_pose;
	/// \brief Marks if imported animation uses global matrices or local
	bool global_poses = false;
};