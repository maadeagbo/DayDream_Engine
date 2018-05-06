#include "FBX_Utility.h"
#include <map>
#include <fstream>

size_t numTabs = 0;

/// \brief Split mesh into subsequent EBO buffers based on shared materials
/// \param _mesh MeshFBX struct that contains all mesh buffer data
/// \param ebo_data Lists buffer sizes for each material (sorted by index)
void AssetFBX::addMesh(MeshFBX & _mesh, dd_array<size_t> &ebo_data)
{
	m_id.set(_mesh.m_id.str());
	m_verts = std::move(_mesh.m_verts);

	printf("Materials\n");
	// resize ebo and material buffer (materials w/ no triangles are skipped)
	dd_array<uint32_t> valid_mats(ebo_data.size());
	std::map<uint32_t, uint32_t> mat_idx;
	uint8_t idx = 0;
	for( size_t i = 0; i < ebo_data.size(); i++ ) {
		printf("ebo #%lu: %lu\n", i, ebo_data[i]);
		if( ebo_data[i] > 0 ) {
			printf("\t%s \tregistered at: %u\n", m_matbin[i].m_id.str(), idx);
			valid_mats[idx] = i;
			mat_idx[(uint8_t)i] = idx;
			idx += 1;
		}
	}
	// set valid materials
	if( idx < m_matbin.size() ) {
		dd_array<MatFBX> temp(idx);
		for( size_t i = 0; i < idx; i++ ) {
			temp[i] = std::move(m_matbin[valid_mats[i]]);
		}
		m_matbin = std::move(temp);
	}
	// size ebo bins for meshes
	m_ebos.resize(m_matbin.size());
	for(auto& idx_pair : mat_idx) {
		m_ebos[idx_pair.second].indices.resize(ebo_data[idx_pair.first]);
	}
	// separate triangles into correct bins
	dd_array<uint32_t> idx_tracker(m_matbin.size());
	for( size_t i = 0; i < _mesh.m_triangles.size(); i++ ) {
		size_t key = _mesh.m_triangles[i].m_mat_idx;
		size_t value = mat_idx[key];
		m_ebos[value].indices[idx_tracker[value]] =
			_mesh.m_triangles[i].m_indices;
		idx_tracker[value] += 1;
	}
}

/// \brief Export skeleton to format specified by dd_entity_map.txt
void AssetFBX::exportSkeleton()
{
	cbuff<512> buff512;
	buff512.format("%s%s.ddb", m_fbxPath.str(), m_fbxName.str());
	std::fstream outfile;
	outfile.open(buff512.str(), std::ofstream::out);

	// check file is open
	if (outfile.bad()) {
		printf("Could not open skeleton output file\n" );
		return;
	}

	// size
	buff512.format("%u\n", m_skeleton.m_numJoints);
	outfile << "<size>\n" << buff512.str() << "</size>\n";

	// joint to world space
	buff512.format("p %.3f %.3f %.3f\n",
				   m_skeleton.m_wspos.x(),
				   m_skeleton.m_wspos.y(),
				   m_skeleton.m_wspos.z());
	outfile << "<global>\n" << buff512.str();

	// change rotation if vicon fix is active
	if (m_viconFormat) {
		buff512.format("r %.3f %.3f %.3f\n",
					   m_skeleton.m_wsrot.x() - 90.f,
					   m_skeleton.m_wsrot.y() + 180.f,
					   m_skeleton.m_wsrot.z());
		outfile << buff512.str();
	}
	else {
		buff512.format("r %.3f %.3f %.3f\n",
					   m_skeleton.m_wsrot.x(),
					   m_skeleton.m_wsrot.y(),
					   m_skeleton.m_wsrot.z());
		outfile << buff512.str();
	}
	buff512.format("s %.3f %.3f %.3f\n",
				   m_skeleton.m_wsscl.x(),
				   m_skeleton.m_wsscl.y(),
				   m_skeleton.m_wsscl.z());
	outfile << buff512.str() << "</global>\n";

	// joints
	for (size_t i = 0; i < m_skeleton.m_numJoints; i++) {
		JointFBX& _j = m_skeleton.m_joints[i];
		buff512.format("%s %u %u\n", _j.m_name.str(), _j.m_idx, _j.m_parent);
		outfile << "<joint>\n" << buff512.str();
		buff512.format("p %.3f %.3f %.3f\n", 
					   _j.m_lspos.x() * scale_factor,
					   _j.m_lspos.y() * scale_factor,
					   _j.m_lspos.z() * scale_factor);
		outfile << buff512.str();
		buff512.format("r %.3f %.3f %.3f\n",
					   _j.m_lsrot.x(),
					   _j.m_lsrot.y(),
					   _j.m_lsrot.z());
		outfile << buff512.str();
		buff512.format("s %.3f %.3f %.3f\n",
					   _j.m_lsscl.x(),
					   _j.m_lsscl.y(),
					   _j.m_lsscl.z());
		outfile << buff512.str() << "</joint>\n";
	}
}

