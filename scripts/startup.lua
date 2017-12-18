-- Config file for loading levels and other settings

local inputs =
{
    "DeadWorld"
}

function generate_levels( event, args, num_args )
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