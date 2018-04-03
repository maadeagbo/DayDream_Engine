#!/usr/bin/env bash

SDIR=$1

# render shader
./../../bin/dd_shader_reflect -o bbox_shader_enums.h -e RE_Line \
	-v "$SDIR/LineRend_V.vert" \
	-f "$SDIR/LineRend_F.frag"
