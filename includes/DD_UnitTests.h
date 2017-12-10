#pragma once

// includes for testing
#include "DD_Container.h"
#include "DD_EventQueue.h"
#include "DD_Model.h"
#include "DD_OBJ_Parser.h"
#include "DD_RenderEngine.h"
#include "DD_ResourceLoader.h"
#include "DD_Timer.h"
#include "SimpleAgent.h"
#include "config.h"

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

namespace PsuedoUnitTest {
namespace ArrayTests {

void Print1DArray_int(const dd_array<int>& dd_container,
                      const char* name = "") {
  for (size_t i = 0; i < dd_container.size(); i++) {
    printf("\t1D array[%zd] = %d\n", i, dd_container[i]);
  }
  printf("\tSize of %s array = %zd Bytes \n\n", name,
         dd_container.sizeInBytes());
}

void Print2DArray_int(dd_2Darray<int>& dd_container, const char* name = "") {
  for (size_t i = 0; i < dd_container.numRows(); i++) {
    for (size_t j = 0; j < dd_container.numColumns(); j++) {
      printf("\t2D array[%zd][%zd] = %d\n", i, j, dd_container[i][j]);
    }
  }
  printf("\tSize of %s array = %zd Bytes \n\n", name,
         dd_container.sizeInBytes());
}

void SimpleTest() {
  printf("\n--------- Testing dd_2Darray and dd_array ---------\n\n");
  std::srand(std::time(0));  // use current time for random generation
  dd_array<int> test_one(10);
  dd_2Darray<int> test_two(10, 5);
  u64 start_t, end_t, total_t;
  DD_Timer timer = DD_Timer();

  start_t = Timer::GetHiResTime();
  // Fill in test_one and test_two arrays with random numbers
  for (size_t i = 0; i < test_one.size(); i++) {
    test_one[i] = std::rand();
    for (size_t j = 0; j < test_two.numColumns(); j++) {
      test_two[i][j] = std::rand();
    }
  }
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to fill in test_one and test_two arrays: %" PRIu64 " ns\n",
         end_t - start_t);

  // Print size and check byte size
  Print1DArray_int(test_one, "test_one");
  Print2DArray_int(test_two, "test_two");

  // Create other dd_array's and test assignment operator
  dd_array<int> test_three(50);
  dd_array<int> test_four(5);

  printf("\tSetting test_three array equal to test_one...\n");
  start_t = Timer::GetHiResTime();
  test_three = test_one;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print1DArray_int(test_three, "test_three");

  printf("\tSetting test_four array equal to test_one...\n");
  start_t = Timer::GetHiResTime();
  test_four = test_one;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print1DArray_int(test_four, "test_four");

  // Create other dd_2Darray's and test assignment operator
  dd_2Darray<int> test_five(10, 5);
  dd_2Darray<int> test_six(10, 3);
  dd_2Darray<int> test_seven(8, 8);
  dd_2Darray<int> test_eight(10, 10);

  printf("\tSetting test_five array equal to test_two...\n");
  start_t = Timer::GetHiResTime();
  test_five = test_two;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print2DArray_int(test_five, "test_five");

  printf("\tSetting test_six array equal to test_two...\n");
  start_t = Timer::GetHiResTime();
  test_six = test_two;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print2DArray_int(test_six, "test_six");

  printf("\tSetting test_seven array equal to test_two...\n");
  start_t = Timer::GetHiResTime();
  test_seven = test_two;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print2DArray_int(test_seven, "test_seven");

  printf("\tSetting test_eight array equal to test_two...\n");
  start_t = Timer::GetHiResTime();
  test_eight = test_two;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tTime to transfer array data: %" PRIu64 " ns\n", total_t);
  Print2DArray_int(test_eight, "test_eight");

  printf("\n------------------------------------\n");
}
}

namespace TimerTests {
void SimpleTest() {
  DD_Timer timer = DD_Timer();
  u64 start_t, end_t, total_t;
  printf("\n--------- Testing timers ---------\n\n");
  printf("\tCurrent cpu time in nanoseconds: %" PRIu64 "\n",
         Timer::GetHiResTime());
  printf("\tResolution of time: %zd Bytes\n", sizeof(u64));

  start_t = Timer::GetHiResTime();
  printf("\tTesting precision...\n");
  for (int i = 0; i < 1000000; i++)
    ;
  end_t = Timer::GetHiResTime();
  total_t = end_t - start_t;
  printf("\tStart: %" PRIu64 "ns\n\tEnd: %" PRIu64 "ns\n\tDiff: %" PRIu64
         "ns\n",
         start_t, end_t, total_t);

  printf("\n------------------------------------\n");
}
}

