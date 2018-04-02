do
  manager = { ctrl = nil }

  created_bboxs = {}
  bbox_select = false
  current_bbox = 0

  gap_click = 0
  gap_tap = 0

  _T = 0
  _R = 1
  _S = 2
  _X = 3
  _Y = 4
  _Z = 5

  function manager:new( params )
		params = params or {}
		self.__index = self
		setmetatable(params, self)

		return params
  end

  function write_bbox_file( bbox, file_name )
    -- create file
    -- find global transform position (offset from (0, 0, 0) )
    -- get extents of min & max vertex and output scale vector
  end
  
  function manager:modify_bbox( ... )
    -- body
  end

  function manager:update( event, args, num_args )
    if event == "new_bbox" then
      -- create new bounding box
      idx = #created_bboxs + 1
      created_bboxs[idx] = ddBBox.new()
      created_bboxs[idx].pos = {0.0, 0.5, 0.0}
      created_bboxs[idx].scale = {0.5, 0.5, 0.5}

      pos = created_bboxs[idx].pos
      ddLib.print("Position: ", pos.x, ", ", pos.y, ", ", pos.z)
    elseif event == "BBoxSkinnedMesh_update" then
      -- regular update
      assets = BBoxSkinnedMesh_assets
    
      -- modification window
      if bbox_select then
        ddBBox.modify_bbox(current_bbox - 1)
      end
      
      gap_click = gap_click + ddLib.ftime()
      gap_tap = gap_tap + ddLib.ftime()

      -- select a box
      if ddInput.mouse_b_r and gap_click > 0.1 then
        gap_click = 0

        -- get ray 
        origin = assets.nav_agent:pos()
        dir = ddLib.raycast(assets.cam_01:id(), ddInput.mouse_x, ddInput.mouse_y)

        -- check all bboxs to see if intersection occurs
        time = math.huge
        selected = nil
        for i=1,#created_bboxs do
          -- check for intersections
          hit, time2 = created_bboxs[i]:intersect(
            {origin.x, origin.y, origin.z}, {dir.x, dir.y, dir.z})
          if hit and time2 < time then
            selected = i
            time = time2
          end
        end

        -- if selected turn on modification window
        if selected then
          bbox_select = true
          current_bbox = selected
        else
          bbox_select = false
          current_bbox = selected
        end
      end

      if gap_tap > 0.08 then
        gap_tap = 0
        
        -- toggle translation
        if ddInput.t then self.ctrl[_T] = not self.ctrl[_T] end
        -- toggle rotation
        if ddInput.r then self.ctrl[_R] = not self.ctrl[_R] end
        -- toggle scale
        if ddInput.e then self.ctrl[_S] = not self.ctrl[_S] end
      end

      -- modifying the box
      if not ddLib.mouse_over_UI() and bbox_select then
        --
      end
    end
  end

  return manager

end