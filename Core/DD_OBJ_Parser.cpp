// Moses Adeagbo 07/24/2016 Obj Parser
#include <DD_OBJ_Parser.h>
#include "DD_Terminal.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cmath>
#include <limits>
#include <string>
#include <cstring>
#include <omp.h>

// parsing specific variables
namespace {
	const std::string whitespace = " \t", dot = ".", slash = "/";
	obj_vec3 empty = obj_vec3();
	char buff[512];

	//std::map<buffer16, int> index_buffer;
	std::string TrimWhiteSpace(const std::string& str)
	{
		const size_t strBegin = str.find_first_not_of(whitespace);
		if( strBegin == std::string::npos ) {
			return str;                 // no leading whitespaces
		}
		char lastChar = str.back();
		if( lastChar == ' ' ) {
			std::string tempStr = str.substr(strBegin);
			tempStr.pop_back();
			return tempStr;				// no leading or trailing whitespaces
		}
		return str.substr(strBegin);
	}

	std::string GetLineID(const std::string& str)
	{
		const size_t strBegin = str.find_first_of(whitespace);
		if( strBegin == std::string::npos ) {
			return "";                 // no leading whitespaces
		}
		return str.substr(0, strBegin);
	}
}

struct FaceVerts
{
	size_t num_tris = 0;
	Vertex verts[6];
	std::string index[6];
};

// MTL file parser
bool ProcessMTLFile(ObjAsset& obj);

// utility function signatures
obj_vec3 GetFloatData(const std::string str);
Vertex CreateVertex(const obj_vec3& pos, const obj_vec3& normal,
					const obj_vec3& texC, const obj_vec3& tang);
size_t CountTrianglesFromFaceInfo(const std::string& str);
FaceStyle GetFaceStyleFromFaceInfo(const std::string& str);
Vertex GetVertexFromFaceInfo(const std::string& str, FaceStyle style,
							 ObjAsset& obj);
FaceVerts SetMultiMeshFaceData(const std::string& data, ObjAsset& obj,
							   const size_t meshIndex);

