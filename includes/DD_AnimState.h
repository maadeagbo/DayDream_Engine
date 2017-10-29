#pragma once

#include <DD_Types.h>
#include <DD_Skeleton.h>

/// \brief Sample from animation clip
struct DD_AnimSample
{
    dd_array<DD_JointPose> m_pose;
};

/// \brief Information contained in animation clip
struct DD_AnimClip
{
    cbuff<64>   m_ID;
	unsigned    num_joints = 0;
	unsigned    num_frames = 0;
    float       fps;
    float       length = 0.f;
	float		step_size = 0.f;
	dd_array<DD_AnimSample> samples;
};

/// \brief Container to hold animation clip information
struct DD_AnimState
{
    cbuff<64>   m_ID;
    cbuff<64>   clip_id;
    float       weight;
    float       local_time = 0.f;
    float       play_back = 1.f;
	bool		interpolate = true;
    bool        active = false;
	bool        flag_loop = false;
	bool		pause = false;
	dd_array<float> fscratch;
};