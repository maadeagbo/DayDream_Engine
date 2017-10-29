#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_GameLevel:
*		- manages loading of assets to engine
*		- support m_handler function for level scripts
*
*	TODO:	==
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"
#include "DD_Terminal.h"

#include "DD_Agent.h"
#include "DD_Camera.h"
#include "DD_Light.h"
#include "DD_ResourceLoader.h"
#include "DD_AITypes.h"

class DD_GameLevel
{
public:
	DD_GameLevel() :
		tickets(5),
		handlers(5),
		num_handlers(0),
		m_scrHorzDist(0.0f),
		m_scrVertDist(0.0f),
		m_cubeMapID(""),
		m_lvlID(""),
		m_assetFile(""),
		m_flagHandler(false),
		m_flagDynamicCubeMap(false)
	{}
	~DD_GameLevel() {}

	// Load assests and setting for game level
	virtual void Init() {};

	void AddAgent(DD_Agent* agent);
	void AddCallback(const char* ticket, EventHandler handler);

	DD_Resources* res_ptr;
	dd_array<std::string> tickets;
	dd_array<EventHandler> handlers;

	DD_Material* GetMaterial(const char* model_ID, const size_t mesh_index = 0,
							 const size_t LOD_lvl = 0);

	/*DD_AIObject* GetGenericAI(const char* ID = "");
	DD_Material* GetGenericMaterial(const char* ID = "");
	DD_LineAgent* GetGenericLineAgent(const char* ID = "");
	DD_Camera* GetGenericCamera(const char* ID = "");
	DD_Light* GetGenericLight(const char* ID = "");
	DD_Agent* GetGenericAgent(const char* ID = "", const char* model = "");*/

	// template<class T>
	// T GetNewObject(const char* ID)
	// {
	// 	return static_cast<T>(ResSpace::GetAsset<T>(res_ptr, false, ID));
	// }

	size_t m_num_callbacks, num_handlers, m_screenW, m_screenH;
	float m_scrHorzDist, m_scrVertDist;
	std::string m_cubeMapID, m_lvlID, m_assetFile;
	bool m_flagHandler, m_flagDynamicCubeMap;
};
