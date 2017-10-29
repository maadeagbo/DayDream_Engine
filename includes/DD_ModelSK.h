#pragma once

#include <DD_Types.h>
#include <DD_Model.h>
#include <DD_AnimState.h>

/// \brief Container for skinned meshes
struct DD_ModelSK : public DD_Model
{
    bool debugStatus() const { return m_flagDebug; }
    void debugOn();
    void debugOff();

    DD_SkeletonPose m_finalPose;
    dd_array<DD_AnimState> m_animStates;
    dd_array<glm::mat4> m_debugSkeleton;
	bool m_daltonFlag = false;
private:
    bool m_flagDebug = false;
};

namespace ModelSKSpace
{
	BoundingBox CalculateBBox(const DD_ModelSK& model);
    void OpenGLBindMesh(const unsigned index,
                        DD_ModelSK& model,
                        const unsigned inst_size,
                        const unsigned inst_c_size);
    void OpenGLUnBindMesh(const unsigned index, DD_ModelSK& model);
}
