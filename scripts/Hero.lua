-- Hero.lua
-- Main character hero script
do
    base_agent = require "scripts.EntityPrototype"

    hero_params = 
    {
        alive = true,
        health = 10.0,
        armour = 3.0,
        dmg = 3.0,
		callbacks = { "update" }
    }

    Hero = base_agent:new(hero_params)

    -- declare attack
    function Hero:attack( enemy )
        print(string.format( "%s, I KILL YOU!!", enemy.name))
    end

    function Hero:curr_status()
        print(string.format("%s status: \n\z
                            \thealth %.3f \n\tarmour %.3f", 
                            self.name, self.health, self.armour))
    end

    function Hero:update( event )
        -- body
		print("I have been ENVOKED")
    end

    return Hero
end