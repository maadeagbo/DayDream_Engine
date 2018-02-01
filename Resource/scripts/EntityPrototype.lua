-- EntityPrototype.lua
-- Entity character script
-- uses DummyAgent as handle to application
do
    Entity = 
    {
        name = "not_set"
    }

    function Entity:new(params)
        params = params or {}
        self.__index = self;
        setmetatable(params, self)

        return params;
    end

    -- check life status
    function Entity:is_alive()
        print(string.format( "%s:alive = %s", self.name, 
            self.alive and "true" or "false"))
    end

    function Entity:initialize()
        -- body
    end

    function Entity:update( event, args, num_args )
        -- body
    end

    return Entity
end