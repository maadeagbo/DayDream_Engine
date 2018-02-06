-- Config file for loading levels and other settings
load_engine = require "scripts.EngineInit"

-- Limit of 10 different projects at once
local worlds = {
	"kinect",
	"dead"
}

function generate_levels( event, args, num_args )
	-- creates a list of found level scripts
	levels = {}
	for i,v in ipairs(worlds) do
		loc = string.format("%s/%s/%s", PROJECTS_DIR, v, v)
		filename = string.format( "%s_world.lua", loc)
		filename_a = string.format( "%s_assets.lua", loc)
		
		_file = io.open(filename)					-- check world file exists
		_file_a = io.open(filename_a)			-- check asset file exists

		if _file and _file_a then
			levels[#levels + 1] = v					-- add to output
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