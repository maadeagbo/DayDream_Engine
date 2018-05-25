@ECHO OFF

set SDIR=%cd%
set SHDIR="%SDIR%\Resource\Shaders"
set REFLECT_H="%SDIR%\include\ddShaderReflect.h"

:: reflect lighting shader and initial file
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -e RE_Light^
	-v "%SHDIR%\Lighting_V.vert" ^
	-f "%SHDIR%\Lighting_F.frag"

:: reflect gbuffer shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_GBuffer^
	-v "%SHDIR%\GBuffer_V.vert" ^
	-f "%SHDIR%\GBuffer_F.frag"

:: reflect post process shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_GBufferSk^
	-v "%SHDIR%\SkinnedGBuffer_V.vert" ^
	-f "%SHDIR%\GBuffer_F.frag"

:: reflect post process shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_PostPr^
	-v "%SHDIR%\PostProcess_V.vert" ^
	-f "%SHDIR%\PostProcess_F.frag"

:: reflect text shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_Text^
	-v "%SHDIR%\Text_V.vert" ^
	-f "%SHDIR%\Text_F.frag"

:: reflect ping pong shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_PingP^
	-v "%SHDIR%\PingPong_V.vert" ^
	-f "%SHDIR%\PingPong_F.frag"

:: reflect luminance shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_Lumin^
	-c "%SHDIR%\Luminance_C.comp"

:: reflect line segment shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_Line^
	-v "%SHDIR%\LineSeg_V.vert" ^
	-f "%SHDIR%\LineSeg_F.frag"

:: reflect lighting stencil shader shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_LightSt^
	-v "%SHDIR%\Lighting_Stencil_V.vert" ^
	-f "%SHDIR%\Lighting_Stencil_F.frag"

:: reflect normal shadow shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_Shadow^
	-v "%SHDIR%\Depth_V.vert" ^
	-f "%SHDIR%\Depth_F.frag"

:: reflect skinned shadow shader
%SDIR%\bin\shader_reflect.exe -o %REFLECT_H% -a -e RE_ShadowSk^
	-v "%SHDIR%\SkinnedDepth_V.vert" ^
	-f "%SHDIR%\Depth_F.frag"
