#include "FBX_MeshFuncs.h"
#include <vector>
#include <cmath>

enum class CurveArgs { TRANS, ROT, SCALE, X_, Y_, Z_ };

dd_array<vec2_f> DisplayCurve(FbxAnimCurve* pCurve);

/// \brief Process asset and export file w/ mesh and animation data
/// \param node FbxNode with mesh and anim information
void processAsset(FbxNode* node, AssetFBX& _asset, bool export_skeleton,
                  bool export_mesh) {
  const char* nodeName = node->GetName();

  // create new mesh w/ id
  MeshFBX mesh(nodeName);

  FbxMesh* currmesh = (FbxMesh*)node->GetMesh();
  mesh.m_ctrlpnts.resize(currmesh->GetControlPointsCount());

  // get vertex positions
  for (size_t i = 0; i < mesh.m_ctrlpnts.size(); i++) {
    CtrlPnt& cp = mesh.m_ctrlpnts[i];
    cp.m_pos.data[0] =
        static_cast<float>(currmesh->GetControlPointAt((int)i).mData[0]);
    cp.m_pos.data[1] =
        static_cast<float>(currmesh->GetControlPointAt((int)i).mData[1]);
    cp.m_pos.data[2] =
        static_cast<float>(currmesh->GetControlPointAt((int)i).mData[2]);
  }

  // get skeleton blend information
  processSkeleton(currmesh, mesh, _asset.m_skeleton);
  // get mesh buffers
  processMesh(node, mesh);
  // get all materials
  _asset.m_matbin = processMats(node);
  // tag and construct ebo buffers
  dd_array<size_t> ebos =
      connectMatToMesh(node, mesh, (uint8_t)_asset.m_matbin.size());
  // finalize asset
  _asset.addMesh(mesh, ebos);

  printf("Mesh name ='%s'(%lu)\n", mesh.m_id.str(), mesh.m_id.gethash());
  if (export_mesh) {
    _asset.exportMesh();
  }
  if (export_skeleton) {
    _asset.exportSkeleton();
  }
}

/// \brief Process node to get skeleton heirarchy
/// \param _geom FbxNode with skeletal heirarchy information
void processSkeletonAsset(FbxNode* node, const size_t index, AssetFBX& _asset) {
  bool recordbone = false;
  for (int i = 0; i < node->GetNodeAttributeCount(); i++) {
    FbxNodeAttribute* attrib = node->GetNodeAttributeByIndex(i);
    if (attrib->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton) {
      recordbone = true;
    }
  }
  // skeleton joint/limb information
  SkelFbx& _sk = _asset.m_skeleton;
  size_t next_parent = _sk.m_numJoints - 1;
  printf("Checking: %s\n", (char*)node->GetName());
  if (recordbone) {
    JointFBX& this_j = _sk.m_joints[_sk.m_numJoints];
    this_j.m_name.set((char*)node->GetName());
    this_j.m_idx = _sk.m_numJoints;
    this_j.m_parent = index;
    printf("Skeleton Name: %s (%u : %u)\n", this_j.m_name.str(), this_j.m_idx,
           this_j.m_parent);
    // increment joint counter and index of next parent
    _sk.m_numJoints += 1;
    next_parent += 1;
  }

  // loop thru children nodes for skeleton
  for (int i = 0; i < node->GetChildCount(); i++) {
    processSkeletonAsset(node->GetChild(i), next_parent, _asset);
  }
}

/// \brief Get FbxAnimCurve from CurveArgs
FbxAnimCurve* getCurve(FbxNode* node, FbxAnimLayer* animlayer,
                       const CurveArgs curvetype, const CurveArgs axis) {
  switch (curvetype) {
    case CurveArgs::TRANS:
      switch (axis) {
        case CurveArgs::X_:
          return node->LclTranslation.GetCurve(animlayer,
                                               FBXSDK_CURVENODE_COMPONENT_X);
        case CurveArgs::Y_:
          return node->LclTranslation.GetCurve(animlayer,
                                               FBXSDK_CURVENODE_COMPONENT_Y);
        case CurveArgs::Z_:
          return node->LclTranslation.GetCurve(animlayer,
                                               FBXSDK_CURVENODE_COMPONENT_Z);
        default:
          break;
      }
      break;
    case CurveArgs::ROT:
      switch (axis) {
        case CurveArgs::X_:
          return node->LclRotation.GetCurve(animlayer,
                                            FBXSDK_CURVENODE_COMPONENT_X);
        case CurveArgs::Y_:
          return node->LclRotation.GetCurve(animlayer,
                                            FBXSDK_CURVENODE_COMPONENT_Y);
        case CurveArgs::Z_:
          return node->LclRotation.GetCurve(animlayer,
                                            FBXSDK_CURVENODE_COMPONENT_Z);
        default:
          break;
      }
      break;
    case CurveArgs::SCALE:
      switch (axis) {
        case CurveArgs::X_:
          return node->LclScaling.GetCurve(animlayer,
                                           FBXSDK_CURVENODE_COMPONENT_X);
        case CurveArgs::Y_:
          return node->LclScaling.GetCurve(animlayer,
                                           FBXSDK_CURVENODE_COMPONENT_Y);
        case CurveArgs::Z_:
          return node->LclScaling.GetCurve(animlayer,
                                           FBXSDK_CURVENODE_COMPONENT_Z);
        default:
          break;
      }
      break;
    default:
      break;
  }
  return nullptr;
}

