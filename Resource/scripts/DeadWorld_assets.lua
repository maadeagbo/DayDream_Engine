-- Asset file for dead world
-- invoke load functions to load assets

deadworld_asset = {
	["cam_id"] = 0,
	["cam_ag"] = 0,
	["floor"] = 0,
	["box"] = 0
}

function load()
	arg = {["path"] = ""}
	-- mesh
	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	floor_key = load_ddm(arg)
	dd_print( {["output"] = "Created plane mesh: "..floor_key} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/sphere_10.ddm"
	box_key = load_ddm(arg)
	dd_print( {["output"] = "Created box mesh: "..box_key} )

	-- light
	arg = { ["id"] = "light_1" }
	key = create_light(arg)
	dd_print( {["output"] = "Created light: "..key} )
	
	-- floor
	args = {
		["id"] = "agent_1",
		["mesh"] = floor_key
	}
	agent_id = create_agent(args)
	deadworld_asset["floor"] = agent_id
	new_p = { ["id"] = agent_id, ["x"] = 0.0, ["y"] = 0.0, ["z"] = 0.0 }
	set_agent_pos(new_p)
	new_s = { ["id"] = agent_id, ["x"] = 3.0, ["y"] = 0.75, ["z"] = 3.0 }
	set_agent_scale(new_s)
	dd_print( {["output"] = "Created agent (floor): "..agent_id} )

	-- box
	args = {
		["id"] = "agent_2",
		["mesh"] = box_key,
		["mass"] = 0.1, 
		["type"] = 1
	}
	agent_id = create_agent(args)
	deadworld_asset["box"] = agent_id
	new_p = { ["id"] = agent_id, ["x"] = 0.0, ["y"] = 20.0, ["z"] = 0.0 }
	set_agent_pos(new_p)
	new_s = { ["id"] = agent_id, ["x"] = 0.01, ["y"] = 0.01, ["z"] = 0.01 }
	set_agent_scale(new_s)
	dd_print( {["output"] = "Created agent (box): "..agent_id} )

	-- camera
	args = {
		["id"] = "cam_agent"
	}
	new_agent_id = create_agent(args)
	deadworld_asset["cam_ag"] = new_agent_id
	arg = {
		["id"] = "cam_1",
		["parent"] = new_agent_id
	}
	key = create_cam(arg)
	deadworld_asset["cam_id"] = key
	dd_print( {["output"] = "Created camera: "..key} )
	-- set camera position
	new_p = { ["id"] = new_agent_id, ["x"] = 0.0, ["y"] = 1.0, ["z"] = 10.0 }
	set_agent_pos(new_p)
	
end
