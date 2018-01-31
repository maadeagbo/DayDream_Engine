#!/usr/bin/env bash

# create file
touch "$1"

# Title comment
echo -e "-- World file for $2 project\n" > $1

# script start
echo -e "do" >> $1

# include useful libraries
echo -e "\tmatrix = require \"scripts.matrix\"" >> $1

# world object
echo -e "\n\t$2 = {}\n" >> $1

# Initialization function
echo -e "\tfunction $2:init( event, args, num_args )" >> $1
echo -e "\t\tdd_print( \"$2 init called.\" )" >> $1
echo -e "\tend\n" >> $1

# Update function
echo -e "\tfunction $2:update( event, args, num_args )" >> $1
echo -e "\t\tdd_print( \"$2 update called.\" )" >> $1
echo -e "\tend\n" >> $1

# script end
echo -e "end" >> $1