/// \brief Get keyframes from animation curve
dd_array<vec2_f> getKeyFrames(FbxAnimCurve* animCurve,
                              const unsigned numFrames) {
  dd_array<vec2_f> output(numFrames);
  FbxTime lKeyTime;
  float lKeyValue;
  char lTimeString[256];

  for (unsigned i = 0; i < numFrames; i++) {
    // get value and frame number
    lKeyValue = static_cast<float>(animCurve->KeyGetValue(i));
    lKeyTime = animCurve->KeyGetTime(i);

    char* frame_num = lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
    output[i].data[0] = std::strtod(frame_num, nullptr);
    output[i].data[1] = lKeyValue;
  }
  return output;
}

dd_array<vec2_f> getKeyFrames2(FbxAnimCurve* animCurve, const unsigned fps) {
	// get total time of animation and use to set limits
	FbxTimeSpan curve_span;
	const bool success = animCurve->GetTimeInterval(curve_span);
	
	//POW2_VERIFY_MSG(success == true, "Failed to get length of curve: %s", 
	//								animCurve->GetName());

	const double curve_length = curve_span.GetDuration().GetSecondDouble();
	unsigned num_frames = (unsigned)(curve_length / (1.0/fps));

	// check if there's a remainder frame
	bool extra_frame = false;
	if (animCurve->KeyGetCount() > (int)num_frames) {
		extra_frame = true;
		num_frames += 1;
	}

	dd_array<vec2_f> output(num_frames);
	float key_val = 0.f;
	FbxTime key_time(FBXSDK_TC_ZERO);
	const FbxLongLong fbx_frametime = FBXSDK_TC_SECOND / fps;
	int last_frame = 0;

	const unsigned f_limit = (extra_frame) ? num_frames - 1 : num_frames;
	for (unsigned i = 0; i < f_limit; i++) {
		// get value and frame number
		key_val = animCurve->Evaluate(key_time, &last_frame);

		output[i].data[0] = i;
		output[i].data[1] = key_val;

		// update time
		key_time += FbxTime(fbx_frametime);
	}

	// get remaining frame (if exists)
	if (extra_frame) {
		key_val = static_cast<float>(animCurve->KeyGetValue(num_frames - 1));
		output[num_frames - 1].data[0] = num_frames - 1;
		output[num_frames - 1].data[1] = key_val;
	}
	return output;
}

