-- DeadWorld.lua
-- Level script for Hero and Monsters
do
	base_hero = require "scripts.Hero"
	base_monster = require "scripts.Monster"
	matrix = require "scripts.matrix"

	DeadWorld = {}
	speed = 10.0
	max_vel_diff = 10.0
	damp_factor = 2.0
	pitch = 0
	yaw = 0

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

	function clamp_vel(val, min, max)
		-- clamps val between min and max
		if val < min then return min end
		if val > max then return max end
		return val
	end

	function DeadWorld:update( event, args, num_args )
		assets = deadworld_asset

		if event == "physics_tick" then
			-- velocity control
			curr_v = get_current_velocity_agent({["id"] = assets["cam_ag"]})
			v3_v = matrix{curr_v["x"], curr_v["y"], curr_v["z"]}
			-- rotation control
			curr_av = get_current_ang_velocity_agent({["id"] = assets["cam_ag"]})
			v3_av = matrix{curr_av["x"], curr_av["y"], curr_av["z"]}
			-- direction
			curr_d = ddCam_get_direction({["id"] = assets["camera"]})
			v3_d = matrix{curr_d["x"], curr_d["y"], curr_d["z"]}
			dd_print(string.format("Dir = %.3f, %.3f, %.3f", 
				curr_d["x"], curr_d["y"], curr_d["z"])
			)

			new_v = matrix{0, 0, 0}
			
			-- left
			if __dd_input["a"] then new_v = -1 * matrix{speed, 0, 0} end
			-- right
			if __dd_input["d"] then new_v = matrix{speed, 0, 0} end
			-- forward
			if __dd_input["w"] then new_v = -1 * matrix{0, 0, speed} end
			-- backwards
			if __dd_input["s"] then new_v = matrix{0, 0, speed} end
			-- up
			if __dd_input["q"] then new_v = matrix{0, speed, 0} end
			-- down
			if __dd_input["e"] then new_v = -1 * matrix{0, speed, 0} end
			-- rotation
			if __dd_input["mouse_b_l"] then
				--
				pitch = pitch + __dd_input["mouse_y_delta"] * 1.0/speed
				yaw = yaw + __dd_input["mouse_x_delta"] * 1.0/speed

				new_cr = {}
				new_cr["id"] = assets["camera"]
				--dd_print(string.format("%d", assets["camera"]))
				new_cr["yaw"] = yaw
				dd_print(""..assets["camera"])
				rotate_camera(new_cr)

				new_r = {}
				new_r["id"] = assets["cam_ag"]
				new_r["pitch"] = pitch
				new_r["yaw"] = 0

				set_agent_rotation(new_r)
				--[[
				dd_print(string.format("Pitch = %.3f || Yaw = %.3f", 
					new_r["pitch"], new_r["yaw"]))
				]]--
			end

			-- damping
			if new_v[3][1] < 0.01 and new_v[3][1] > -0.01 then
				--new_v[3][1] = -v3_v[3][1] * damp_factor
			end
			if new_v[2][1] < 0.01 and new_v[2][1] > -0.01 then
				--new_v[2][1] = -v3_v[2][1] * damp_factor
			end
			if new_v[1][1] < 0.01 and new_v[1][1] > -0.01 then
				--new_v[1][1] = -v3_v[1][1] * damp_factor
			end

			-- update position (and clamp maximum velocity difference)
			delta_v = {
				["x"] = clamp_vel(new_v[1][1] - curr_v["x"], -max_vel_diff, max_vel_diff),
				["y"] = clamp_vel(new_v[2][1] - curr_v["y"], -max_vel_diff, max_vel_diff),
				["z"] = clamp_vel(new_v[3][1] - curr_v["z"], -max_vel_diff, max_vel_diff)
			}

			delta_v["id"] = assets["cam_ag"]
			apply_force_agent(delta_v)
			--if update_rot then rotate_camera(cam_rot) end
		end
	end
end