namespace QueueTests {
DD_Event sampleHandler(DD_Event event) {
  printf("Simple event has registered and was called\n");
  DD_Event result = DD_Event();
  return result;
}

void SimpleTest() {
  printf("\n--------- Testing DD_Queue ---------\n\n");
  DD_Queue sampleQ = DD_Queue(5);

  DD_Event temp[5], _event;
  temp[0].m_type = "";
  temp[1].m_type = "";
  temp[2].m_type = "load";
  temp[3].m_type = "";
  temp[4].m_type = "load";

  printf("\tPushing 5 object into the queue...\n");
  for (size_t i = 0; i < 5; i++) {
    sampleQ.push(temp[i]);
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.head(), sampleQ.numEvents());

    printf("\t\tType associated w/ event %zd:\n", i);
    if (temp[i].m_type == "") {
      printf("\t\tType 1: NULL\n\n");
    } else {
      printf("\t\tType 2: Loading Screen\n\n");
    }
  }

  printf("\tPushing more than size of queue.data...\n");
  if (sampleQ.push(temp[5])) {
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.tail(), sampleQ.numEvents());
    printf("\t\tIncorrect behavior. Overflowed full queue\n\n");
  } else {
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.tail(), sampleQ.numEvents());
    printf("\t\tQueue resisted push. Returned false\n\n");
  }

  printf("\tPopping from queue...\n");
  for (size_t i = 0; i < 5; i++) {
    sampleQ.pop(_event);
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.tail(), sampleQ.numEvents());

    printf("\t\tType associated w/ event %zd:\n");
    if (temp[i].m_type == "") {
      printf("\t\tType 1: NULL\n\n");
    } else {
      printf("\t\tType 2: Loading Screen\n\n");
    }
  }

  printf("\tPopping from empty queue...\n");
  if (sampleQ.pop(_event)) {
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.tail(), sampleQ.numEvents());
    printf("\t\tIncorrect behavior. Popped from empty queue\n\n");
  } else {
    printf("\t\tQueue --> head: %zd, tail %zd, size %zd\n", sampleQ.head(),
           sampleQ.tail(), sampleQ.numEvents());
    printf("\t\tQueue resisted pop. Returned empty\n\n");
  }

  printf("\tTesting registration of event handlers.\n");
  printf("\t\tRegistering input ticket...\n");
  EventHandler example = &PsuedoUnitTest::QueueTests::sampleHandler;
  DD_Event inputE = DD_Event();
  inputE.m_type = "input";
  sampleQ.push(inputE);

  sampleQ.RegisterHandler(example, "input");
  sampleQ.ProcessQueue();

  printf("\n------------------------------------\n");
}
}

namespace OBJParserTests {
void SimpleTests() {
  printf("\n--------- Testing DD_ObjParser ---------\n\n");
  std::string objLoc = MESH_DIR + "town/Street_environment_V03.obj";
  ObjAsset cylinders = ObjAsset();

  ObjAssetParser::PreProcess(cylinders, objLoc.c_str());
  ObjAssetParser::PrintInfo(cylinders);
  ObjAssetParser::FormatForOpenGL(cylinders);

  for (size_t i = 0; i < cylinders.meshes.size(); i++) {
    obj_vec3 bb_min = cylinders.meshes[i].bbox_min;
    obj_vec3 bb_max = cylinders.meshes[i].bbox_max;
    printf("Mesh %zd\n", i);
    printf("\n\tBBox min: %.3f, %.3f, %.3f\n", bb_min.x(), bb_min.y(),
           bb_min.z());
    printf("\tBBox max: %.3f, %.3f, %.3f\n", bb_max.x(), bb_max.y(),
           bb_max.z());
  }

  std::string objLoc2 = MESH_DIR + "cylinders.obj";
  DD_Model* testmodel = std::move(ResSpace::loadModel(objLoc2.c_str()));
  for (size_t i = 0; i < testmodel->meshes.size(); i++) {
    ModelSpace::OpenGLBindMesh(i, *testmodel);
  }

  /*for (size_t i = 0; i < cylinders.meshes.size(); i++) {
          printf("\n\nMesh %zd\n", i);
          const size_t limit = cylinders.meshes[i].data.size();
          printf("Num verts: %zd\n", limit);
          for (size_t j = 0; j < limit; j++) {
                  printf("Vert %zd -> %f, %f, %f\n",
                          j,
                          cylinders.meshes[i].data[j].position[0],
                          cylinders.meshes[i].data[j].position[1],
                          cylinders.meshes[i].data[j].position[2]
                  );
          }
  }*/

  printf("\n------------------------------------\n");
}
}

