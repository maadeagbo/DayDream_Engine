-- Asset file for dead world
-- invoke load functions to load assets

deadworld_asset = {}

function load()
	arg = {["path"] = ""}
	-- mesh
	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	floor_key = dd_load_ddm(arg)
	dd_print( "Created plane mesh: "..floor_key )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/sphere_10.ddm"
	sphere_key = dd_load_ddm(arg)
	dd_print( "Created box mesh: "..sphere_key )

	arg["path"] = ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm"
	box_key = dd_load_ddm(arg)
	dd_print( "Created box mesh: "..box_key )

	arg["path"] = "C:/Users/Moses/Documents/kachujin/Kachujin_0.ddm"
	--ninja_key = dd_load_ddm(arg)
	--dd_print( "Created ninja mesh: "..ninja_key )

	-- light
	arg = { ["id"] = "light_1" }
	key = dd_create_light(arg)
	dd_print( "Created light: "..key )
	
	-- floor
	args = {
		["id"] = "agent_1",
		["mesh"] = floor_key,
		["scale_x"] = 50.0,
		["scale_y"] = 0.2,
		["scale_z"] = 50.0
	}
	agent_id = dd_create_agent(args)
	deadworld_asset["floor"] = agent_id
	dd_print( "Created agent (floor): "..agent_id )

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
	b_agent_id = dd_create_agent(args)
	deadworld_asset["sphere"] = b_agent_id
	dd_print( "Created agent (sphere): "..b_agent_id )

	-- box
	args = {
		["id"] = "agent_3",
		["mesh"] = box_key,
		["mass"] = 0.9,
		["pos_y"] = 11.0,
		["pos_x"] = 0.48
	}
	agent_id = dd_create_agent(args)
	deadworld_asset["box"] = agent_id
	dd_print( "Created agent (box): "..agent_id )

	-- circle 2
	args = {
		["id"] = "agent_4",
		["mesh"] = sphere_key,
		["mass"] = 1.9,
		["type"] = 1,
		["pos_y"] = 20.0,
		["pos_x"] = -3.9,
		["scale_x"] = 0.007,
		["scale_y"] = 0.007,
		["scale_z"] = 0.007
	}
	agent_id = dd_create_agent(args)

	-- camera
	args = {
		["id"] = "cam_agent",
		["mass"] = 0.1,
		["type"] = -1,
		["pos_y"] = 5.0,
		["pos_z"] = 12.0,
	}
	new_agent_id = dd_create_agent(args)
	deadworld_asset["cam_ag"] = new_agent_id
	arg = {
		["id"] = "cam_2",
		["parent"] = new_agent_id
	}
	key = dd_create_camera(arg)
	deadworld_asset["camera"] = key
	dd_print( "Created camera: "..key )
	
	-- box 2
	args = {
		["id"] = "agent_5",
		["parent"] = new_agent_id,
		["mesh"] = sphere_key,
		["mass"] = 1.0,
		["type"] = -2,
		["scale_x"] = 0.007,
		["scale_y"] = 0.007,
		["scale_z"] = 0.007,
		["pos_y"] = 5.0,
		["pos_z"] = 9.0
	}
	agent_id = dd_create_agent(args)
	deadworld_asset["box2"] = agent_id
	dd_print( "Created agent (box 2): "..agent_id )

end
