#!/usr/bin/env bash

SDIR=/home/maadeagbo/Documents/DayDream_Engine/Resource/Shaders

# reflect lighting shader and initial file
./bin/dd_shader_reflect -o include/ddShaderReflect.h -e RE_Light \
	-v "$SDIR/Lighting_V.vert" -f "$SDIR/Lighting_F.frag"

# reflect gbuffer shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_GBuffer \
	-v "$SDIR/GBuffer_V.vert" -f "$SDIR/GBuffer_F.frag"

# reflect post process shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_PostPr \
	-v "$SDIR/PostProcess_V.vert" -f "$SDIR/PostProcess_F.frag"

# reflect text shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_Text \
	-v "$SDIR/Text_V.vert" -f "$SDIR/Text_F.frag"

# reflect ping pong shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_PingP \
	-v "$SDIR/PingPong_V.vert" -f "$SDIR/PingPong_F.frag"

# reflect luminance shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_Lumin \
	-c "$SDIR/Luminance_C.comp"

# reflect line segment shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_Line \
	-v "$SDIR/LineSeg_V.vert" -f "$SDIR/LineSeg_F.frag"

# reflect lighting stencil shader shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_LightSt \
	-v "$SDIR/Lighting_Stencil_V.vert" -f "$SDIR/Lighting_Stencil_F.frag"

# reflect normal shadow shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_Shadow \
	-v "$SDIR/Depth_V.vert" -f "$SDIR/Depth_F.frag"

# reflect skinned shadow shader
./bin/dd_shader_reflect -o include/ddShaderReflect.h -a -e RE_ShadowSk \
	-v "$SDIR/SkinnedDepth_V.vert" -f "$SDIR/Depth_F.frag"