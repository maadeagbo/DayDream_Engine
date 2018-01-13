-- Asset file for dead world
-- invoke load functions to load assets

deadworld_asset = {
	["cam_id"] = 0,
	["cam_agent_id"] = 0,
	["box"] = 0
}

function load()
	-- mesh
	arg = {
		["path"] = ROOT_DIR.."/Resource/Meshes/primitives/tempmesh.ddm"
	}
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )
	
	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cylinder_10.ddm"
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/plane.ddm"
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/sphere_10.ddm"
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	mkey = load_ddm(arg)
	dd_print( {["output"] = "Created mesh: "..mkey} )

	-- light
	arg = {
		["id"] = "light_1"
	}
	key = create_light(arg)
	dd_print( {["output"] = "Created light: "..key} )
	dd_print( {["output"] = "Creating new agent w/ mesh: "..mkey} )
	
	-- agent (box)
	args = {
		["id"] = "agent_1",
		["mesh"] = mkey
	}
	agent_id = create_agent(args)
	deadworld_asset["box"] = agent_id
	new_p = { ["id"] = agent_id, ["x"] = 0.0, ["y"] = 0.0, ["z"] = 0.0 }
	set_agent_pos(new_p)
	new_s = { ["id"] = agent_id, ["x"] = 0.1, ["y"] = 0.1, ["z"] = 0.1 }
	set_agent_scale(new_s)
	dd_print( {["output"] = "Created agent: "..agent_id} )

	-- camera
	args = {
		["id"] = "cam_agent"
	}
	new_agent_id = create_agent(args)
	deadworld_asset["cam_agent_id"] = new_agent_id
	arg = {
		["id"] = "cam_1",
		["parent"] = new_agent_id
	}
	key = create_cam(arg)
	deadworld_asset["cam_id"] = key
	dd_print( {["output"] = "Created camera: "..key} )
	-- set camera position
	new_p = { ["id"] = new_agent_id, ["x"] = 0.0, ["y"] = 0.0, ["z"] = -10.0 }
	set_agent_pos(new_p)
	-- set camera rotation
	new_r = { ["id"] = new_agent_id, ["x"] = 0.0, ["y"] = 10.0, ["z"] = 0.0 }
	set_agent_rot(new_r)
	
end