/// \brief Get animation curve data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimLayer with animation information
/// \param _asset AssetFBX that holds to be exported data
void getCurveInfo(FbxNode* node, FbxAnimLayer* animlayer, AnimClipFBX& animclip,
                  const unsigned jnt_idx, bool vicon_fix = false) {
  FbxAnimCurve* lAnimCurve = NULL;
  dd_array<vec2_f> frame_X, frame_Y, frame_Z;

  // set animations
  CurveArgs order[] = {CurveArgs::X_, CurveArgs::Y_, CurveArgs::Z_};
  dd_array<vec2_f> bin[] = {frame_X, frame_Y, frame_Z};

  for (auto& transform : {CurveArgs::ROT, CurveArgs::TRANS}) {
		//// x axis
		//lAnimCurve = getCurve(node, animlayer, transform, order[0]);
		//bin[0] = getKeyFrames2(lAnimCurve, animclip.m_framerate);
		//// y axis
		//lAnimCurve = getCurve(node, animlayer, transform, order[1]);
		//bin[1] = getKeyFrames2(lAnimCurve, animclip.m_framerate);
		//// z axis
		//lAnimCurve = getCurve(node, animlayer, transform, order[2]);
		//bin[2] = getKeyFrames2(lAnimCurve, animclip.m_framerate);



    // loop thru x, y, and z axis
    for (unsigned idx = 0; idx < 3; idx++) {
      lAnimCurve = getCurve(node, animlayer, transform, order[idx]);
      if (lAnimCurve) {
				//bin[idx] = getKeyFrames(lAnimCurve, lAnimCurve->KeyGetCount());
				bin[idx] = getKeyFrames2(lAnimCurve, animclip.m_framerate);
        // fill in animation for particular axis
        for (unsigned i = 0; i < bin[idx].size(); i++) {
          unsigned frame_num = (unsigned)bin[idx][i].x();

          // if clause should only execute once
          if (animclip.m_clip.count(frame_num) == 0) {
            animclip.m_clip[frame_num] = PoseSample();
            animclip.m_clip[frame_num].pose.resize(animclip.m_joints);
            animclip.m_clip[frame_num].logged_r.resize(animclip.m_joints);
            animclip.m_clip[frame_num].logged_t.resize(animclip.m_joints);
          }
          // fill in dictionary of animations
          dd_array<vec3_u>& log = (transform == CurveArgs::ROT)
                                      ? animclip.m_clip[frame_num].logged_r
                                      : animclip.m_clip[frame_num].logged_t;
          vec3_f& trans = (transform == CurveArgs::ROT)
                              ? animclip.m_clip[frame_num].pose[jnt_idx].rot
                              : animclip.m_clip[frame_num].pose[jnt_idx].pos;

          if (transform == CurveArgs::TRANS && vicon_fix) {
            switch (idx) {
              case 1:
                trans.data[idx + 1] = bin[idx][i].y();
                log[jnt_idx].data[idx + 1] = 1;
                break;
              case 2:
                trans.data[idx - 1] = bin[idx][i].y();
                log[jnt_idx].data[idx - 1] = 1;
                break;
              default:
                trans.data[idx] = -bin[idx][i].y();
                log[jnt_idx].data[idx] = 1;
                break;
            }
          } else {
            trans.data[idx] = bin[idx][i].y();
            log[jnt_idx].data[idx] = 1;
          }
        }
      }
    }
  }
}

/// \brief Get animation data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimLayer with animation information
/// \param _asset AssetFBX that holds to be exported data
void processAnimLayer(FbxNode* node, FbxAnimLayer* animlayer, AssetFBX& _asset,
                      AnimClipFBX& clip) {
  FbxString lOutputString;
  lOutputString = "     Node found: ";
  lOutputString += node->GetName();

  cbuff<32> node_name;
  node_name.set(node->GetName());
  bool bone_found = false;
  // only save animations from skeleton
  for (unsigned i = 0; i < _asset.m_skeleton.m_numJoints && !bone_found; i++) {
    if (_asset.m_skeleton.m_joints[i].m_name == node_name) {
      printf("%s\n", lOutputString.Buffer());
      getCurveInfo(node, animlayer, clip, i, _asset.m_viconFormat);
			bone_found = true;
    }
  }

  for (int i = 0; i < node->GetChildCount(); i++) {
    processAnimLayer(node->GetChild(i), animlayer, _asset, clip);
  }
}

