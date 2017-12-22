#pragma once

#include <DD_EventQueue.h>
#include <DD_ModelSK.h>
#include <DD_ResourceLoader.h>
#include <DD_Types.h>

/// \brief Animations are updated in this entity (only called thru event update)
class DD_AnimSystem {
 public:
  DD_AnimSystem() {}
  ~DD_AnimSystem() {}

  DD_Resources* res_ptr;
  PushFunc push;

  DD_Event update(DD_Event& event);

  void anim_update(DD_LEvent& _event);

 private:
  void processAnimState(DD_ModelSK* mdlsk, const DD_Skeleton* sk,
                        const float time);
};