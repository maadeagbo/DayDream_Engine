-- Config file for loading levels and other settings

local inputs =
{
    "scripts/DeadWorld"
}

function generate_levels( event, args, num_args )
    -- creates a list of found level scripts
    levels = {}
    for i,v in ipairs(inputs) do
        filename = string.format( "%s%s.lua", ROOT_DIR, v)
        _file = io.open(filename)   -- check file exists
        if _file then 
            levels[#levels + 1] = v -- add to output
        else
            print("Not found: "..filename)
        end
    end
    levels["num_levels"] = #levels
    return levels
end