/// \brief Get animation data from fbx
/// \param node FbxNode with animation information
/// \param animstack FbxAnimStack with animation information
/// \param _asset AssetFBX that holds to be exported data
void processAnimation(FbxNode* node, FbxAnimStack* animstack, AssetFBX& _asset,
                      const float framerate, const char* stack_name) {
  /// \brief Fill in missing animations
  auto fillIn = [&](std::map<unsigned, PoseSample> og_ps, const bool rotFlag,
                    const unsigned jnt_idx) {
    vec3_f last_saved = vec3_f(0, 0, 0);
    unsigned last_log = 0;
    PoseSample* ps = nullptr;

    for (unsigned k = 0; k < og_ps.size(); k++) {
      if (og_ps.count(k) > 0 && jnt_idx == 0) {
        // initialize variable for saving position and checking logs
        ps = &og_ps[k];
        last_log = k;
        dd_array<vec3_u>& check = og_ps[k].logged_t;
        vec3_f& value = ps->pose[jnt_idx].pos;
        if (rotFlag) {
          check = og_ps[k].logged_r;
          value = ps->pose[jnt_idx].rot;
        }

        // printf("\t%u : ", k);
        // check if value was key framed. If not, copy last value
        if (og_ps[k].logged_t[jnt_idx].x() == 1) {  // x axis
          // printf("a(%.3f) ", value.x());
          last_saved.x() = value.x();
        } else {
          // printf("F~(%.3f)~", last_saved.x());
          value.x() = last_saved.x();
        }
        if (og_ps[k].logged_t[jnt_idx].y() == 1) {  // y axis
          // printf("b(%.3f) ", value.y());
          last_saved.y() = value.y();
        } else {
          // printf("F~(%.3f)~", last_saved.y());
          value.y() = last_saved.y();
        }
        if (og_ps[k].logged_t[jnt_idx].z() == 1) {  // z axis
          // printf("c(%.3f) ", value.z());
          last_saved.z() = value.z();
        } else {
          // printf("F~(%.3f)~", last_saved.z());
          value.z() = last_saved.z();
        }
        // printf("\n");
      }
      // fill in completely missing frames with previous logged frame
      else if (og_ps.count(k) == 0 && jnt_idx == 0 && ps) {
        // printf("\t%u : skipped and set to %u\n", k, last_log);
        og_ps[k] = PoseSample();
        og_ps[k].pose = ps->pose;
      }
    }
  };

  int nbAnimLayers = animstack->GetMemberCount<FbxAnimLayer>();
  FbxString lOutputString;
  _asset.m_clips.resize(nbAnimLayers);

  lOutputString = "Animation stack contains ";
  lOutputString += nbAnimLayers;
  lOutputString += " Animation Layer(s)\n";
  printf("%s\n", lOutputString.Buffer());

  for (int i = 0; i < nbAnimLayers; i++) {
    FbxAnimLayer* lAnimLayer = animstack->GetMember<FbxAnimLayer>(i);

    lOutputString = stack_name;
    lOutputString += "_animLayer_";
    lOutputString += i;
    printf("%s\n\n", lOutputString.Buffer());

    _asset.m_clips[i].m_id.set(lOutputString.Buffer());
    _asset.m_clips[i].m_framerate = framerate;
    _asset.m_clips[i].m_joints = _asset.m_skeleton.m_numJoints;
    processAnimLayer(node, lAnimLayer, _asset, _asset.m_clips[i]);

    for (unsigned j = 0; j < _asset.m_clips[i].m_joints; j++) {
      // printf("%s\n", _asset.m_skeleton.m_joints[j].m_name.str());
      // fix issues with keyed animation
      //fillIn(_asset.m_clips[i].m_clip, false, j);
      //fillIn(_asset.m_clips[i].m_clip, true, j);
    }
  }
}

