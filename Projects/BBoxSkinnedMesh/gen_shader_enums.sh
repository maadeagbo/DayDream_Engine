#!/usr/bin/env bash

# render shader
./../../bin/shader_reflect -o bbox_shader_enums.h -e RE_Line \
	-v "./LineRend_V.vert" \
	-f "./LineRend_F.frag"
