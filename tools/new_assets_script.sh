#!/usr/bin/env bash

# create file
touch "$1"

# Title comment
echo -e "-- Asset file for $2 project\n" > $1

# Table for storing loaded assets
echo -e "$2_assets = {}\n" >> $1

# Create load function
echo -e "function load()" >> $1

echo -e "\targ = {}\n" >> $1

# load box primitive for floor
echo -e "\t-- Cube mesh" >> $1
echo -e "\t$2_assets.cube_m = ddModel.new(ROOT_DIR..\"/Resource/Meshes/primitives/cube.ddm\")" >> $1
echo -e "\tddLib.print( \"Created plane mesh: \", $2_assets.cube_m:id() )\n" >> $1

# create simple directional light
echo -e "\t-- Directional light" >> $1
echo -e "\t$2_assets.light_1 = ddLight.new(\"light_1\")" >> $1
echo -e "\t$2_assets.light_1:set_active(true)" >> $1
echo -e "\tddLib.print( \"Created light: \", $2_assets.light_1:id() )\n" >> $1

# create floor object
echo -e "\t-- Floor agent" >> $1
echo -e "\t$2_assets.floor = ddAgent.new(\"floor_agent\", 0.0, \"box\")" >> $1
echo -e "\t$2_assets.floor:set_scale(100.0, 0.2, 100.0)" >> $1
echo -e "\t$2_assets.floor:add_mesh($2_assets.cube_m:id(), 0.1, 100.0)" >> $1
echo -e "\tddLib.print( \"Created agent (floor): \", $2_assets.floor:id() )\n" >> $1

# create random object
echo -e "\t-- Box agent" >> $1
echo -e "\t$2_assets.rand_obj = ddAgent.new(\"rand_obj\", 1.0, \"box\")" >> $1
echo -e "\t$2_assets.rand_obj:set_pos(0.0, 5.0, 0.0)" >> $1
echo -e "\t$2_assets.rand_obj:add_mesh($2_assets.cube_m:id(), 0.1, 100.0)" >> $1
echo -e "\tddLib.print( \"Created agent (rand_obj): \", $2_assets.rand_obj:id() )\n" >> $1

# create parent agent for camera manipulation
echo -e "\t-- Empty camera agent" >> $1
echo -e "\t$2_assets.nav_agent = ddAgent.new(\"$2_agent\", 1.0, \"kinematic\")" >> $1
echo -e "\t$2_assets.nav_agent:set_pos(0.0, 2.0, 5.0)" >> $1
echo -e "\t$2_assets.nav_agent:set_eulerPYR(0.0, 0.0)" >> $1
echo -e "\tddLib.print( \"Created agent ($2_agent): \", $2_assets.nav_agent:id() )\n" >> $1

# attach camera
echo -e "\t-- Attach camera" >> $1
echo -e "\targs = {}" >> $1
echo -e "\targs[\"id\"] = \"cam_01\"" >> $1
echo -e "\targs[\"parent\"] = cam_agent_id" >> $1

echo -e "\t$2_assets.cam_01 = ddCam.new(\"cam_01\", $2_assets.nav_agent:id())" >> $1
echo -e "\t$2_assets.cam_01:set_active(true)" >> $1
echo -e "\tddLib.print( \"Created camera: \", $2_assets.cam_01:id() )\n" >> $1

# end load function
echo -e "end" >> $1

