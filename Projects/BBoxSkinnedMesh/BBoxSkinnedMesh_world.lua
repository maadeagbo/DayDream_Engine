-- World file for BBoxSkinnedMesh project

do
	matrix = require "scripts.matrix"
	actor_prototype = require "BBoxSkinnedMesh.BBoxSkinnedMesh_actor"
	bbox_manager = require "BBoxSkinnedMesh.ManageBBox"

	BBoxSkinnedMesh = {}

	first = true

	level_tag = "BBoxSkinnedMesh_update"

	function BBoxSkinnedMesh:init( event, args, num_args )
		-- Make assets object local
		assets = BBoxSkinnedMesh_assets
		assets.level_tag = level_tag

		-- Create actor controller and set variables
		BBoxSkinnedMesh.actor = actor_prototype:new( {name = "actor_01"} )
		-- Register actor callback
		dd_register_callback(BBoxSkinnedMesh.actor.name, BBoxSkinnedMesh.actor)
		-- Subscribe to BBoxSkinnedMesh_world generated callback
		dd_subscribe( {key = BBoxSkinnedMesh.actor.name, event = level_tag} )

		-- Create & Register bbox manager callback
		assets.bb_m = bbox_manager:new( {id = "bb_m"} )
		dd_register_callback(assets.bb_m.id, assets.bb_m)
		dd_subscribe( { key = assets.bb_m.id, event = level_tag } )
		--dd_subscribe( { key = assets.bb_m.id, event = "new_bbox" } )
		--dd_subscribe( { key = assets.bb_m.id, event = "export_bbox" } )

		-- get controller for lvl
		assets.bb_m.ctrl = Lvlctrl.get()

		-- create demo sample model
		create_model()

		ddLib.print( "BBoxSkinnedMesh init called." )
	end

	function BBoxSkinnedMesh:update( event, args, num_args )
		assets = BBoxSkinnedMesh_assets

		if event == "update" then
			-- Push level-specific update event
			dd_push( {event_id = level_tag} )
			assets.sample_clip[ddEnums.LOCAL_TIME] = 
				assets.sample_clip[ddEnums.LOCAL_TIME] + ddLib.ftime() * 
				assets.sample_clip[ddEnums.PLAY_SPEED]

			-- init gpu buffers
			if first then
				init_gpu_stuff()

				first = false
			end
		end
	end

	function create_model()
		-- Make assets object local
		assets = BBoxSkinnedMesh_assets
  
		-- load skeleton
		ddAnimation.load_skeleton("sk_1", 
			PROJECT_DIR.."/BBoxSkinnedMesh/sample/vanguard_t_choonyung.ddb")
		
		-- load animation clip
		ddAnimation.load_clip("sample01", 
			PROJECT_DIR.."/BBoxSkinnedMesh/sample/Walking2_0.dda")
		
		-- load mesh
		assets.sample_mesh = ddModel.new(
			PROJECT_DIR.."/BBoxSkinnedMesh/sample/vanguard.ddg")
		
		-- load agent
		assets.sample = ddAgent.new("sample_agent", 0.0, "box")
		assets.sample:set_pos(0.0, 0.0, 0.0)
	
		-- add mesh to agent
		assets.sample:add_mesh(assets.sample_mesh:id(), 0.1, 100.0)
		
		-- add skeleton to agent
		assets.sample:set_skeleton("sk_1")
		
		-- add clip to agent
		assets.sample_clip = assets.sample:add_animation("sample_walk", "sample01")
		assets.sample_clip[ddEnums.ACTIVE] = true
		assets.sample_clip[ddEnums.PLAY_SPEED] = 1.2
		assets.sample_clip[ddEnums.LOCAL_TIME] = 0.01
		--assets.sample_clip[ddEnums.INTERPOLATE] = false
		assets.sample_clip[ddEnums.LOOP] = true
	
		-- add specialized bounding box to agent
		assets.sample:add_oobb(PROJECT_DIR.."/BBoxSkinnedMesh/output/first.ddx")
		
		ddLib.print("sample: ", tostring(assets.sample))
	end

end
