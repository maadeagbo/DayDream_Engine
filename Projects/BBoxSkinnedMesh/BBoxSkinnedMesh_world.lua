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

		-- create demo arissa model
		create_model()

		ddLib.print( "BBoxSkinnedMesh init called." )
	end

	function BBoxSkinnedMesh:update( event, args, num_args )
		assets = BBoxSkinnedMesh_assets

		if event == "update" then
			-- Push level-specific update event
			dd_push( {event_id = level_tag} )
			assets.arissa_clip[ddEnums.LOCAL_TIME] = 
				assets.arissa_clip[ddEnums.LOCAL_TIME] - ddLib.ftime() * 
				assets.arissa_clip[ddEnums.PLAY_SPEED]

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
			PROJECT_DIR.."/BBoxSkinnedMesh/arissa/arissa.ddb")
		
		-- load animation clip
		ddAnimation.load_clip("sample", 
			PROJECT_DIR.."/BBoxSkinnedMesh/arissa/Fast_Run_0.dda")
		
		-- load mesh
		assets.arissa_mesh = ddModel.new(
			PROJECT_DIR.."/BBoxSkinnedMesh/arissa/arissa.ddg")
		
		-- load agent
		assets.arissa = ddAgent.new("arissa", 0.0, "box")
		assets.arissa:set_pos(1.0, 0.0, 0.0)
	
		-- add mesh to agent
		assets.arissa:add_mesh(assets.arissa_mesh:id(), 0.1, 100.0)
		
		-- add skeleton to agent
		assets.arissa:set_skeleton("sk_1")
		-- add clip to agent
		assets.arissa_clip = assets.arissa:add_animation("arissa_walk", "sample")
		assets.arissa_clip[ddEnums.ACTIVE] = true
		assets.arissa_clip[ddEnums.PLAY_SPEED] = 0.5
		assets.arissa_clip[ddEnums.LOOP] = true
	
		-- add specialized bounding box to agent
		assets.arissa:add_oobb(PROJECT_DIR.."/BBoxSkinnedMesh/output/first.ddx")
		
		ddLib.print("Arissa: ", tostring(assets.arissa))
	end

end
