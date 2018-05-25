#pragma once

#include <cstdio>
#include <cstdlib>
#include <map>
#include <functional>
#include <fbxsdk.h>
#include "StringLib.h"

#define MAX_JOINTS ((uint8_t)-1)

template<typename T>
struct dd_vec4
{
	T data[4];
	dd_vec4(T x = 0, T y = 0, T z = 0, T w = 0)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}

	dd_vec4(T other_data[4], const unsigned size = 4)
	{
		for (unsigned i = 0; i < size; i++) {
			data[i] = other_data[i];
		}
	}

	void operator=(const dd_vec4 other)
	{
		data[0] = other.data[0];
		data[1] = other.data[1];
		data[2] = other.data[2];
		data[3] = other.data[3];
	}

	dd_vec4 operator-(const dd_vec4 other) const
	{
		return dd_vec4(data[0] - other.data[0],
					   data[1] - other.data[1],
					   data[2] - other.data[2],
					   data[3] - other.data[3]);
	}

	T& x() { return data[0]; }
	T const& x() const { return data[0]; }
	T& y() { return data[1]; }
	T const& y() const { return data[1]; }
	T& z() { return data[2]; }
	T const& z() const { return data[2]; }
	T& w() { return data[3]; }
	T const& w() const { return data[3]; }
};

#define CREATE_VEC(TYPE, SYMBOL, SIZE) \
struct vec##SIZE##_##SYMBOL : public dd_vec4<TYPE> \
{ \
	vec##SIZE##_##SYMBOL(TYPE x = 0, TYPE y = 0, TYPE z = 0, TYPE w = 0) : \
		dd_vec4(x, y, z, w) {} \
 \
 	vec##SIZE##_##SYMBOL(TYPE bin[SIZE]) : dd_vec4(bin, SIZE) \
	{ \
		for(unsigned i = (SIZE - 1); i < 4; i++) { data[i] = 0; } \
	} \
};

CREATE_VEC(float, f, 4)
CREATE_VEC(float, f, 3)
CREATE_VEC(float, f, 2)
CREATE_VEC(uint32_t, u, 4)
CREATE_VEC(uint32_t, u, 3)

// Enum bitwise flags
template<typename Enum>
struct EnableBitMaskOperators
{
	static const bool enable = false;
};

// bitwise Or
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
}

// bitwise Or-Eq
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |=(Enum &lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
	return lhs;
}

// bitwise And
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator &(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
		);
}

/// Triangle information (uses typedef arrays for least amount of padding)
struct VertPNTUV
{
	vec4_u m_joint;
	vec4_f m_jblend;
	vec3_f m_pos;
	vec3_f m_norm;
	vec3_f m_tang;
	vec2_f m_uv;
};

struct TriFBX
{
	vec3_u		m_indices;
	size_t		m_mat_idx;
};

struct CtrlPnt
{
	CtrlPnt()
	{
		for (size_t i = 0; i < 4; i++) {
			m_joint[i] = 0;
			m_blend[i] = 0;
		}
	}
	vec3_f		m_pos;
	unsigned	m_joint[4];
	float		m_blend[4];
	// add joint blend weights to control point
	void addBWPair(const uint16_t idx, const float bw)
	{
		limit += 1;
		if( limit < 4 ) {
			m_joint[limit - 1] = idx;
			m_blend[limit - 1] = bw;
		}
	}
private:
	uint8_t limit = 0;
};

struct MeshFBX
{
	MeshFBX() {}
	MeshFBX(const char* name) { m_id.set(name); }

	cbuff<32>			m_id;
	dd_array<TriFBX>	m_triangles;
	dd_array<CtrlPnt>	m_ctrlpnts;
	dd_array<VertPNTUV> m_verts;

	bool operator==(const MeshFBX &other) const
	{
		return m_id.gethash() == other.m_id.gethash();
	}
};

struct JointFBX
{
	cbuff<32>	m_name;
	uint8_t		m_idx;
	uint8_t		m_parent;
	vec3_f		m_lspos = { 0, 0, 0 };
	vec3_f		m_lsrot = { 0, 0, 0 };
	vec3_f		m_lsscl = { 1, 1, 1 };
};

struct SkelFbx
{
	JointFBX 	m_joints[MAX_JOINTS];
	uint8_t		m_numJoints = 0;
	vec3_f		m_wspos = { 0, 0, 0 };
	vec3_f		m_wsrot = { 0, 0, 0 };
	vec3_f		m_wsscl = { 1, 1, 1 };
};

enum class MatType
{
	NONE = 0x0,
	DIFF = 0x1,
	NORMAL = 0x2,
	SPEC = 0x4,
	ROUGH = 0x8,
	METAL = 0x10,
	EMIT = 0x20,
	AO = 0x40//,
	// ACOL = 0x80,
	// DCOL = 0x100,
	// SCOL = 0x200,
	// ECOL = 0x400,
	// TFAC = 0x800,
	// RFAC = 0x1000,
	// SFAC = 0x2000
};

// enable bitwise operators
template<>
struct EnableBitMaskOperators<MatType> { static const bool enable = true; };

struct MatFBX
{
	MatFBX() {}
	MatFBX(const char* name) { m_id.set(name); }

	cbuff<128>	m_diffmap;
	cbuff<128>	m_normmap;
	cbuff<128>	m_specmap;
	cbuff<128>	m_roughmap;
	cbuff<128>	m_emitmap;
	cbuff<128>	m_aomap;
	cbuff<128>	m_metalmap;
	cbuff<64>	m_id;
	vec3_f		m_ambient;
	vec3_f		m_diffuse;
	vec3_f		m_specular;
	vec3_f		m_emmisive;
	float		m_transfactor;
	float		m_reflectfactor;
	float		m_specfactor;
	MatType	m_textypes = MatType::NONE;

	bool operator==(const MatFBX &other) const
	{
		return m_id.gethash() == other.m_id.gethash();
	}
};

struct AnimSample
{
	vec3_f		rot;
	vec3_f		pos;
	//float		scal;
};

struct PoseSample
{
	dd_array<AnimSample> pose;
	dd_array<vec3_u> logged_r;
	dd_array<vec3_u> logged_t;
};

struct AnimClipFBX
{
	AnimClipFBX() {}
	AnimClipFBX(const char* name) { m_id.set(name); } 

	cbuff<64> 	m_id;
	float		m_framerate;
	uint8_t		m_joints;
	std::map<unsigned, PoseSample> m_clip;
};

struct AssetFBX
{
	struct EboMesh { dd_array<vec3_u> indices; };
	AssetFBX() : 
		m_matbin(0), 
		m_verts(0), 
		m_ebos(0), 
		m_clips(0), 
		m_viconFormat(false) 
	{}

	cbuff<32>			m_id;
	cbuff<64>			m_fbxName;
	cbuff<512>			m_fbxPath;
	dd_array<MatFBX> 	m_matbin;
	dd_array<VertPNTUV> m_verts;
	dd_array<EboMesh> 	m_ebos;
	SkelFbx				m_skeleton;
	dd_array<AnimClipFBX> m_clips;
	bool				m_viconFormat;
	float				scale_factor;

	void addMesh(MeshFBX& _mesh, dd_array<size_t> &ebo_data);
	void exportMesh();
	void exportSkeleton();
	void exportAnimation();
};