// Read file and log statistics
ObjAssetParser::ParseFlag ObjAssetParser::PreProcess(
	ObjAsset& obj, const char * obj_full_path, const char* mtlName)
{
	ParseFlag pFlag;

	std::ifstream file(obj_full_path);
	if( file.good() ) {
		std::stringstream analysisBuf;
		analysisBuf << file.rdbuf();
		// store in file buffer
		const std::string tempStr = analysisBuf.str();
		obj.file_buffer = new char[tempStr.length() + 1]();
		std::strcpy(
			obj.file_buffer,
			tempStr.c_str()
		);
		file.close();
		std::string line;
		// get directory of obj file
		const std::string tempPath = obj_full_path;
		std::string objName = "";
		size_t dirEnd = tempPath.find_last_of("/");
		if( dirEnd != std::string::npos ) {
			obj.info.directory = tempPath.substr(0, dirEnd + 1);
			objName = tempPath.substr(dirEnd + 1);
			objName = objName.erase(objName.length() - 4); // remove ".obj"
		}
		snprintf(buff, 512, "Loading: %s\n", objName.c_str());
		DD_Terminal::post(buff);

		// check if <mesh name>.ddmesh file exists
		std::string ddmeshName = obj.info.directory + objName + ".ddmesh";
		if( FileExists(ddmeshName.c_str()) ) {
			//printf("Loading: %s\n", ddmeshName.c_str());
			//obj.info.ddMeshName = objName + ".ddmesh";
			//// return to use separate parser
			//pFlag.success = true;
			//pFlag.ddMesh = true;
			//return pFlag;
		}
		else {
			//printf("Creating %s for future use.\n", ddmeshName.c_str());
			obj.info.ddMeshName = objName + ".ddmesh";
			//std::ofstream outfile(ddmeshName.c_str());
			//outfile << "ID " << objName.c_str() << "\n";
			//outfile << "DIR " << obj.info.directory.c_str() << "\n";
			//outfile.close();
		}

		// count number of meshes and get info
		int oldIndex = -1;
		while( std::getline(analysisBuf, line) ) {
			// remove whitespace
			line = TrimWhiteSpace(line);
			std::string lineID = GetLineID(line);

			if( lineID == "usemtl" ) {
				// set mesh indices (lines numbers for faces in submesh)
				obj.info.mesh_indices[obj.info.num_meshes].start =
					obj.info.num_f;
				if( obj.info.num_meshes != 0 ) {
					obj.info.mesh_indices[obj.info.num_meshes - 1].end =
						obj.info.num_f - 1;
				}
				// set material id
				obj.info.mat_IDs[obj.info.num_meshes] = line.substr(7);
				obj.info.num_meshes += 1;
			}
			else if( lineID == "v" ) {
				obj.info.num_v += 1;
			}
			else if( lineID == "vn" ) {
				obj.info.num_vn += 1;
			}
			else if( lineID == "vt" ) {
				obj.info.num_vt += 1;
			}
			else if( lineID == "f" ) {
				// count number of triangles in face
				obj.info.mesh_indices[obj.info.num_meshes - 1].tris +=
					CountTrianglesFromFaceInfo(line.substr(2));
				// set face style
				if( obj.info.num_meshes != (size_t)oldIndex ) {
					obj.info.f_style[obj.info.num_meshes - 1] =
						GetFaceStyleFromFaceInfo(line.substr(2));
					oldIndex = (int)obj.info.num_meshes;
				}
				obj.info.num_f += 1;
			}
			else if( lineID == "mtllib" ) {
				// Get filename of mtl file
				size_t strBegin = line.find_first_of(whitespace);
				size_t strEnd = line.find_first_of(dot);
				size_t range = strEnd - strBegin - 1;
				obj.info.mtl_filename = line.substr(strBegin + 1, range);
			}
		}
		// set last index in last submesh
		obj.info.mesh_indices[obj.info.num_meshes - 1].end =
			obj.info.num_f - 1;

		// resize mesh indices array (because its initialized w/ 50 meshes)
		dd_array<MeshIndex> temp(obj.info.num_meshes);
		temp = obj.info.mesh_indices;
		obj.info.mesh_indices.resize(obj.info.num_meshes);
		obj.info.mesh_indices = std::move(temp);
		// also resize material ID array
		dd_array<std::string> temp2(obj.info.num_meshes);
		temp2 = obj.info.mat_IDs;
		obj.info.mat_IDs.resize(obj.info.num_meshes);
		obj.info.mat_IDs = std::move(temp2);

		// read matl file if present
		if( std::string(mtlName).compare("") != 0 ) {
			obj.info.mtl_filename = mtlName;
		}
		else {
			obj.info.mtl_filename = "not_set";
		}
		ProcessMTLFile(obj);

		pFlag.success = true;
		return pFlag;
	}
	else {
		snprintf(buff, 512, "Could not open asset: %s\n", obj_full_path);
		DD_Terminal::post(buff);
		return pFlag;
	}
}

bool ProcessMTLFile(ObjAsset & obj)
{

	std::string mtlfile = obj.info.directory + obj.info.mtl_filename + ".mtl";

	std::ifstream file(mtlfile.c_str());
	if( file.good() ) {
		std::stringstream sstr;
		sstr << file.rdbuf();
		file.close();
		std::string line;

		int index = -1;
		while( std::getline(sstr, line) ) {
			// remove whitespace
			line = TrimWhiteSpace(line);
			std::string lineID = GetLineID(line);

			if( lineID == "newmtl" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				index += 1;

				obj.info.mat_buffer[index].ID = line;
				obj.info.mat_buffer[index].directory = obj.info.directory;
			}
			else if( lineID == "map_Kd" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].albedo_flag = true;
				obj.info.mat_buffer[index].albedo_tex = line.c_str();
			}
			else if( lineID == "map_Kd_m" ) {
				line = line.substr(9);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].multiplier = true;
				obj.info.mat_buffer[index].albedo_flag = true;
				obj.info.mat_buffer[index].albedo_tex = line.c_str();
			}
			else if( lineID == "map_Ks" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].spec_flag = true;
				obj.info.mat_buffer[index].specular_tex = line.c_str();
			}
			else if( lineID == "map_Ka" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].ao_flag = true;
				obj.info.mat_buffer[index].ao_tex = line.c_str();
			}
			else if( lineID == "map_Pr" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].rough_flag = true;
				obj.info.mat_buffer[index].roughness_tex = line.c_str();
			}
			else if( lineID == "map_Pm" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].metal_flag = true;
				obj.info.mat_buffer[index].metalness_tex = line.c_str();
			}
			else if( lineID == "map_Ke" ) {
				line = line.substr(7);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].emit_flag = true;
				obj.info.mat_buffer[index].emissive_tex = line.c_str();
			}
			else if( lineID == "bump" || lineID == "norm" ) {
				line = line.substr(5);
				size_t options = line.find_first_of(whitespace);
				if( options != std::string::npos ) {
					line = line.substr(0, options);
				}
				obj.info.mat_buffer[index].norm_flag = true;
				obj.info.mat_buffer[index].normal_tex = line.c_str();
			}
		}
		// resize material buffer array (because its initialized w/ 50 mats)
		dd_array<obj_mat> temp(index + 1);
		temp = obj.info.mat_buffer;
		obj.info.mat_buffer.resize(index + 1);
		obj.info.mat_buffer = std::move(temp);

		return true;
	}
	else {
		snprintf(buff, 512, "%s not found. Setting material to default.\n",
				 mtlfile.c_str());
		DD_Terminal::post(buff);
		obj.info.mat_buffer.resize(1);
		obj.info.mat_buffer[0].ID = "default";
		for( size_t i = 0; i < obj.meshes.size(); i++ ) {
			obj.meshes[i].material_ID = "default";
		}
		return false;
	}
}

