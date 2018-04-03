#!/usr/bin/env bash

# create file
touch "$1"

# Title comment
echo -e "-- Actor script for $2 project\n" > $1

# script start
echo -e "do" >> $1

# matrix
echo -e "\tmatrix = require \"scripts.matrix\"\n" >> $1

# Actor object
echo -e "\t$2_actor = {}\n" >> $1

# pitch and yaw variables
echo -e "\tspeed = 5.0\n\tpitch = 0\n\tyaw = 0\n" >> $1

# create init function
echo -e "\tfunction $2_actor:new( params )" >> $1

echo -e "\t\tparams = params or {}" >> $1
echo -e "\t\tself.__index = self" >> $1
echo -e "\t\tsetmetatable(params, self)\n" >> $1
echo -e "\t\treturn params" >> $1

# init function end
echo -e "\tend\n" >> $1

# update function start
echo -e "\tfunction $2_actor:update( event, args, num_args )" >> $1

# localize assets object
echo -e "\t\tassets = $2_assets\n" >> $1

# get vectors necessary for movement
echo -e "\t\t-- Get current position" >> $1
echo -e "\t\tcurr_pos = assets.nav_agent:pos()\n" >> $1
echo -e "\t\t-- Get current facing direction and convert to vector" >> $1
echo -e "\t\tcurr_d = assets.cam_01:dir()" >> $1
echo -e "\t\tv3_fdir = matrix{ curr_d.x, curr_d.y, curr_d.z }\n" >> $1
echo -e "\t\t-- Direction facing to the right" >> $1
echo -e "\t\tv3_udir = matrix{0, 1, 0}" >> $1
echo -e "\t\tv3_rdir = matrix.cross( v3_fdir, v3_udir )\n" >> $1

# read input keys and calculate movement
echo -e "\t\t--ddLib.print( string.format(\"Pos = %.3f, %.3f, %.3f\"," >> $1
echo -e "\t\t\t--curr_pos.x, curr_pos.y, curr_pos.z))\n" >> $1
echo -e "\t\t-- Get frame time and setup new position variable" >> $1 
echo -e "\t\tnew_pos = matrix{curr_pos.x, curr_pos.y, curr_pos.z}" >> $1
echo -e "\t\tftime = ddLib.ftime()\n" >> $1 

# position
echo -e "\t\t-- left" >> $1 
echo -e "\t\tif ddInput.a then new_pos = new_pos - (v3_rdir * ftime * speed) end" >> $1
echo -e "\t\t-- right" >> $1 
echo -e "\t\tif ddInput.d then new_pos = new_pos + (v3_rdir * ftime * speed) end" >> $1 
echo -e "\t\t-- forward" >> $1 
echo -e "\t\tif ddInput.w then new_pos = new_pos + (v3_fdir * ftime * speed) end" >> $1 
echo -e "\t\t-- back" >> $1 
echo -e "\t\tif ddInput.s then new_pos = new_pos - (v3_fdir * ftime * speed) end" >> $1 
echo -e "\t\t-- down" >> $1 
echo -e "\t\tif ddInput.l_shift then new_pos = new_pos - (v3_udir * ftime * speed) end" >> $1
echo -e "\t\t-- up" >> $1 
echo -e "\t\tif ddInput.space then new_pos = new_pos + (v3_udir * ftime * speed) end" >> $1

# rotation
echo -e "\n\t\t-- rotation" >> $1
echo -e "\t\tif ddInput.mouse_b_l then" >> $1
echo -e "\t\t\tpitch = pitch + ddInput.mouse_y_delta * 1.0/speed" >> $1
echo -e "\t\t\tyaw = yaw + ddInput.mouse_x_delta * 1.0/speed\n" >> $1
echo -e "\t\t\tassets.cam_01:set_eulerPYR(pitch, 0, 0)" >> $1
echo -e "\t\t\tassets.nav_agent:set_eulerPYR(0, yaw, 0)" >> $1
echo -e "\t\tend" >> $1

echo -e "\t\tassets.nav_agent:set_pos(new_pos[1][1], new_pos[2][1], new_pos[3][1])" >> $1

# update function end
echo -e "\tend" >> $1

# return prototype object
echo -e "\n\treturn $2_actor" >> $1

# script end
echo -e "end" >> $1
