#pragma once
#include "FBX_Utility.h"
#include <typeinfo>
#include <string>

void processAsset(FbxNode* node, 
				  AssetFBX &_asset, 
				  bool export_skeleton,
				  bool export_mesh);
void processSkeletonAsset(FbxNode *node, const size_t index, AssetFBX &_asset);
void processAnimation(FbxNode *node,
					  FbxAnimStack *animstack,
					  AssetFBX &_asset,
					  const float framerate,
					  const char* stack_name);

// functions for mesh and animation parsing
void processMesh(FbxNode *node, MeshFBX &new_mesh);
dd_array<size_t> connectMatToMesh(FbxNode *node, MeshFBX &mesh,
								  const uint8_t num_mats);
dd_array<MatFBX> processMats(FbxNode *node);
void processSkeleton(FbxGeometry *_geom, MeshFBX &mesh, SkelFbx& _sk);

template<class T>
void getVertInfo(T info_type,
				 const size_t ctrlpnt_idx,
				 const size_t polyvert_idx,
				 vec3_f &vec)
{
	if (!info_type) {
		vec.x() = 0;
		vec.y() = 0;
		vec.z() = 0;
		return;
	}
	std::string type_id = typeid(T).name();
	
	// The y coordinate of the UV is flipped because SOIL image lbrary reads
	// textures upside down
	switch( info_type->GetMappingMode() ) {
		case FbxGeometryElement::eByControlPoint:
			switch( info_type->GetReferenceMode() ) {
				case FbxGeometryElement::eDirect:
					if( typeid(T) == typeid(FbxGeometryElementUV*) ) {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(ctrlpnt_idx).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(ctrlpnt_idx).mData[1]);
						vec.z() = 0.f;
					}
					else {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(ctrlpnt_idx).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(ctrlpnt_idx).mData[1]);
						vec.z() = static_cast<float>(
							info_type->GetDirectArray().GetAt(ctrlpnt_idx).mData[2]);
					}
					break;
				case FbxGeometryElement::eIndexToDirect:
				{
					int index = info_type->GetIndexArray().GetAt(ctrlpnt_idx);
					if(typeid(T) == typeid(FbxGeometryElementUV*)) {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[1]);
						vec.z() = 0.f;
					}
					else {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[1]);
						vec.z() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[2]);
					}
					break;
				}
				default:
					fprintf(stderr, "Invalid Reference");
					break;
			}
			break;
		case FbxGeometryElement::eByPolygonVertex:
			switch( info_type->GetReferenceMode() ) {
				case FbxGeometryElement::eDirect:
					if(typeid(T) == typeid(FbxGeometryElementUV*)) {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[1]);
						vec.z() = 0.f;
					}
					else {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[1]);
						vec.z() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[2]);
					}
					break;
				case FbxGeometryElement::eIndexToDirect:
				{
					int index = info_type->GetIndexArray().GetAt(ctrlpnt_idx);
					if(typeid(T) == typeid(FbxGeometryElementUV*)) {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(polyvert_idx).mData[1]);
						vec.z() = 0.f;
					}
					else {
						vec.x() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[0]);
						vec.y() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[1]);
						vec.z() = static_cast<float>(
							info_type->GetDirectArray().GetAt(index).mData[2]);
					}
					break;
				}
				default:
					fprintf(stderr, "Invalid Reference");
					break;
			}
			break;
		default:
			break;
	}
}
