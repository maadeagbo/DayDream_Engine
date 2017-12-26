-- Asset file for dead world
-- invoke load functions to load assets

function load()
	if WIN32 then
		arg = {
			["path"] = "C:/Users/Moses/Documents/DayDream_Engine/Resource/Meshes/primitives/cube.ddm"
		}
	elseif LINUX then
		arg = {
			["path"] = "/home/maadeagbo/Documents/DayDream_Engine/Resource/Meshes/primitives/cube.ddm"
		}
	end
	mkey = load_ddm(arg);
	dd_print( {["output"] = "Created mesh: "..mkey} )
	arg = {
		["id"] = "cam_1"
	}
	key = create_cam(arg)
	dd_print( {["output"] = "Created camera: "..key} )
	arg = {
		["id"] = "light_1"
	}
	key = create_light(arg)
	dd_print( {["output"] = "Created light: "..key} )
	print()
	dd_print( {["output"] = "Creating new agent w/ mesh: "..mkey} )
	args = {
		["id"] = "agent_1",
		["mesh"] = mkey
	}
	key = create_agent(args)
	dd_print( {["output"] = "Created agent: "..key} )
end