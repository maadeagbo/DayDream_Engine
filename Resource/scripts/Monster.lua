-- Moster.lua
-- Enemy characters
do
    base_agent = require "scripts.EntityPrototype"

    monster_params = 
    {
        alive = true,
        health = 6.0,
        dmg = 2.0
    }

    Monster = base_agent:new(monster_params)

    -- declare attack
    function Monster:attack( enemy )
        print(string.format( "%s, I KILL YOU!!", enemy.name))
    end

    -- print status
    function Monster:curr_status()
        print(string.format("%s status: \n\z
                            \thealth %.3f", self.name, self.health))
    end

    function Monster:update( event )
        -- body
    end

    return Monster
end