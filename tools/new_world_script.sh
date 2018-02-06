#!/usr/bin/env bash

# create file
touch "$1"

# Title comment
echo -e "-- World file for $2 project\n" > $1

# script start
echo -e "do" >> $1

# include useful libraries and scripts
echo -e "\tmatrix = require \"scripts.matrix\"" >> $1
echo -e "\tactor_prototype = require \"$2.$2_actor\"" >> $1

# world object
echo -e "\n\t$2 = {}\n" >> $1

# Initialization function
echo -e "\tfunction $2:init( event, args, num_args )" >> $1
echo -e "\t\t-- Make assets object local" >> $1
echo -e "\t\tassets = $2_assets\n" >> $1
echo -e "\t\t-- Create actor controller and set variables" >> $1
echo -e "\t\t$2.actor = actor_prototype:new( {name = \"actor_01\"} )" >> $1
echo -e "\t\t$2.actor.agent_id = assets.$2_agent" >> $1
echo -e "\t\t$2.actor.cam_id = assets.cam_01" >> $1
echo -e "\t\t-- Register actor callback" >> $1
echo -e "\t\tdd_register_callback($2.actor.name, $2.actor)" >> $1
echo -e "\t\t-- Subscribe to $2_world generated callback" >> $1
echo -e "\t\tdd_subscribe( {key = $2.actor.name, event = \"$2_update\"} )" >> $1
echo -e "\n\t\tdd_print( \"$2 init called.\" )" >> $1
echo -e "\tend\n" >> $1

# Update function
echo -e "\tfunction $2:update( event, args, num_args )" >> $1
echo -e "\t\tif event == \"update\" then" >> $1
echo -e "\t\t\t-- Push level-specific update event" >> $1
echo -e "\t\t\tdd_push( {event_id = \"$2_update\"} )\n\t\tend" >> $1
echo -e "\tend\n" >> $1

# script end
echo -e "end" >> $1
