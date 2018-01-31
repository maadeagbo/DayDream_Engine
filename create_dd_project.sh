#!/usr/bin/env bash

if [[ $# -eq 0 ]] ; then
    echo -e "\nProvide project name\n"
    exit 1
fi

# colors for echo
GREEN='\033[0;32m'
NC='\033[0m' # No Color

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WRITE_DIR="$ROOT_DIR/Projects/$1_lvl"
PROJ_NAME=$1

# create directory if it doesn't exist
echo -e "Creating new project called ${GREEN}'$1'${NC}..."
mkdir -p $WRITE_DIR

# create asset file
echo -e "  Creating assest file: ${GREEN}$1_assets.lua${NC}"
source "$ROOT_DIR/tools/new_assets_script.sh" $WRITE_DIR/"$1_assets.lua" $1

# create world file
echo -e "  Creating world file: ${GREEN}$1_world.lua${NC}"
source "$ROOT_DIR/tools/new_world_script.sh" $WRITE_DIR/"$1_world.lua" $1