/// \brief Export mesh to format specified by dd_entity_map.txt
void AssetFBX::exportMesh()
{
	cbuff<512> buff512;
	// remove/replace restricted filename symbols
	std::string id = m_id.str();
	std::replace(id.begin(), id.end(), ':', '_');

	buff512.format("%s%s.ddm", m_fbxPath.str(), id.c_str());
	std::fstream outfile;
	outfile.open(buff512.str(), std::ios::out);

	// check file is open
	if (outfile.bad()) {
		printf("Could not open mesh output file\n" );
		return;
	}

	// name
	buff512.format("%s\n", m_id.str());
	outfile << "<name>\n" << buff512.str() << "</name>\n";

	// buffer sizes
	buff512.format("v %lu\n", m_verts.size());
	outfile << "<buffer>\n" << buff512.str();
	buff512.format("e %lu\n", m_ebos.size());
	outfile << buff512.str();
	buff512.format("m %lu\n", m_matbin.size());
	outfile << buff512.str() << "</buffer>\n";

	// material data
	for (size_t i = 0; i < m_matbin.size(); i++) {
		MatFBX& _m = m_matbin[i];
		buff512.format("n %s\n", _m.m_id.str());
		outfile << "<material>\n" << buff512.str();
		if (bool(_m.m_textypes & MatType::DIFF)) {
			buff512.format("D %s\n", _m.m_diffmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::NORMAL)) {
			buff512.format("N %s\n", _m.m_normmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::SPEC)) {
			buff512.format("S %s\n", _m.m_specmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::ROUGH)) {
			buff512.format("R %s\n", _m.m_roughmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::METAL)) {
			buff512.format("M %s\n", _m.m_metalmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::EMIT)) {
			buff512.format("E %s\n", _m.m_emitmap.str());
			outfile << buff512.str();
		}
		if (bool(_m.m_textypes & MatType::AO)) {
			buff512.format("A %s\n", _m.m_aomap.str());
			outfile << buff512.str();
		}
		// vector properties
		buff512.format("a %.3f %.3f %.3f\n", 
					   _m.m_ambient.x(), _m.m_ambient.y(), _m.m_ambient.z());
		outfile << buff512.str();
		buff512.format("d %.3f %.3f %.3f\n",
					   _m.m_diffuse.x(), _m.m_diffuse.y(), _m.m_diffuse.z());
		outfile << buff512.str();
		buff512.format("s %.3f %.3f %.3f\n",
					   _m.m_specular.x(), _m.m_specular.y(), _m.m_specular.z());
		outfile << buff512.str();
		buff512.format("e %.3f %.3f %.3f\n",
					   _m.m_emmisive.x(), _m.m_emmisive.y(), _m.m_emmisive.z());
		outfile << buff512.str();

		// float properties
		buff512.format("x %.3f\n", _m.m_transfactor);
		outfile << buff512.str();
		buff512.format("y %.3f\n", _m.m_reflectfactor);
		outfile << buff512.str();
		buff512.format("z %.3f\n", _m.m_specfactor);
		outfile << buff512.str() << "</material>\n";
	}

	// vertex data
	outfile << "<vertex>\n";
	for (size_t i = 0; i < m_verts.size(); i++) {
		buff512.format("v %.3f %.3f %.3f\n",
					   m_verts[i].m_pos.x() * scale_factor,
					   m_verts[i].m_pos.y() * scale_factor,
					   m_verts[i].m_pos.z() * scale_factor);
		outfile << buff512.str();
		buff512.format("n %.3f %.3f %.3f\n",
					   m_verts[i].m_norm.x(),
					   m_verts[i].m_norm.y(),
					   m_verts[i].m_norm.z());
		outfile << buff512.str();
		buff512.format("t %.3f %.3f %.3f\n",
					   m_verts[i].m_tang.x(),
					   m_verts[i].m_tang.y(),
					   m_verts[i].m_tang.z());
		outfile << buff512.str();
		buff512.format("u %.3f %.3f\n",
					   m_verts[i].m_uv.x(),
					   m_verts[i].m_uv.y());
		outfile << buff512.str();
		buff512.format("j %u %u %u %u\n",
					   m_verts[i].m_joint.x(),
					   m_verts[i].m_joint.y(),
					   m_verts[i].m_joint.z(),
					   m_verts[i].m_joint.w());
		outfile << buff512.str();
		buff512.format("b %.3f %.3f %.3f %.3f\n",
					   m_verts[i].m_jblend.x(),
					   m_verts[i].m_jblend.y(),
					   m_verts[i].m_jblend.z(),
					   m_verts[i].m_jblend.w());
		outfile << buff512.str();
	}
	outfile << "</vertex>\n";

	// ebo data
	for (size_t i = 0; i < m_ebos.size(); i++) {
		EboMesh& _e = m_ebos[i];
		buff512.format("s %lu\n", _e.indices.size() * 3); // ebo size
		outfile << "<ebo>\n" << buff512.str();
		buff512.format("m %lu\n", i); // material index
		outfile << buff512.str();

		for (size_t j = 0; j < _e.indices.size(); j++) {
			buff512.format("- %u %u %u\n",
						   _e.indices[j].x(),
						   _e.indices[j].y(),
						   _e.indices[j].z());
			outfile << buff512.str();
		}
		outfile << "</ebo>\n";
	}

	outfile.flush();
	//outfile.close();
}

