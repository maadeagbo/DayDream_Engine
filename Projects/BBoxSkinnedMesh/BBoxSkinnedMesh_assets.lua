-- Asset file for BBoxSkinnedMesh project

BBoxSkinnedMesh_assets = {}

function load()
	arg = {}

	-- Cube mesh
	BBoxSkinnedMesh_assets.cube_m = ddModel.new(ROOT_DIR.."/Resource/Meshes/primitives/cube.ddm")
	ddLib.print( "Created plane mesh: ", BBoxSkinnedMesh_assets.cube_m:id() )

	-- Directional light
	BBoxSkinnedMesh_assets.light_1 = ddLight.new("light_1")
	BBoxSkinnedMesh_assets.light_1:set_active(true)
	ddLib.print( "Created light: ", BBoxSkinnedMesh_assets.light_1:id() )

	-- Floor agent
	--[[
		BBoxSkinnedMesh_assets.floor = ddAgent.new("floor_agent", 0.0, "box")
		BBoxSkinnedMesh_assets.floor:set_scale(100.0, 0.2, 100.0)
		BBoxSkinnedMesh_assets.floor:add_mesh(BBoxSkinnedMesh_assets.cube_m:id(), 0.1, 100.0)
		ddLib.print( "Created agent (floor): ", BBoxSkinnedMesh_assets.floor:id() )
	--]]

	-- Red material
	BBoxSkinnedMesh_assets.red_mat = ddMat.new("mat_red")
	BBoxSkinnedMesh_assets.red_mat:set_base_color(0.5, 0.0, 0.0, 1.0)
	BBoxSkinnedMesh_assets.red_mat:set_specular(0.3)
	
	-- Box agent
	--[[
		BBoxSkinnedMesh_assets.rand_obj = ddAgent.new("rand_obj", 1.0, "box")
		BBoxSkinnedMesh_assets.rand_obj:set_pos(0.0, 5.0, 0.0)
		BBoxSkinnedMesh_assets.rand_obj:add_mesh(BBoxSkinnedMesh_assets.cube_m:id(), 0.1, 100.0)
		BBoxSkinnedMesh_assets.rand_obj:set_mat_at_idx(0, 0, BBoxSkinnedMesh_assets.red_mat:id())
		ddLib.print( "Created agent (rand_obj): ", BBoxSkinnedMesh_assets.rand_obj:id() )
	--]]

	-- Empty camera agent
	BBoxSkinnedMesh_assets.nav_agent = ddAgent.new("BBoxSkinnedMesh_agent", 1.0, "kinematic")
	BBoxSkinnedMesh_assets.nav_agent:set_pos(0.0, 2.0, 5.0)
	BBoxSkinnedMesh_assets.nav_agent:set_eulerPYR(0.0, 0.0)
	ddLib.print( "Created agent (BBoxSkinnedMesh_agent): ", BBoxSkinnedMesh_assets.nav_agent:id() )

	-- Attach camera
	args = {}
	args["id"] = "cam_01"
	args["parent"] = cam_agent_id
	BBoxSkinnedMesh_assets.cam_01 = ddCam.new("cam_01", BBoxSkinnedMesh_assets.nav_agent:id())
	BBoxSkinnedMesh_assets.cam_01:set_active(true)
	BBoxSkinnedMesh_assets.cam_01:set_far(200.0)
	ddLib.print( "Created camera: ", BBoxSkinnedMesh_assets.cam_01:id() )

end