/// \brief Process node to get skeleton structure with tranforms
/// \param _geom FbxNode with geometry and cluster information
/// \param mesh MeshFBX mesh for modifing CtrlPnt data
/// \param _sk SkelFBX skeleton for indexing joints
void processSkeleton(FbxGeometry* _geom, MeshFBX& mesh, SkelFbx& _sk) {
  int lSkinCount = 0;
  int lClusterCount = 0;
  FbxCluster* lCluster;
  bool joint_to_world = true;

  auto matrixToStr = [](FbxAMatrix& lMatrix) {
    std::string out;
    for (size_t i = 0; i < 4; i++) {
      FbxVector4 lRow = lMatrix.GetRow(i);
      char lRowValue[1024];

      snprintf(lRowValue, 1024, "%9.4f %9.4f %9.4f %9.4f\n", lRow[0], lRow[1],
               lRow[2], lRow[3]);
      out += std::string("        ") + std::string(lRowValue);
    }
    return out;
  };

  auto printvec3f = [](const vec3_f out, const char* tag = "") {
    printf("            %.4f %.4f %.4f\t%s\n", out.x(), out.y(), out.z(), tag);
  };

  lSkinCount = _geom->GetDeformerCount(FbxDeformer::eSkin);
  for (size_t i = 0; i != (size_t)lSkinCount; i++) {
    lClusterCount = ((FbxSkin*)_geom->GetDeformer(i, FbxDeformer::eSkin))
                        ->GetClusterCount();
    for (size_t j = 0; j != (size_t)lClusterCount; j++) {
      //printf("    Cluster %lu\n", i);
      lCluster =
          ((FbxSkin*)_geom->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
      // check blending mode
      const char* lClusterModes[] = {"Normalize", "Additive", "Total1"};
      //printf("    Mode: %s\n", lClusterModes[lCluster->GetLinkMode()]);

      // get name (and joint index)
      std::string clustername = (char*)lCluster->GetLink()->GetName();
      int j_idx = -1;
      for (size_t k = 0; k < _sk.m_numJoints; k++) {
        std::string id = _sk.m_joints[k].m_name.str();
        if (id == clustername) {
          j_idx = _sk.m_joints[k].m_idx;
          break;
        }
      }
      if (lCluster->GetLink() != NULL) {
        //printf("        Name: %s (%d)\n", clustername.c_str(), j_idx);
      }

      // get matrices
      FbxAMatrix lMatrix;
      if (joint_to_world) {
        lMatrix = lCluster->GetTransformMatrix(lMatrix);
        // printf("%s\n", matrixToStr(lMatrix).c_str());
        FbxVector4 pValue1 = lMatrix.GetT();
        _sk.m_wspos =
            vec3_f(pValue1.mData[0], pValue1.mData[1], pValue1.mData[2]);
        //printvec3f(_sk.m_wspos, "global");

        pValue1 = lMatrix.GetR();
        _sk.m_wsrot =
            vec3_f(pValue1.mData[0], pValue1.mData[1], pValue1.mData[2]);
        //printvec3f(_sk.m_wsrot, "global");

        pValue1 = lMatrix.GetS();
        _sk.m_wsscl =
            vec3_f(pValue1.mData[0], pValue1.mData[1], pValue1.mData[2]);
        //printvec3f(_sk.m_wsscl, "global");

        joint_to_world = false;
      }
      lMatrix = lCluster->GetTransformLinkMatrix(lMatrix);
      // printf("%s\n", matrixToStr(lMatrix).c_str());
      FbxVector4 pValue2 = lMatrix.GetT();
      _sk.m_joints[j_idx].m_lspos =
          vec3_f(pValue2.mData[0], pValue2.mData[1], pValue2.mData[2]);
      //printvec3f(_sk.m_joints[j_idx].m_lspos, clustername.c_str());

      pValue2 = lMatrix.GetR();
      _sk.m_joints[j_idx].m_lsrot =
          vec3_f(pValue2.mData[0], pValue2.mData[1], pValue2.mData[2]);
      //printvec3f(_sk.m_joints[j_idx].m_lsrot, clustername.c_str());

      pValue2 = lMatrix.GetS();
      _sk.m_joints[j_idx].m_lsscl =
          vec3_f(pValue2.mData[0], pValue2.mData[1], pValue2.mData[2]);
      //printvec3f(_sk.m_joints[j_idx].m_lsscl, clustername.c_str());

      // get control point blending weights and joint indices
      std::string lString1 = "        Link Indices: ";
      std::string lString2 = "        Weight Values: ";
      int lIndexCount = lCluster->GetControlPointIndicesCount();
      int* jnts = lCluster->GetControlPointIndices();
      double* weights = lCluster->GetControlPointWeights();

      // assign ctrl point weight
      for (size_t k = 0; k < (size_t)lIndexCount; k++) {
        lString1 += std::to_string(jnts[k]);
        lString2 += std::to_string((float)weights[k]);
        mesh.m_ctrlpnts[jnts[k]].addBWPair(j_idx, (float)weights[k]);
        if (k < (size_t)lIndexCount - 1) {
          lString1 += ", ";
          lString2 += ", ";
        }
      }
      // printf("%s\n", lString1.c_str());
      // printf("%s\n", lString2.c_str());
    }
  }
}

/// \brief Process mesh node to in-engine mesh structure
/// \param node FbxNode with mesh information
/// \param mesh mesh structure
void processMesh(FbxNode* node, MeshFBX& mesh) {
  FbxMesh* currmesh = (FbxMesh*)node->GetMesh();
  mesh.m_triangles.resize(currmesh->GetPolygonCount());
  printf("\nNum tris: %u\n", (uint32_t)mesh.m_triangles.size());
  mesh.m_verts.resize(mesh.m_triangles.size() * 3);
  size_t vert_idx = 0;

  for (size_t i = 0; i < mesh.m_triangles.size(); i++) {
    vec3_f norm[3];
    vec3_f tang[3];
    vec3_f uv[3];
    TriFBX& _tri = mesh.m_triangles[i];
    // printf("Tri #%u\n", (unsigned int)i);

    for (int j = 0; j < 3; j++) {
      // pull information for each vertex in the triangle
      size_t cp_idx = (size_t)currmesh->GetPolygonVertex(i, j);
      CtrlPnt& currCtrlPnt = mesh.m_ctrlpnts[cp_idx];

      // position
      mesh.m_verts[vert_idx].m_pos = currCtrlPnt.m_pos;
      // uv
      getVertInfo<FbxGeometryElementUV*>(currmesh->GetElementUV(), cp_idx,
                                         currmesh->GetTextureUVIndex(i, j),
                                         uv[j]);
      mesh.m_verts[vert_idx].m_uv = vec2_f(uv[j].x(), uv[j].y());
      // normals
      getVertInfo<FbxGeometryElementNormal*>(currmesh->GetElementNormal(),
                                             cp_idx, vert_idx, norm[j]);
      mesh.m_verts[vert_idx].m_norm = norm[j];
      // tangent
      getVertInfo<FbxGeometryElementTangent*>(currmesh->GetElementTangent(),
                                              cp_idx, vert_idx, tang[j]);
      mesh.m_verts[vert_idx].m_tang = tang[j];
      // joints
      mesh.m_verts[vert_idx].m_joint = vec4_u(currCtrlPnt.m_joint);
      // blends
      mesh.m_verts[vert_idx].m_jblend = vec4_f(currCtrlPnt.m_blend);
      /*
      printf("\t pos_%u: %f, %f, %f\n", j, currCtrlPnt.m_pos[0],
              currCtrlPnt.m_pos[1], currCtrlPnt.m_pos[2]);
      printf("\tnorm_%u: %f, %f, %f\n", j, norm[j][0], norm[j][1], norm[j][2]);
      printf("\t tan_%u: %f, %f, %f\n", j, tang[j][0], tang[j][1], tang[j][2]);
      printf("\t  uv_%u: %f, %f\n", j, uv[j][0], uv[j][1]);
      */
      _tri.m_indices.data[j] = vert_idx;
      vert_idx += 1;
    }
  }
}

/// \brief Process connect material index to mesh (per-triangle)
/// \param node FbxNode with mesh information
/// \param mesh mesh structure
dd_array<size_t> connectMatToMesh(FbxNode* node, MeshFBX& mesh,
                                  const uint8_t num_mats) {
  FbxLayerElementArrayTemplate<int>* mat_idxes;
  FbxGeometryElement::EMappingMode mat_mapmode = FbxGeometryElement::eNone;
  FbxMesh* currMesh = node->GetMesh();
  // counts number of triangle per material (ebo buffer)
  dd_array<size_t> tris_in_mat(num_mats);

  if (currMesh->GetElementMaterial()) {
    mat_idxes = &(currMesh->GetElementMaterial()->GetIndexArray());
    mat_mapmode = currMesh->GetElementMaterial()->GetMappingMode();

    if (mat_idxes) {
      switch (mat_mapmode) {
        case FbxGeometryElement::eByPolygon: {
          if (mat_idxes->GetCount() == (int)mesh.m_triangles.size()) {
            for (size_t i = 0; i < mesh.m_triangles.size(); ++i) {
              size_t mat_idx = mat_idxes->GetAt(i);
              mesh.m_triangles[i].m_mat_idx = mat_idx;
              tris_in_mat[mat_idx] += 1;
            }
          }
          break;
        }
        case FbxGeometryElement::eAllSame: {
          unsigned int mat_idx = mat_idxes->GetAt(0);
          for (unsigned int i = 0; i < mesh.m_triangles.size(); ++i) {
            mesh.m_triangles[i].m_mat_idx = mat_idx;
            tris_in_mat[0] += 1;
          }
          break;
        }
        default:
          fprintf(stderr, "FBX Mat::Error::Invalid mapping index\n");
          break;
      }
    }
  }
  return tris_in_mat;
}

/// \brief Process all materials for mesh
/// \param node FbxNode with mesh information
/// \param mats material container
dd_array<MatFBX> processMats(FbxNode* node) {
  size_t mat_count = node->GetMaterialCount();
  dd_array<MatFBX> mats(mat_count);

  auto transferD3 = [&](FbxDouble3& source, vec3_f& sink) {
    sink.x() = static_cast<float>(source.mData[0]);
    sink.y() = static_cast<float>(source.mData[1]);
    sink.z() = static_cast<float>(source.mData[2]);
  };

  auto flushSink = [](vec3_f& sink) { sink.x() = sink.y() = sink.z() = 0.f; };

  auto getTexChannels = [&](FbxProperty& _prop, const int m_idx,
                            const MatType type) {
    // fill in textures and set flag
    int num_texts = _prop.GetSrcObjectCount<FbxTexture>();
    MatFBX& _m = mats[m_idx];
    if (num_texts > 0) {
      for (int i = 0; i < num_texts; i++) {
        FbxTexture* tex = _prop.GetSrcObject<FbxTexture>(i);
        FbxFileTexture* file_tex = FbxCast<FbxFileTexture>(tex);
        if (tex) {
          switch ((int)type) {
            case (int)MatType::DIFF:
              _m.m_diffmap.set(file_tex->GetFileName());
              _m.m_textypes |= MatType::DIFF;
              break;
            case (int)MatType::SPEC:
              _m.m_specmap.set(file_tex->GetFileName());
              _m.m_textypes |= MatType::SPEC;
              break;
            case (int)MatType::EMIT:
              _m.m_emitmap.set(file_tex->GetFileName());
              _m.m_textypes |= MatType::EMIT;
              break;
            case (int)MatType::NORMAL:
              _m.m_normmap.set(file_tex->GetFileName());
              _m.m_textypes |= MatType::NORMAL;
              break;
            default:
              break;
          }
        }
      }
    }
  };

  // grab material name and base atrributes
  for (size_t i = 0; i < mat_count; i++) {
    FbxSurfaceMaterial* surface_mat = node->GetMaterial(i);
    mats[i] = MatFBX(surface_mat->GetName());

    FbxDouble3 _d3;
    FbxDouble _d1;
    FbxProperty lProperty;

    if (surface_mat->GetClassId().Is(FbxSurfaceLambert::ClassId)) {
      // ambient
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Ambient;
      transferD3(_d3, mats[i].m_ambient);

      // diffuse
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Diffuse;
      transferD3(_d3, mats[i].m_diffuse);
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
      getTexChannels(lProperty, i, MatType::DIFF);

      // emissive
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Emissive;
      transferD3(_d3, mats[i].m_emmisive);
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sEmissive);
      getTexChannels(lProperty, i, MatType::EMIT);

      // normal
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
      getTexChannels(lProperty, i, MatType::NORMAL);

      // specular
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sSpecular);
      getTexChannels(lProperty, i, MatType::SPEC);

      // Transparency factor
      _d1 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->TransparencyFactor;
      mats[i].m_transfactor = static_cast<float>(_d1);

      flushSink(mats[i].m_specular);
      mats[i].m_specfactor = 0.f;
      mats[i].m_reflectfactor = 0.f;
    }
    if (surface_mat->GetClassId().Is(FbxSurfacePhong::ClassId)) {
      // ambient
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Ambient;
      transferD3(_d3, mats[i].m_ambient);

      // diffuse
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Diffuse;
      transferD3(_d3, mats[i].m_diffuse);
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
      getTexChannels(lProperty, i, MatType::DIFF);

      // emissive
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Emissive;
      transferD3(_d3, mats[i].m_emmisive);
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sEmissive);
      getTexChannels(lProperty, i, MatType::EMIT);

      // normal
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
      getTexChannels(lProperty, i, MatType::NORMAL);

      // Transparency factor
      _d1 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->TransparencyFactor;
      mats[i].m_transfactor = static_cast<float>(_d1);

      // specular
      _d3 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->Specular;
      transferD3(_d3, mats[i].m_specular);
      lProperty = surface_mat->FindProperty(FbxSurfaceMaterial::sShininess);
      getTexChannels(lProperty, i, MatType::SPEC);

      // reflection factor
      _d1 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->ReflectionFactor;
      mats[i].m_reflectfactor = static_cast<float>(_d1);

      // specular factor
      _d1 = reinterpret_cast<FbxSurfacePhong*>(surface_mat)->SpecularFactor;
      mats[i].m_specfactor = static_cast<float>(_d1);
    }
  }
  return mats;
}

