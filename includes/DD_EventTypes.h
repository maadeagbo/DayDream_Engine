#pragma once

#include <string>
#include <functional>
#define GLM_FORCE_CXX98
#define GLM_FORCE_CXX11 
#define GLM_FORCE_CXX14 // removes non-standard extensions warnings in VS compiler
#include <glm/glm.hpp>

// abstract class for events
struct BaseData
{
	virtual ~BaseData() {};
};

struct trans4m : public BaseData
{
	float pos[3], scale[3], quat[4];
};

struct flagBuff : public BaseData
{
	bool flag = false;
};

struct inputBuff : public BaseData
{
	bool rawInput[512];
	bool mouseLMR[3];
	int mouseXDelta, mouseYDelta, mouseX, mouseY, mouseScroll;
};

struct messageBuff : public BaseData
{
	char message512[512];
};

// Agent to agent data types
struct agentMsgStr : public BaseData
{
	char data[64], reciever[64], sender[64];
};

struct agentMsgInt : public BaseData
{
	char reciever[64], sender[64];
	int data;
};

struct agentMsgFloat : public BaseData
{
	char reciever[64], sender[64];
	float data;
};

struct agentMsgTrans4m : public BaseData
{
	char reciever[64], sender[64];
	trans4m data;
};

// shader structs
struct shaderDataA : public BaseData
{
	std::string sender, texture_ID, texture2_ID, shader_ID;
	size_t data_info[3] = { 1, 1, 1 };
	glm::vec4 *data_01 = nullptr, *data_02 = nullptr;
	glm::vec2 uniform_01;
	float byte_size01 = 0, byte_size02 = 0;
};

// Event struct and signature

struct DD_Event
{
	DD_Event()
	{
		m_type = "";
		m_time = 0.0f;
		m_total_runtime = 0.0f;
		m_message = nullptr;
	}
	std::string m_type;
	float m_time;
	float m_total_runtime;
	BaseData* m_message;
};

// event handler signature
typedef std::function<DD_Event(DD_Event&)> EventHandler;