-- DeadWorld.lua
-- Level script for Hero and Monsters
do
	base_hero = require "scripts.Hero"
	base_monster = require "scripts.Monster"

	DeadWorld = {}
	ticks = 0.0
	speed = 5.0

	main_character = base_hero:new({name = "Slayer"})
		enemies = {}

	function make_person( enitity )
		--e = {}
		e = {
			["agent.name"] = enitity.name,
			["agent.pos.x"] = enitity.position[1],
			["agent.pos.y"] = enitity.position[2],
			["agent.pos.z"] = enitity.position[3],
			["agent.alive"] = enitity.alive
		}
	end

	function DeadWorld:init( event, args, num_args )
		-- spawn hero
		print("Deadworld init called\n")
		main_character.position = { 0.0, 1.0, -2.0 }
		main_character.alive = true

		-- spawn some monsters
		for i=1,5 do
			new_name = string.format("monster_%d",i)
			enemies[i] = base_monster:new({name = new_name, alive = true})
		end

	end

	function DeadWorld:world_status()
			main_character:curr_status()
			print(string.format("# of Monsters = %d", #(enemies)))
			for i,v in ipairs(enemies) do
					v:curr_status()
			end
	end

	function DeadWorld:update( event, args, num_args )
		ticks = ticks + __frame_time

		-- get camera position
		old_pos = get_agent_ls_pos( {["id"] = deadworld_asset["cam_ag"]} )
		--[[
		out = { ["output"] = string.format("Cam pos = %.3f, %.3f, %.3f", 
			old_pos["x"], old_pos["y"], old_pos["z"]) 
		}
		dd_print(out)
		--]]

		-- input
		cam_pos = { ["id"] = deadworld_asset["cam_ag"] }
		update_pos = false
		update_rot = false
		if __dd_input["a"]["active"] then
			--
			cam_pos["x"] = old_pos["x"] - speed * __frame_time
			update_pos = true
		end
		if __dd_input["d"]["active"] then
			--
			cam_pos["x"] = old_pos["x"] + speed * __frame_time
			update_pos = true
		end
		if __dd_input["w"]["active"] then
			--
			cam_pos["z"] = old_pos["z"] - speed * __frame_time
			update_pos = true
		end
		if __dd_input["s"]["active"] then
			--
			cam_pos["z"] = old_pos["z"] + speed * __frame_time
			update_pos = true
		end

		-- update position
		if update_pos then set_agent_pos(cam_pos) end
	end

end