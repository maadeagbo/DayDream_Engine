-- Config file for loading levels and other settings
load_engine = require "scripts.EngineInit"

local worlds = {
	{ PROJECTS_DIR.."/dead_lvl", "dead" },
	{ PROJECTS_DIR.."/kinect_lvl", "kinect" }
}

function generate_levels( event, args, num_args )
	-- creates a list of found level scripts
	levels = {}
	for i,v in ipairs(worlds) do
		filename = string.format( "%s/%s_world.lua", v[1], v[2])
		filename_a = string.format( "%s/%s_assets.lua", v[1], v[2])
		
		_file = io.open(filename)					-- check world file exists
		_file_a = io.open(filename_a)			-- check asset file exists

		if _file and _file_a then
			levels[#levels + 1] = v[2] 			-- add to output
			-- clean up
			_file.close()
			_file_a.close()
		else
			if not _file then print("Not found: "..filename) end
			if not _file_a then print("Not found: "..filename_a) end
		end
	end
	-- return # of levels and their ID's to dd_engine
	levels["num_levels"] = #levels
	return levels
end