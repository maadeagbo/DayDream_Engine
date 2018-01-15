-- Asset file for dead world
-- invoke load functions to load assets

deadworld_asset = {}

function load()
	arg = {["path"] = ""}
	-- mesh
	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	floor_key = load_ddm(arg)
	dd_print( {["output"] = "Created plane mesh: "..floor_key} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/sphere_10.ddm"
	sphere_key = load_ddm(arg)
	dd_print( {["output"] = "Created box mesh: "..sphere_key} )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	box_key = load_ddm(arg)
	dd_print( {["output"] = "Created box mesh: "..box_key} )

	-- light
	arg = { ["id"] = "light_1" }
	key = create_light(arg)
	dd_print( {["output"] = "Created light: "..key} )
	
	-- floor
	args = {
		["id"] = "agent_1",
		["mesh"] = floor_key,
		["scale_x"] = 7.0,
		["scale_y"] = 0.2,
		["scale_z"] = 7.0
	}
	agent_id = create_agent(args)
	deadworld_asset["floor"] = agent_id
	dd_print( {["output"] = "Created agent (floor): "..agent_id} )

	-- circle
	args = {
		["id"] = "agent_2",
		["mesh"] = sphere_key,
		["mass"] = 0.4,
		["type"] = 1,
		["pos_y"] = 7.0,
		["pos_x"] = 0.2,
		["scale_x"] = 0.02,
		["scale_y"] = 0.02,
		["scale_z"] = 0.02
	}
	agent_id = create_agent(args)
	deadworld_asset["sphere"] = agent_id
	dd_print( {["output"] = "Created agent (sphere): "..agent_id} )

	-- box
	args = {
		["id"] = "agent_3",
		["mesh"] = box_key,
		["mass"] = 0.3,
		["pos_y"] = 15.0,
		["pos_x"] = 0.45
	}
	agent_id = create_agent(args)
	deadworld_asset["box"] = agent_id
	--new_s = { ["id"] = agent_id, ["x"] = 0.01, ["y"] = 0.01, ["z"] = 0.01 }
	--set_agent_scale(new_s)
	dd_print( {["output"] = "Created agent (box): "..agent_id} )

	-- circle 2
	args = {
		["id"] = "agent_4",
		["mesh"] = sphere_key,
		["mass"] = 1.0,
		["type"] = 1,
		["pos_y"] = 20.0,
		["pos_x"] = -0.6,
		["scale_x"] = 0.007,
		["scale_y"] = 0.007,
		["scale_z"] = 0.007
	}
	agent_id = create_agent(args)

	-- camera
	args = {
		["id"] = "cam_agent",
		["type"] = -1,
		["pos_y"] = 2.0,
		["pos_z"] = 12.0,
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
	
end
