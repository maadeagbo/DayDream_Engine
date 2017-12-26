-- Config file for loading levels and other settings

local inputs =
{
    "DeadWorld"
}

local function test_01()
	arg = {
		["path"] = "C:/Users/Moses/Documents/DayDream_Engine/Resource/Meshes/primitives/cube.ddm"
	}
	mkey = load_ddm(arg);
	print("Created mesh: "..mkey)
	arg = {
		["id"] = "cam_1"
	}
	key = create_cam(arg)
	print("Created camera: "..key)
	arg = {
		["id"] = "light_1"
	}
	key = create_light(arg)
	print("Created light: "..key)
	print("Creating new agent w/ mesh: "..mkey)
	args = {
		["id"] = "agent_1",
		["mesh"] = mkey
	}
	key = create_agent(args)
	print("Created agent: "..key)
end

function generate_levels( event, args, num_args )
		
		test_01()

    -- creates a list of found level scripts
    levels = {}
    for i,v in ipairs(inputs) do
				filename = string.format( "%sscripts/%s.lua", ROOT_DIR, v)
				filename_a = string.format( "%sscripts/%s_assets.lua", ROOT_DIR, v)
				_file = io.open(filename)					-- check file exists
				_file_a = io.open(filename_a)			-- check asset file exists
				if _file and _file_a then 
						levels[#levels + 1] = v -- add to output
				else
						if not _file then print("Not found: "..filename) end
						if not _file_a then print("Not found: "..filename_a) end
				end
    end
    levels["num_levels"] = #levels
    return levels
end