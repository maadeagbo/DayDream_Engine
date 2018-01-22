-- DeadWorld.lua
-- Level script for Hero and Monsters
do
	base_hero = require "scripts.Hero"
	base_monster = require "scripts.Monster"
	matrix = require "scripts.matrix"

	DeadWorld = {}
	speed = 2.0
	max_vel = 5.0
	jump_power = 5.0
	forward_damp = 0.8
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

		--if event == "physics_tick" then
		-- world up 
		world_up = matrix{0, 1, 0}
		-- velocity control
		curr_v = ddAgent_get_velocity({["id"] = assets["cam_ag"]})
		--[[]]
		dd_print(string.format("Vel = %.3f, %.3f, %.3f", 
			curr_v["x"], curr_v["y"], curr_v["z"])
		)
		--]]--
		v3_v = matrix{curr_v["x"], curr_v["y"], curr_v["z"]}
		-- rotation control
		curr_av = ddAgent_get_ang_velocity({["id"] = assets["cam_ag"]})
		v3_av = matrix{curr_av["x"], curr_av["y"], curr_av["z"]}
		-- direction
		curr_d = ddCam_get_direction({["id"] = assets["camera"]})
		v3_d = matrix{curr_d["x"], curr_d["y"], curr_d["z"]}
		--[[
		dd_print(string.format("Dir = %.3f, %.3f, %.3f", 
			curr_d["x"], curr_d["y"], curr_d["z"])
		)
		]]--
		-- "right" direction
		v3_rdir = matrix.cross(v3_d, world_up)
		--[[
		dd_print(string.format(
			"F_dir %.3f, %.3f, %.3f", v3_rdir[1][1], v3_rdir[2][1], v3_rdir[3][1]))
		dd_print(string.format(
			"R_dir %.3f, %.3f, %.3f", v3_d[1][1], v3_d[2][1], v3_d[3][1]))
		]]--

		new_v = matrix{0, 0, 0}
		damping = { ["velocity"] = 0.9 }
		on_ground = curr_v["y"] > -0.01 and curr_v["y"] < 0.01
		--dd_print(string.format("On ground: %s", on_ground and "true" or "false"))
		if not on_ground then damping["velocity"] = 0.0 end

		-- left
		if __dd_input["a"] and on_ground then 
			new_v = new_v - v3_rdir
			new_v[2][1] = 0.0 --remove upward push
			damping["velocity"] = forward_damp
		end
		-- right
		if __dd_input["d"] and on_ground then 
			new_v = new_v + v3_rdir
			new_v[2][1] = 0.0 --remove upward push
			damping["velocity"] = forward_damp
		end
		-- forward
		if __dd_input["w"] and on_ground then 
			new_v = new_v + v3_d
			new_v[2][1] = 0.0 --remove upward push
			damping["velocity"] = forward_damp
		end
		-- backwards
		if __dd_input["s"] and on_ground then 
			new_v = new_v - v3_d
			new_v[2][1] = 0.0 --remove upward push
			damping["velocity"] = forward_damp
		end
		-- up
		if __dd_input["space"] and on_ground then 
			new_v = new_v + matrix{0, jump_power, 0} 
			damping["velocity"] = 0.0
		end
		--[[ down
		if __dd_input["l_shift"] then 
			new_v = new_v - matrix{0, 1, 0}
			damping["velocity"] = 0.1
		end
		]]-- rotation
		if __dd_input["mouse_b_l"] then
			--
			pitch = pitch + __dd_input["mouse_y_delta"] * 1.0/speed
			yaw = yaw + __dd_input["mouse_x_delta"] * 1.0/speed

			new_cr = {}
			new_cr["id"] = assets["camera"]
			--dd_print(string.format("%d", assets["camera"]))
			new_cr["pitch"] = pitch
			ddCam_rotate(new_cr)

			new_r = {}
			new_r["id"] = assets["cam_ag"]
			new_r["yaw"] = yaw
			ddAgent_set_rotation(new_r)
			--[[
			dd_print(string.format("Pitch = %.3f || Yaw = %.3f", 
				new_r["pitch"], new_r["yaw"]))
			]]--
		end

		-- damping
		damping["id"] = assets["cam_ag"]
		ddAgent_set_damping(damping)

		-- update position (and clamp maximum velocity difference w/ deceleration)
		new_v = v3_v + new_v * speed;
		--[[
		delta_v = {
			["x"] = clamp_vel(new_v[1][1] - curr_v["x"], -max_vel, max_vel),
			["y"] = clamp_vel(new_v[2][1] - curr_v["y"], -max_vel, max_vel),
			["z"] = clamp_vel(new_v[3][1] - curr_v["z"], -max_vel, max_vel)
		}
		]]--
		delta_v = {
			["x"] = clamp_vel(new_v[1][1], -max_vel, max_vel),
			["y"] = clamp_vel(new_v[2][1], -max_vel, max_vel),
			["z"] = clamp_vel(new_v[3][1], -max_vel, max_vel)
		}

		delta_v["id"] = assets["cam_ag"]
		ddAgent_set_velocity(delta_v)
		--if update_rot then rotate_camera(cam_rot) end
		--end
	end
end