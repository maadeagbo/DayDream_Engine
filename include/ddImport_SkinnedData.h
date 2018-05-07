#pragma once

#include "LuaHooks.h"
#include "ddAssets.h"
#include "ddFileIO.h"
#include "ddIncludes.h"

/**
 * \brief Create ddSkeleton from ddb file
 * \param ddb_file Path to .ddb file
 * \param id String id name
 * \return ddSkeleton pointer
 */
ddSkeleton *load_skeleton(const char *ddb_file, const char *id);

/**
 * \brief Recursively calculate delta transform between bone joints
 * \param sk ddSkeleton object
 * \param visited Flag container to mark if tree node has been calculated
 * \param idx Node/bone position to calucaulate
 */
void calc_delta_bone_trans(ddSkeleton *sk, bool visited[], const unsigned idx);

/**
 * \brief Load animation from a dda file
 * \param dda_file Path to .dda file
 * \param id String id name
 * \return ddSkeleton pointer
 */
ddAnimClip *load_animation(const char *dda_file, const char *id);