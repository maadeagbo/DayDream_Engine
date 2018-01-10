-- Asset file for dead world
-- invoke load functions to load assets

function load()
	-- mesh
	arg = {
		["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	}
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )
	
	-- camera
	arg = {
		["id"] = "cam_1"
	}
	key = create_cam(arg)
	dd_print( {["output"] = "Created camera: "..key} )
	
	-- light
	arg = {
		["id"] = "light_1"
	}
	key = create_light(arg)
	dd_print( {["output"] = "Created light: "..key} )
	dd_print( {["output"] = "Creating new agent w/ mesh: "..mkey} )
	
	-- agent
	args = {
		["id"] = "agent_1",
		["mesh"] = mkey
	}
	key = create_agent(args)
	dd_print( {["output"] = "Created agent: "..key} )

end