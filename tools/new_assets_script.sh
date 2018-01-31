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
echo -e "\targ[\"path\"] = ROOT_DIR..\"/Resource/Meshes/primitives/cube.ddm\""\
  >> $1
echo -e "\tcube_mesh = dd_load_ddm(arg)" >> $1
echo -e "\tdd_print( \"Created plane mesh: \"..cube_mesh )\n" >> $1

# create simple directional light
echo -e "\t-- Directional light" >> $1
echo -e "\targ = { [\"id\"] = \"light_1\" }" >> $1
echo -e "\tlight_key = dd_create_light(arg)" >> $1
echo -e "\tdd_print( \"Created light: \"..light_key )\n" >> $1

# create floor object
echo -e "\t-- Floor agent" >> $1
echo -e "\targs = {}" >> $1
echo -e "\targs[\"id\"] = \"floor_agent\"" >> $1
echo -e "\targs[\"mesh\"] = cube_mesh" >> $1
echo -e "\targs[\"scale_x\"] = 100.0" >> $1
echo -e "\targs[\"scale_y\"] = 0.2" >> $1
echo -e "\targs[\"scale_z\"] = 100.0" >> $1
echo -e "\tfloor_agent = dd_create_agent(args)" >> $1
echo -e "\t-- save to asset table" >> $1
echo -e "\t$2_assets[\"floor\"] = floor_agent" >> $1
echo -e "\tdd_print( \"Created agent (floor): \"..floor_agent )\n" >> $1

# create parent agent for camera manipulation
echo -e "\t-- Empty camera agent" >> $1
echo -e "\targs = {}" >> $1
echo -e "\targs[\"id\"] = \"$2_agent\"" >> $1
echo -e "\targs[\"mass\"] = 1.0" >> $1
echo -e "\targs[\"type\"] = -1" >> $1
echo -e "\targs[\"pos_y\"] = 5.0" >> $1
echo -e "\targs[\"pos_z\"] = 5.0" >> $1
echo -e "\tcam_agent_id = dd_create_agent(args)" >> $1
echo -e "\t$2_assets[\"$2_agent\"] = cam_agent_id\n" >> $1

# attach camera
echo -e "\t-- Attach camera" >> $1
echo -e "\targs = {}" >> $1
echo -e "\targs[\"id\"] = \"cam_01\"" >> $1
echo -e "\targs[\"parent\"] = cam_agent_id" >> $1
echo -e "\tcam_key = dd_create_camera(args)" >> $1
echo -e "\t$2_assets[\"cam_01\"] = cam_key" >> $1
echo -e "\tdd_print( \"Created camera: \"..cam_key )\n" >> $1

# end load function
echo -e "end" >> $1

