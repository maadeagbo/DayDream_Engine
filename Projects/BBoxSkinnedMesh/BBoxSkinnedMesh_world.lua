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
		dd_subscribe( { key = assets.bb_m.id, event = "new_bbox" } )

		-- get controller for lvl
		assets.bb_m.ctrl = Lvlctrl.get()

		ddLib.print( "BBoxSkinnedMesh init called." )
	end

	function BBoxSkinnedMesh:update( event, args, num_args )
		assets = BBoxSkinnedMesh_assets

		if event == "update" then
			-- Push level-specific update event
			dd_push( {event_id = level_tag} )

			-- init gpu buffers
			if first then
				init_gpu_stuff()

				first = false
			end
		end
	end

end
