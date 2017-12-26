-- DeadWorld.lua
-- Level script for Hero and Monsters
do
    base_hero = require "scripts.Hero"
    base_monster = require "scripts.Monster"

    DeadWorld = {}

	main_character = base_hero:new({name = "Slayer"})
    enemies = {}

	function make_person( enitity )
		--e =	{}
    e = {
      ["agent.name"] = enitity.name,
      ["agent.pos.x"] = enitity.position[1],
      ["agent.pos.y"] = enitity.position[2],
      ["agent.pos.z"] = enitity.position[3],
      ["agent.alive"] = enitity.alive
    }
	end

    function DeadWorld:init( map )
		-- spawn hero
		print("Deadworld init called\n")
		main_character.position = { 0.0, 1.0, -2.0 }
		main_character.alive = true
		make_person(main_character)

        -- spawn some monsters
        for i=1,5 do
            new_name = string.format("monster_%d",i)
            enemies[i] = base_monster:new({name = new_name, alive = true})
			make_person(enemies[i])
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
        if event == "post" then
            for k,v in pairs(args) do
                if k == "test_float" then print("Deadworld out val: "..v) end
            end
            e = {}
            e["event_id"] = "DeadWorld post called"
            return e
        end
        return { 0 }
    end

end