// debugging print
void ObjAssetParser::PrintInfo(ObjAsset& obj)
{
	printf("Mesh directory: %s\n", obj.info.directory.c_str());
	printf("Num meshes: %zd\n", obj.info.num_meshes);
	printf("Num verts: %zd\n", obj.info.num_v);
	printf("Num normals: %zd\n", obj.info.num_vn);
	printf("Num uvs: %zd\n", obj.info.num_vt);
	printf("Num faces: %zd\n", obj.info.num_f);
	printf("MTL filename: %s\n", obj.info.mtl_filename.c_str());

	for( size_t i = 0; i < obj.info.num_meshes; i++ ) {
		printf("Mesh %zd--> ", i);
		switch( obj.info.f_style[i] ) {
			case FaceStyle::ONLY_V:
				printf("Face line info: vertex only.\n");
				break;
			case FaceStyle::V_VN:
				printf("Face line info: vertex and normals.\n");
				break;
			case FaceStyle::V_VT:
				printf("Face line info: vertex and UVs.\n");
				break;
			case FaceStyle::V_VT_VN:
				printf("Face line info: vertex, UVs, and normals.\n");
				break;
			default:
				break;
		}
	}

	for( size_t i = 0; i < obj.info.mesh_indices.size(); i++ ) {
		printf("Mesh %zd: \n\tstart: %zd \n\tend: %zd \n\ttriangles: %zd\n",
			   i,
			   obj.info.mesh_indices[i].start,
			   obj.info.mesh_indices[i].end,
			   obj.info.mesh_indices[i].tris
		);
	}
	for( size_t i = 0; i < obj.info.mat_buffer.size(); i++ ) {
		printf("\nMaterial %zd: \n\tID: %s\n",
			   i, obj.info.mat_buffer[i].ID.c_str());
		if( obj.info.mat_buffer[i].albedo_flag ) {
			printf("\tdiffuse: %s\n", obj.info.mat_buffer[i].albedo_tex.c_str());
		}
		if( obj.info.mat_buffer[i].spec_flag ) {
			printf("\tspec: %s\n", obj.info.mat_buffer[i].specular_tex.c_str());
		}
		if( obj.info.mat_buffer[i].norm_flag ) {
			printf("\tnormal: %s\n", obj.info.mat_buffer[i].normal_tex.c_str());
		}
	}
}

