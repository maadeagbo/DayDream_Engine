-- Engine initialization script

local args = {}

-- Shadow resolution **********************************************************

args = {
	["width"] = 1024,
	["height"] = 1024
}
shadow_resolution(args)

-- Cube map resolution ********************************************************

args = {
	["width"] = 1024,
	["height"] = 1024
}
cubemap_resolution(args)

-- Light volume plane *********************************************************

args = {
	["path"] = ROOT_DIR.."/Resource/Meshes/primitives/light_plane.ddm"
}
local lplane = load_ddm(args)
args = {
	["id"] = "_dd_light_plane",
	["mesh"] = lplane
}
local lplane_id = create_agent(args)
print("Found and added light plane: "..lplane_id)

-- Shaders ********************************************************************

args = {
	["path"] = "",
	["type"] = 0,
	["id"] = ""
}

-- geometry
args["path"] = ROOT_DIR.."/Resource/Shaders/GBuffer_V.vert"
args["type"] = VERT_SHADER
args["id"] = "geometry"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/GBuffer_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- light
args["path"] = ROOT_DIR.."/Resource/Shaders/Lighting_V.vert"
args["type"] = VERT_SHADER
args["id"] = "light"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/Lighting_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- post processing
args["path"] = ROOT_DIR.."/Resource/Shaders/PostProcess_V.vert"
args["type"] = VERT_SHADER
args["id"] = "postprocess"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/PostProcess_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- text
args["path"] = ROOT_DIR.."/Resource/Shaders/Text_V.vert"
args["type"] = VERT_SHADER
args["id"] = "text"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/Text_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- ping pong
args["path"] = ROOT_DIR.."/Resource/Shaders/PingPong_V.vert"
args["type"] = VERT_SHADER
args["id"] = "pingpong"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/PingPong_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- luminance
args["path"] = ROOT_DIR.."/Resource/Shaders/Luminance_C.comp"
args["type"] = COMP_SHADER
args["id"] = "luminance"
add_shader(args)

-- line render
args["path"] = ROOT_DIR.."/Resource/Shaders/LineSeg_V.vert"
args["type"] = VERT_SHADER
args["id"] = "line"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/LineSeg_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- light stencil
args["path"] = ROOT_DIR.."/Resource/Shaders/Lighting_Stencil_V.vert"
args["type"] = VERT_SHADER
args["id"] = "light_stencil"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/Lighting_Stencil_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- shadow
args["path"] = ROOT_DIR.."/Resource/Shaders/Depth_V.vert"
args["type"] = VERT_SHADER
args["id"] = "shadow"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/Depth_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- skinned shadow
args["path"] = ROOT_DIR.."/Resource/Shaders/SkinnedDepth_V.vert"
args["type"] = VERT_SHADER
args["id"] = "shadow_skinned"
add_shader(args)
args["path"] = ROOT_DIR.."/Resource/Shaders/Depth_F.frag"
args["type"] = FRAG_SHADER
add_shader(args)

-- Load screen ****************************************************************

args = {
	["path"] = ROOT_DIR.."/Resource/Textures/loading_base.png",
	["id"] = "load_screen"
}
create_texture(args)