namespace ResourceTests {
void SimpleTests() {
  printf("\n--------- Testing DD_ResourceLoader ---------\n\n");

  DD_Resources res = DD_Resources();
  res = std::move(ResSpace::Load("sample01"));

  for (size_t i = 0; i < res.tex_counter; i++) {
    printf("\nTeture loaded thru sample01.ddres: %s\n", res.textures[i]->m_ID);
    printf("\tDimensions: width %d height %d\n", res.textures[i]->Width,
           res.textures[i]->Height);
  }

  for (size_t i = 0; i < res.mdl_counter; i++) {
    ModelSpace::PrintInfo(*res.models[i]);
  }
  for (size_t i = 0; i < res.light_counter; i++) {
    LightSpace::PrintInfo(*res.lights[i]);
  }
  for (size_t i = 0; i < res.cam_counter; i++) {
    CamSpace::PrintInfo(*res.cameras[i]);
  }
  for (size_t i = 0; i < res.skybox_counter; i++) {
    printf("\nSkybox: %s\n", res.skyboxes[i]->ID.c_str());
    printf("\tright: \t%s\n", res.skyboxes[i]->right.c_str());
    printf("\tleft: \t%s\n", res.skyboxes[i]->left.c_str());
    printf("\ttop: \t%s\n", res.skyboxes[i]->top.c_str());
    printf("\tbottom: %s\n", res.skyboxes[i]->bottom.c_str());
    printf("\tfront: \t%s\n", res.skyboxes[i]->front.c_str());
    printf("\tback: \t%s\n", res.skyboxes[i]->back.c_str());
  }

  printf("\n------------------------------------\n");
}

void AgentTests() {
  printf("\n--------- Testing DD_Agent ---------\n\n");

  DD_Resources res = DD_Resources();
  res = std::move(ResSpace::Load("sample01"));
  DD_Queue sampleQ = DD_Queue(5);
  res.queue = &sampleQ;

  printf("\tAdding agents and registering callbacks.\n\n");
  SimpleAgent myAgent("MyTestAgent", "cylinders4");
  ResSpace::AddAgent(&res, &myAgent);

  printf("\n\tTesting callbacks.\n\n");
  DD_Event rendT = DD_Event();
  rendT.m_type = "trnasform";
  DD_Event rendI = DD_Event();
  rendI.m_type = "input";
  sampleQ.push(rendT);
  sampleQ.push(rendI);
  sampleQ.ProcessQueue();

  printf("\n------------------------------------\n");
}
}

namespace RendererTest {
void SimpleTests(float width, float height, SDL_Window* win) {
  printf("\n--------- Testing DD_Renderer ---------\n\n");

  DD_Resources res = DD_Resources();
  res = std::move(ResSpace::Load((RESOURCE_DIR + "sample01").c_str()));
  DD_Queue sampleQ = DD_Queue(5);
  res.queue = &sampleQ;

  DD_Renderer renderer = DD_Renderer();

  printf("\tTesting shaders.\n");
  renderer.m_resourceBin = &res;
  renderer.LoadRendererEngine(width, height);
  renderer.QueryShaders();

  printf("\tTesting registration of event handler.\n");
  EventHandler handle =
      std::bind(&DD_Renderer::RenderHandler, &renderer, std::placeholders::_1);
  sampleQ.RegisterHandler(handle, "render");
  DD_Event rendR = DD_Event();
  rendR.m_type = "render";

  printf("\tTesting render and culling of agents.\n");
  SimpleAgent myAgent("MyTestAgent", "nanosuit");
  SimpleAgent myAgent2("MyTestAgent01", "town_01");
  SimpleAgent myAgent3("MyTestAgent02", "town_02");
  SimpleAgent myAgent4("MyTestAgent03", "town_03");
  SimpleAgent myAgent5("MyTestAgent04", "town_04");
  SimpleAgent myAgent6("MyTestAgent05", "town_05");
  // instances and colors
  /*
  glm::mat4 secondGuy, thirdGuy, fourthGuy;
  secondGuy = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 4.0f));
  thirdGuy = glm::translate(secondGuy, glm::vec3(4.0f, 0.0f, 0.0f));
  fourthGuy = glm::translate(thirdGuy, glm::vec3(0.0f, 0.0f, 4.0f));
  dd_array<glm::mat4> modelMats(4);
  modelMats[0] = glm::mat4();
  modelMats[1] = secondGuy;
  modelMats[2] = thirdGuy;
  modelMats[3] = fourthGuy;

  dd_array<glm::vec3> colors(4);
  colors[0] = glm::vec3(1.0f);
  colors[1] = glm::vec3(0.0f, 1.0f, 0.0f);
  colors[2] = glm::vec3(1.0f, 0.0f, 0.0f);
  colors[3] = glm::vec3(0.0f, 0.0f, 1.0f);

  myAgent.SetInstances(modelMats);
  myAgent.SetColorInstances(colors);
  //*/
  ResSpace::AddAgent(&res, &myAgent);
  ResSpace::AddAgent(&res, &myAgent2);
  ResSpace::AddAgent(&res, &myAgent3);
  ResSpace::AddAgent(&res, &myAgent4);
  ResSpace::AddAgent(&res, &myAgent5);
  ResSpace::AddAgent(&res, &myAgent6);

  // show skybox
  renderer.CreateCubeMap("sampleSB");

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  while (true) {
    sampleQ.push(rendR);
    sampleQ.ProcessQueue();
    // renderer.QueryShaders();
    // swap buffers
    SDL_GL_SwapWindow(win);
  }

  printf("\n------------------------------------\n");
}
}
}