// Convert to usable format for DayDream Engine
bool ObjAssetParser::FormatForOpenGL(ObjAsset & obj)
{
	// set array sizes
	obj.meshes.resize(obj.info.num_meshes);
	obj.indexed_v.resize(obj.info.num_v);
	obj.indexed_vn.resize(obj.info.num_vn);
	obj.indexed_vt.resize(obj.info.num_vt);
	for( size_t i = 0; i < obj.info.num_meshes; i++ ) {
		obj.meshes[i].data.resize(obj.info.mesh_indices[i].tris * 3);
		// 3 indices per triangle
		obj.meshes[i].indices.resize(obj.info.mesh_indices[i].tris * 3);
	}

	if( obj.file_buffer == nullptr ) {
		return false;
	}
	else {
		std::stringstream analysisBuf(obj.file_buffer);
		std::string line;
		size_t face_index = 0, vert_index = 0, norm_index = 0, uv_index = 0,
			element_index = 0;
		int mesh_index = -1;

		FaceVerts triangles;

		while( std::getline(analysisBuf, line) ) {
			// remove whitespace
			line = TrimWhiteSpace(line);
			std::string lineID = GetLineID(line);

			if( lineID == "v" ) {
				obj_vec3 v = GetFloatData(line.substr(2));
				ObjAssetParser::SetBoundingBox(v, obj.meshes[mesh_index + 1]);
				obj.indexed_v[vert_index] = v;
				vert_index += 1;
			}
			else if( lineID == "vn" ) {
				obj.indexed_vn[norm_index] = GetFloatData(line.substr(3));
				norm_index += 1;
			}
			else if( lineID == "vt" ) {
				obj.indexed_vt[uv_index] = GetFloatData(line.substr(3));
				// this may or may not apply (depends on how textures are read)
				obj.indexed_vt[uv_index].y() = 1.0f - obj.indexed_vt[uv_index].y();
				uv_index += 1;
			}
			else if( lineID == "f" ) {
				triangles = SetMultiMeshFaceData(line.substr(2), obj, mesh_index);
				// update the element indices and triangles outside func
				for( size_t i = 0; i < triangles.num_tris; i++ ) {
					obj.meshes[mesh_index].data[element_index] =
						triangles.verts[i * 3];
					obj.meshes[mesh_index].indices[element_index] =
						(int)element_index;
					element_index += 1;
					obj.meshes[mesh_index].data[element_index] =
						triangles.verts[i * 3 + 1];
					obj.meshes[mesh_index].indices[element_index] =
						(int)element_index;
					element_index += 1;
					obj.meshes[mesh_index].data[element_index] =
						triangles.verts[i * 3 + 2];
					obj.meshes[mesh_index].indices[element_index] =
						(int)element_index;
					element_index += 1;
				}

				//if (obj.info.num_meshes > 1) {
				//	// multimesh
				//}
				//else {
				//	// single mesh
				//}

				face_index += 1;
			}
			else if( lineID == "usemtl" ) {
				mesh_index += 1;
				element_index = 0;
				// get mat info
				for( size_t i = 0; i < obj.info.mat_buffer.size(); i++ ) {
					bool test = obj.info.mat_IDs[mesh_index] ==
						obj.info.mat_buffer[i].ID;
					if( test ) {
						obj.meshes[mesh_index].material_info =
							obj.info.mat_buffer[i];
						// get mat ID
						std::string selected_m = obj.info.mat_buffer[i].ID;
						obj.meshes[mesh_index].material_ID = selected_m;
					}
				}
			}
		}
		// calculate tangent space
		for( size_t i = 0; i < obj.meshes.size(); i++ ) {
			if( obj.info.f_style[i] == FaceStyle::V_VT_VN ) {
				CalculateTangentSpace(obj, i);
			}
		}
		// save to .ddmesh format for future use
		//CreateDDMesh(obj);

		// removing excess from memory
		obj.indexed_v.resize(1);
		obj.indexed_vn.resize(1);
		obj.indexed_vt.resize(1);

		return true;
	}
}