/// \brief Extract animation data from curves
dd_array<vec2_f> DisplayCurve(FbxAnimCurve* pCurve) {
  dd_array<vec2_f> output;

  static const char* interpolation[] = {"?", "constant", "linear", "cubic"};
  static const char* constantMode[] = {"?", "Standard", "Next"};
  static const char* cubicMode[] = {"?",    "Auto",  "Auto break", "Tcb",
                                    "User", "Break", "User break"};
  static const char* tangentWVMode[] = {"?", "None", "Right", "Next left"};

  auto InterpolationFlagToIndex = [](int flags) {
    if ((flags & FbxAnimCurveDef::eInterpolationConstant) ==
        FbxAnimCurveDef::eInterpolationConstant) {
      return 1;
    }
    if ((flags & FbxAnimCurveDef::eInterpolationLinear) ==
        FbxAnimCurveDef::eInterpolationLinear) {
      return 2;
    }
    if ((flags & FbxAnimCurveDef::eInterpolationCubic) ==
        FbxAnimCurveDef::eInterpolationCubic) {
      return 3;
    }
    return 0;
  };

  auto ConstantmodeFlagToIndex = [](int flags) {
    if ((flags & FbxAnimCurveDef::eConstantStandard) ==
        FbxAnimCurveDef::eConstantStandard) {
      return 1;
    }
    if ((flags & FbxAnimCurveDef::eConstantNext) ==
        FbxAnimCurveDef::eConstantNext) {
      return 2;
    }
    return 0;
  };

  auto TangentmodeFlagToIndex = [](int flags) {
    if ((flags & FbxAnimCurveDef::eTangentAuto) ==
        FbxAnimCurveDef::eTangentAuto) {
      return 1;
    }
    if ((flags & FbxAnimCurveDef::eTangentAutoBreak) ==
        FbxAnimCurveDef::eTangentAutoBreak) {
      return 2;
    }
    if ((flags & FbxAnimCurveDef::eTangentTCB) ==
        FbxAnimCurveDef::eTangentTCB) {
      return 3;
    }
    if ((flags & FbxAnimCurveDef::eTangentUser) ==
        FbxAnimCurveDef::eTangentUser) {
      return 4;
    }
    if ((flags & FbxAnimCurveDef::eTangentGenericBreak) ==
        FbxAnimCurveDef::eTangentGenericBreak) {
      return 5;
    }
    if ((flags & FbxAnimCurveDef::eTangentBreak) ==
        FbxAnimCurveDef::eTangentBreak) {
      return 6;
    }
    return 0;
  };

  auto TangentweightFlagToIndex = [](int flags) {
    if ((flags & FbxAnimCurveDef::eWeightedNone) ==
        FbxAnimCurveDef::eWeightedNone) {
      return 1;
    }
    if ((flags & FbxAnimCurveDef::eWeightedRight) ==
        FbxAnimCurveDef::eWeightedRight) {
      return 2;
    }
    if ((flags & FbxAnimCurveDef::eWeightedNextLeft) ==
        FbxAnimCurveDef::eWeightedNextLeft) {
      return 3;
    }
    return 0;
  };

  auto TangentVelocityFlagToIndex = [](int flags) {
    if ((flags & FbxAnimCurveDef::eVelocityNone) ==
        FbxAnimCurveDef::eVelocityNone) {
      return 1;
    }
    if ((flags & FbxAnimCurveDef::eVelocityRight) ==
        FbxAnimCurveDef::eVelocityRight) {
      return 2;
    }
    if ((flags & FbxAnimCurveDef::eVelocityNextLeft) ==
        FbxAnimCurveDef::eVelocityNextLeft) {
      return 3;
    }
    return 0;
  };

  FbxTime lKeyTime;
  float lKeyValue;
  char lTimeString[256];
  FbxString lOutputString;
  int lCount;

  int lKeyCount = pCurve->KeyGetCount();
  output.resize(lKeyCount);

  for (lCount = 0; lCount < lKeyCount; lCount++) {
    lKeyValue = static_cast<float>(pCurve->KeyGetValue(lCount));
    lKeyTime = pCurve->KeyGetTime(lCount);

    lOutputString = "            Key Time: ";
    lOutputString += lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
    lOutputString += ".... Key Value: ";
    lOutputString += lKeyValue;
    lOutputString += " [ ";
    lOutputString += interpolation[InterpolationFlagToIndex(
        pCurve->KeyGetInterpolation(lCount))];

    // get value and frame number
    char* frame_num = lKeyTime.GetTimeString(lTimeString, FbxUShort(256));
    output[lCount].data[0] = std::strtod(frame_num, nullptr);
    output[lCount].data[1] = lKeyValue;

    if ((pCurve->KeyGetInterpolation(lCount) &
         FbxAnimCurveDef::eInterpolationConstant) ==
        FbxAnimCurveDef::eInterpolationConstant) {
      lOutputString += " | ";
      lOutputString += constantMode[ConstantmodeFlagToIndex(
          pCurve->KeyGetConstantMode(lCount))];
    } else if ((pCurve->KeyGetInterpolation(lCount) &
                FbxAnimCurveDef::eInterpolationCubic) ==
               FbxAnimCurveDef::eInterpolationCubic) {
      lOutputString += " | ";
      lOutputString +=
          cubicMode[TangentmodeFlagToIndex(pCurve->KeyGetTangentMode(lCount))];
      lOutputString += " | ";
      lOutputString += tangentWVMode[TangentweightFlagToIndex(
          pCurve->KeyGet(lCount).GetTangentWeightMode())];
      lOutputString += " | ";
      lOutputString += tangentWVMode[TangentVelocityFlagToIndex(
          pCurve->KeyGet(lCount).GetTangentVelocityMode())];
    }
    lOutputString += " ]";
    lOutputString += "\n";
    // printf("%s\n", lOutputString.Buffer());
  }
  return output;
}