/// \brief Export animation to format specified by dd_entity_map.txt
void AssetFBX::exportAnimation()
{
	for(unsigned i = 0; i < m_clips.size(); i++) {
		cbuff<512> buff512;
		buff512.format("%s%s_%u.dda",  m_fbxPath.str(), m_fbxName.str(), i);
		std::fstream outfile;
		outfile.open(buff512.str(), std::ofstream::out);
	
		// check file is open
		if (outfile.bad()) {
			printf("Could not open animation output file\n" );
			return;
		}

		// framerate
		buff512.format("%.3f\n", m_clips[i].m_framerate);
		outfile << "<framerate>\n" << buff512.str() << "</framerate>\n";

		// repeat
		outfile << "<repeat>\n" << 0 << "\n</repeat>\n";

		// buffer sizes
		buff512.format("j %u\n", m_clips[i].m_joints);
		outfile << "<buffer>\n" << buff512.str();
		buff512.format("f %lu\n", m_clips[i].m_clip.size());
		outfile << buff512.str() << "</buffer>\n";

		// animation data
		for(unsigned j = 0; j < m_clips[i].m_joints; j++) {
			buff512.format("- %u\n", j);
			outfile << "<animation>\n" << buff512.str();
			for(auto& p : m_clips[i].m_clip) {
				buff512.format("r %.3f %.3f %.3f\n",
							   p.second.pose[j].rot.x(),
							   p.second.pose[j].rot.y(),
							   p.second.pose[j].rot.z());
				outfile << buff512.str();
				buff512.format("p %.3f %.3f %.3f\n",
							   p.second.pose[j].pos.x() * scale_factor,
							   p.second.pose[j].pos.y() * scale_factor,
							   p.second.pose[j].pos.z() * scale_factor);
				outfile << buff512.str();
			}
			outfile << "</animation>\n";
		}
		outfile.close();
	}
}