void ObjAssetParser::CreateDDMesh(ObjAsset & obj)
{
	std::ofstream outfile;
	std::string meshName = obj.info.directory + obj.info.ddMeshName;
	outfile.open(meshName, std::ios_base::app);

	// number of meshes
	outfile << "NUM_MESH " << obj.meshes.size() << "\n";

	// info per mesh
	for( size_t i = 0; i < obj.meshes.size(); i++ ) {
		MeshData& m = obj.meshes[i];
		outfile << "\nmesh_id " << i << "\n";

		outfile << "\nmat_id " << m.material_ID.c_str() << "\n";
		obj_mat& mat = m.material_info;
		outfile << "mat_dif " <<
			mat.diffuseRaw.x() << " " <<
			mat.diffuseRaw.y() << " " <<
			mat.diffuseRaw.z() << "\n";
		const std::string tex_AB = (mat.albedo_flag) ? mat.albedo_tex : "<>";
		const std::string tex_SP = (mat.spec_flag) ? mat.specular_tex : "<>";
		const std::string tex_AO = (mat.ao_flag) ? mat.ao_tex : "<>";
		const std::string tex_MT = (mat.metal_flag) ? mat.metalness_tex : "<>";
		const std::string tex_RG = (mat.rough_flag) ? mat.roughness_tex : "<>";
		const std::string tex_NM = (mat.norm_flag) ? mat.normal_tex : "<>";
		const std::string tex_EM = (mat.emit_flag) ? mat.emissive_tex : "<>";
		const std::string tex_ML = (mat.multiplier) ? "T" : "F";
		// mat info
		outfile << "mat_ab " << tex_AB.c_str() << "\n";
		outfile << "mat_sp " << tex_SP.c_str() << "\n";
		outfile << "mat_ao " << tex_AO.c_str() << "\n";
		outfile << "amt_mt " << tex_MT.c_str() << "\n";
		outfile << "mat_rh " << tex_RG.c_str() << "\n";
		outfile << "mat_nm " << tex_NM.c_str() << "\n";
		outfile << "mat_em " << tex_EM.c_str() << "\n";
		outfile << "mat_ml " << tex_ML.c_str() << "\n";

		// mesh info
		outfile << "\nnum_ind " << m.indices.size() << "\n";
		outfile << "num_vts " << m.data.size() << "\n";
		for( size_t j = 0; j < m.data.size(); j++ ) {
			Vertex& vert = m.data[j];
			outfile << "pos " << vert.position[0] << " " <<
				vert.position[1] << " " << vert.position[2] << " " << "\n";
			outfile << "norm " << vert.normal[0] << " " <<
				vert.normal[1] << " " << vert.normal[2] << " " << "\n";
			outfile << "tan " << vert.tangent[0] << " " <<
				vert.tangent[1] << " " << vert.tangent[2] << " " << "\n";
			outfile << "uv " << vert.texCoords[0] << " " << vert.texCoords[1]
				<< "\n";
		}
	}
}

