@ECHO OFF

set SDIR=%1

:: reflect lighting shader and initial file
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -e RE_Light^
	-v "%SDIR%\Resource\Shaders\Lighting_V.vert" ^
	-f "%SDIR%\Resource\Shaders\Lighting_F.frag"

:: reflect gbuffer shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_GBuffer^
	-v "%SDIR%\Resource\Shaders\GBuffer_V.vert" ^
	-f "%SDIR%\Resource\Shaders\GBuffer_F.frag"

:: reflect post process shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_PostPr^
	-v "%SDIR%\Resource\Shaders\PostProcess_V.vert" ^
	-f "%SDIR%\Resource\Shaders\PostProcess_F.frag"

:: reflect text shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_Text^
	-v "%SDIR%\Resource\Shaders\Text_V.vert" ^
	-f "%SDIR%\Resource\Shaders\Text_F.frag"

:: reflect ping pong shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_PingP^
	-v "%SDIR%\Resource\Shaders\PingPong_V.vert" ^
	-f "%SDIR%\Resource\Shaders\PingPong_F.frag"

:: reflect luminance shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_Lumin^
	-c "%SDIR%\Resource\Shaders\Luminance_C.comp"

:: reflect line segment shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_Line^
	-v "%SDIR%\Resource\Shaders\LineSeg_V.vert" ^
	-f "%SDIR%\Resource\Shaders\LineSeg_F.frag"

:: reflect lighting stencil shader shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_LightSt^
	-v "%SDIR%\Resource\Shaders\Lighting_Stencil_V.vert" ^
	-f "%SDIR%\Resource\Shaders\Lighting_Stencil_F.frag"

:: reflect normal shadow shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_Shadow^
	-v "%SDIR%\Resource\Shaders\Depth_V.vert" ^
	-f "%SDIR%\Resource\Shaders\Depth_F.frag"

:: reflect skinned shadow shader
%cd%\bin\dd_shader_reflect.exe -o includes\ddShaderReflect.h -a -e RE_ShadowSk^
	-v "%SDIR%\Resource\Shaders\SkinnedDepth_V.vert" ^
	-f "%SDIR%\Resource\Shaders\Depth_F.frag"