bool ObjAssetParser::LoadDDMesh(ObjAsset & obj)
{
	const std::string meshName = obj.info.directory + obj.info.ddMeshName;
	std::ifstream inFile;
	inFile.open(meshName.c_str());
	if( inFile.good() ) {
		std::stringstream analysisBuf;
		analysisBuf << inFile.rdbuf();
		inFile.close();
		std::string line, subline;
		size_t meshIndex = 0;

		while( std::getline(analysisBuf, line) ) {
			// remove whitespace
			line = TrimWhiteSpace(line);
			std::string lineID = GetLineID(line);

			if( lineID == "NUM_MESH" ) {
				int num_mesh = std::atoi((line.substr(9)).c_str());
				obj.meshes.resize(num_mesh);
			}
			else if( lineID == "mesh_id" ) {
				meshIndex = std::atoi((line.substr(8).c_str()));
			}
			else if( lineID == "mat_id" ) {
				obj.meshes[meshIndex].material_ID = line.substr(7);
				obj.meshes[meshIndex].material_info.ID = line.substr(7);
				obj.meshes[meshIndex].material_info.directory =
					obj.info.directory;
			}
			else if( lineID == "mat_dif" ) {
				obj_mat& mat = obj.meshes[meshIndex].material_info;

				std::string tempStr = TrimWhiteSpace(line.substr(8));
				obj_vec3 vec = GetFloatData(tempStr);
				mat.diffuseRaw = vec;
				// grab material info (skip 7 chars to get line info)
				for( size_t i = 0; i < 8; i++ ) {
					switch( i ) {
						case 0:
							// albedo
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.albedo_flag = true;
								mat.albedo_tex = tempStr;
							}
							break;
						case 1:
							// specular
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.spec_flag = true;
								mat.specular_tex = tempStr;
							}
							break;
						case 2:
							// ambient occlusion
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.ao_flag = true;
								mat.ao_tex = tempStr;
							}
							break;
						case 3:
							// metalness
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.metal_flag = true;
								mat.metalness_tex = tempStr;
							}
							break;
						case 4:
							// roughness
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.rough_flag = true;
								mat.roughness_tex = tempStr;
							}
							break;
						case 5:
							// normal
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.norm_flag = true;
								mat.normal_tex = tempStr;
							}
							break;
						case 6:
							// emissive
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							if( tempStr != "<>" ) {
								mat.emit_flag = true;
								mat.emissive_tex = tempStr;
							}
							break;
						case 7:
							// multiplier flag
							std::getline(analysisBuf, line);
							tempStr = line.substr(7);
							mat.multiplier = (tempStr == "T") ? true : false;
							break;
						default:
							break;
					}
				}
			}
			else if( lineID == "num_ind" ) {
				int num_ind = std::atoi((line.substr(8).c_str()));
				obj.meshes[meshIndex].indices.resize(num_ind);
			}
			else if( lineID == "num_vts" ) {
				int num_vts = std::atoi((line.substr(8).c_str()));
				obj.meshes[meshIndex].data.resize(num_vts);
			}
			else if( lineID == "pos" ) {
				obj.meshes[meshIndex].indices[0] = 0;
				dd_array<Vertex>& data = obj.meshes[meshIndex].data;
				// get first set before looping through the rest
				std::string tempStr = TrimWhiteSpace(line.substr(4));
				obj_vec3 tempVec = GetFloatData(tempStr);
				ObjAssetParser::SetBoundingBox(tempVec, obj.meshes[meshIndex]);
				data[0].position[0] = tempVec.x();
				data[0].position[1] = tempVec.y();
				data[0].position[2] = tempVec.z();

				std::getline(analysisBuf, line);
				line = TrimWhiteSpace(line);
				tempVec = GetFloatData((line.substr(5)));
				data[0].normal[0] = tempVec.x();
				data[0].normal[1] = tempVec.y();
				data[0].normal[2] = tempVec.z();

				std::getline(analysisBuf, line);
				line = TrimWhiteSpace(line);
				tempVec = GetFloatData((line.substr(4)));
				data[0].tangent[0] = tempVec.x();
				data[0].tangent[1] = tempVec.y();
				data[0].tangent[2] = tempVec.z();

				std::getline(analysisBuf, line);
				line = TrimWhiteSpace(line);
				tempVec = GetFloatData((line.substr(3)));
				data[0].texCoords[0] = tempVec.x();
				data[0].texCoords[1] = tempVec.y();
				// loop thru remaining verts
				for( size_t i = 1; i < data.size(); i++ ) {
					obj.meshes[meshIndex].indices[i] = (int)i;
					std::getline(analysisBuf, line);
					line = TrimWhiteSpace(line);
					tempVec = GetFloatData((line.substr(4)));
					ObjAssetParser::SetBoundingBox(tempVec, obj.meshes[meshIndex]);
					data[i].position[0] = tempVec.x();
					data[i].position[1] = tempVec.y();
					data[i].position[2] = tempVec.z();

					std::getline(analysisBuf, line);
					line = TrimWhiteSpace(line);
					tempVec = GetFloatData((line.substr(5)));
					data[i].normal[0] = tempVec.x();
					data[i].normal[1] = tempVec.y();
					data[i].normal[2] = tempVec.z();

					std::getline(analysisBuf, line);
					line = TrimWhiteSpace(line);
					tempVec = GetFloatData((line.substr(4)));
					data[i].tangent[0] = tempVec.x();
					data[i].tangent[1] = tempVec.y();
					data[i].tangent[2] = tempVec.z();

					std::getline(analysisBuf, line);
					line = TrimWhiteSpace(line);
					tempVec = GetFloatData((line.substr(3)));
					data[i].texCoords[0] = tempVec.x();
					data[i].texCoords[1] = tempVec.y();
				}
			}
		}
		// removing excess from memory
		obj.indexed_v.resize(1);
		obj.indexed_vn.resize(1);
		obj.indexed_vt.resize(1);

		return true;
	}
	else {
		printf("DDMesh file %s doesn't exist.\n", meshName.c_str());
		return false;
	}
}

void ObjAssetParser::SetBoundingBox(const obj_vec3 & test, MeshData & mesh)
{

	mesh.bbox_max.x() = (mesh.bbox_max.x() > test.x()) ? mesh.bbox_max.x() :
		test.x();
	mesh.bbox_max.y() = (mesh.bbox_max.y() > test.y()) ? mesh.bbox_max.y() :
		test.y();
	mesh.bbox_max.z() = (mesh.bbox_max.z() > test.z()) ? mesh.bbox_max.z() :
		test.z();

	mesh.bbox_min.x() = (mesh.bbox_min.x() < test.x()) ? mesh.bbox_min.x() :
		test.x();
	mesh.bbox_min.y() = (mesh.bbox_min.y() < test.y()) ? mesh.bbox_min.y() :
		test.y();
	mesh.bbox_min.z() = (mesh.bbox_min.z() < test.z()) ? mesh.bbox_min.z() :
		test.z();
}

void ObjAssetParser::CalculateTangentSpace(ObjAsset & obj, const size_t meshIndex)
{
	// calculate tangent and bitangent vectors for triangle faces
	obj_vec3 tan, edge1, edge2, vert1, vert2, vert3;
	obj_vec3 uv1, uv2, uv3, deltaUV1, deltaUV2;
	float factor;

	size_t range = obj.meshes[meshIndex].data.size();
	dd_array<int>& indices = obj.meshes[meshIndex].indices;
	dd_array<Vertex>& vertices = obj.meshes[meshIndex].data;
	dd_array<obj_vec3> faceTanSpace(range / 3);

#pragma omp parallel for
	for( int i = 0; i < (int)range; i += 3 ) {
		vert1 = obj_vec3(vertices[indices[i]].position);
		vert2 = obj_vec3(vertices[indices[i + 1]].position);
		vert3 = obj_vec3(vertices[indices[i + 2]].position);
		uv1 = obj_vec3(
			vertices[indices[i]].texCoords[0],
			vertices[indices[i]].texCoords[1],
			0.0f
		);
		uv2 = obj_vec3(
			vertices[indices[i + 1]].texCoords[0],
			vertices[indices[i + 1]].texCoords[1],
			0.0f
		);
		uv3 = obj_vec3(
			vertices[indices[i + 2]].texCoords[0],
			vertices[indices[i + 2]].texCoords[1],
			0.0f
		);
		edge1 = vert2 - vert1;
		edge2 = vert3 - vert1;
		deltaUV1 = uv2 - uv1;
		deltaUV2 = uv3 - uv1;

		// calculate the inverse of the UV matrix and multiply by edge 1 & 2
		factor = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y());

		tan.x() = factor * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x());
		tan.y() = factor * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y());
		tan.z() = factor * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z());
		tan = tan.normalize();

		faceTanSpace[i / 3] = tan;
	}

	// average face normals across each vertex
	for( size_t i = 0; i < range; ++i ) {
		int face = (int)i / 3;		// face index is a part of (3 per triangle)
		int vert = indices[i];		// specific vertex
		// add face normals to vert
		vertices[vert].tangent[0] += faceTanSpace[face].x();
		vertices[vert].tangent[1] += faceTanSpace[face].y();
		vertices[vert].tangent[2] += faceTanSpace[face].z();
	}
}

// ********************** Utility function definitions ************************

obj_vec3 GetFloatData(const std::string str)
{
	obj_vec3 v_data;
	std::string tempData = str;

	size_t numEnd = tempData.find_first_of(whitespace);
	u8 index = 0;
	while( numEnd != std::string::npos ) {
		v_data.data[index] = float(atof((tempData.substr(0, numEnd)).c_str()));
		index += 1;
		tempData = tempData.substr(numEnd + 1);	// cut number out
		numEnd = tempData.find_first_of(whitespace);
	}
	v_data.data[index] = float(atof(tempData.c_str()));

	return v_data;
}

Vertex CreateVertex(const obj_vec3& pos, const obj_vec3& normal,
					const obj_vec3& texC, const obj_vec3& tang)
{
	Vertex temp;

	for( int i = 0; i < 3; ++i ) {
		temp.position[i] = pos.data[i];
		temp.normal[i] = normal.data[i];
		temp.tangent[i] = tang.data[i];
	}
	temp.texCoords[0] = texC.data[0];
	temp.texCoords[1] = texC.data[1];

	return temp;
}

size_t CountTrianglesFromFaceInfo(const std::string & str)
{
	size_t num_tris = 0, num_verts = 0;

	std::string tempData = str;
	size_t vert_index = tempData.find_first_of(whitespace);
	while( vert_index != std::string::npos ) {
		num_verts += 1;
		tempData = tempData.substr(vert_index + 1);
		vert_index = tempData.find_first_of(whitespace);
	}
	num_verts += 1;
	num_tris = num_verts - 2;  // triangulate all faces using triengle fan

	return num_tris;
}

FaceStyle GetFaceStyleFromFaceInfo(const std::string & str)
{
	std::string tempData = str;
	size_t vert = tempData.find_first_of(whitespace);
	tempData = tempData.substr(0, vert + 1);

	std::string tempStr[] = { "", "", "" };
	int index = 0;
	// split string to gather indices
	size_t strEnd = tempData.find_first_of(slash);
	while( strEnd != std::string::npos ) {
		tempStr[index] = tempData.substr(0, strEnd);
		tempData = tempData.substr(strEnd + 1);
		strEnd = tempData.find_first_of(slash);		// update conditional
		index += 1;									// update index
	}
	tempStr[index] = tempData;

	FaceStyle style = FaceStyle::NOT_SET;
	switch( index ) {
		case 0:
			style = FaceStyle::ONLY_V;
			break;
		case 1:
			style = FaceStyle::V_VT;
			break;
		case 2:
			if( tempStr[1] == "" ) {
				style = FaceStyle::V_VN;
			}
			else {
				style = FaceStyle::V_VT_VN;
			}
			break;
		default:
			break;
	}

	return style;
}

Vertex GetVertexFromFaceInfo(const std::string & str, FaceStyle style,
							 ObjAsset& obj)
{
	Vertex vert;

	std::string tempStr = str;
	std::string tempStr2[] = { "", "", "" };
	int index = 0;
	// split string to gather indices
	size_t strEnd = tempStr.find_first_of(slash);
	while( strEnd != std::string::npos ) {
		tempStr2[index] = tempStr.substr(0, strEnd);
		tempStr = tempStr.substr(strEnd + 1);
		strEnd = tempStr.find_first_of(slash);		// update conditional
		index += 1;                                 // update index
	}
	tempStr2[index] = tempStr;

	switch( style ) {
		case ONLY_V:
		{
			int pos_index = atoi(tempStr2[0].c_str());

			obj_vec3 pos = obj.indexed_v[(size_t)pos_index - 1];
			vert = CreateVertex(pos, empty, empty, empty);
		}
		break;
		case V_VT:
		{
			int pos_index = atoi(tempStr2[0].c_str());
			int uv_index = atoi(tempStr2[1].c_str());

			obj_vec3 pos = obj.indexed_v[(size_t)pos_index - 1];
			obj_vec3 uv = obj.indexed_vt[(size_t)uv_index - 1];
			vert = CreateVertex(pos, empty, uv, empty);
		}
		break;
		case V_VN:
		{
			int pos_index = atoi(tempStr2[0].c_str());
			int norm_index = atoi(tempStr2[2].c_str());

			obj_vec3 pos = obj.indexed_v[(size_t)pos_index - 1];
			obj_vec3 norm = obj.indexed_vn[(size_t)norm_index - 1];
			vert = CreateVertex(pos, norm, empty, empty);
		}
		break;
		case V_VT_VN:
		{
			// vertex, normal, uv info
			int pos_index = atoi(tempStr2[0].c_str());
			int uv_index = atoi(tempStr2[1].c_str());
			int norm_index = atoi(tempStr2[2].c_str());

			obj_vec3 pos = obj.indexed_v[(size_t)pos_index - 1];
			obj_vec3 norm = obj.indexed_vn[(size_t)norm_index - 1];
			obj_vec3 uv = obj.indexed_vt[(size_t)uv_index - 1];
			vert = CreateVertex(pos, norm, uv, empty);
		}
		break;
		default:
			break;
	}

	return vert;
}

FaceVerts SetMultiMeshFaceData(const std::string & data, ObjAsset & obj,
							   const size_t meshIndex)
{
	FaceVerts result;
	std::string tempData = data;
	dd_array<std::string> face_verts(4);

	size_t vert_index = tempData.find_first_of(whitespace);
	size_t num_vert = 0;
	while( vert_index != std::string::npos && num_vert < face_verts.size() ) {
		face_verts[num_vert] = tempData.substr(0, vert_index);
		tempData = tempData.substr(vert_index + 1);
		vert_index = tempData.find_first_of(whitespace);
		num_vert += 1;
	}
	face_verts[num_vert] = tempData;
	num_vert += 1;

	result.num_tris = num_vert - 2;  // triangulate all faces
	for( int i = 0; i < (int)result.num_tris; ++i ) {
		// triangle fan verts on face
		std::string trisStr[] = {
			face_verts[0],
			face_verts[i + 1],
			face_verts[i + 2]
		};

		// for each vertex in this triangle, add to mesh array in order
		for( int j = 0; j < 3; ++j ) {
			// fill in mesh data based on face string and number of meshes
			Vertex vert = GetVertexFromFaceInfo(trisStr[j],
												obj.info.f_style[meshIndex],
												obj
			);
			result.verts[i * 3 + j] = vert;
		}
	}
	return result;
}

ObjAsset::ObjAsset() : file_buffer(nullptr) {}

ObjAsset::~ObjAsset()
{
	if( file_buffer != nullptr ) {
		delete[] file_buffer;
	}
}
