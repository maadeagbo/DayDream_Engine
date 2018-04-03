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

  modify_switch = {
    "pos",
    "rot",
    "scale",
    "x",
    "y",
    "z",
  }

  function manager:new( params )
		params = params or {}
		self.__index = self
		setmetatable(params, self)

		return params
  end

  --- Creates a table of strings for bbox output (uses table.concat for output)
  function gen_bbox_string( c1, c2, c3, c4, c5, c6, c7, c8, jnt_id )
    out_str = {}
    -- write begin tag
    out_str[#out_str + 1] = "<bbox>"

    -- joint id
    out_str[#out_str + 1] = "<joint>"
    out_str[#out_str + 1] = jnt_id
    out_str[#out_str + 1] = "</joint>"

    out_str[#out_str + 1] = "<verts>"
    -- vert 1
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c1.x, c1.y, c1.z)
    -- vert 2
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c2.x, c2.y, c2.z)
    -- vert 3
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c3.x, c3.y, c3.z)
    -- vert 4
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c4.x, c4.y, c4.z)
    -- vert 5
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c5.x, c5.y, c5.z)
    -- vert 6
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c6.x, c6.y, c6.z)
    -- vert 7
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c7.x, c7.y, c7.z)
    -- vert 8
    out_str[#out_str + 1] = string.format("v %.3f %.3f %.3f", c8.x, c8.y, c8.z)
    out_str[#out_str + 1] = "</verts>"

    -- write end tag
    out_str[#out_str + 1] = "</bbox>"

    -- return string
    return table.concat( out_str, "\n" )
  end

  function write_bbox_file( file_name )
    name = string.format("%s/BBoxSkinnedMesh/output/%s.ddx", PROJECT_DIR, 
                         file_name)
    ddLib.print("[status]Exporting <", name, ">.ddx ...")
    -- create file
    file = io.open(name, "w")
    -- collect total # of boxes
    num_boxes = 0
    for i=1,#created_bboxs do
      num_boxes = num_boxes + 1
      -- check for mirrors
      mirror = created_bboxs[i].mirror
      if mirror.x ~= 0 or mirror.y ~= 0 or mirror.z ~= 0 then
        num_boxes = num_boxes + 1
      end
    end
    ddLib.print("Num boxes out = ", num_boxes)
    -- for each box:
    for i=1,#created_bboxs do
      -- generate the 8 vertices that make up the box
      --c1, c2, c3, c4, c5, c6, c7, c8 = created_bboxs[i]:get_corners(false)

      -- get generated string

      -- repeat if mirrored box
      mirror = created_bboxs[i].mirror
      if mirror.x ~= 0 or mirror.y ~= 0 or mirror.z ~= 0 then
        --c1, c2, c3, c4, c5, c6, c7, c8 = created_bboxs[i]:get_corners(true)
      end
    end
      
  end
  
  function manager:modify_bbox( bbox_idx, trs_idx, xyz_idx, sign )
    --[[]]
    -- bbox
    bbox = created_bboxs[current_bbox]
    --ddLib.print("BBox #", bbox_idx, ": ", tostring(bbox))

    -- use switch table to get properties to modify
    trs = modify_switch[trs_idx]
    xyz = modify_switch[xyz_idx]
    
    -- value to set
    val = bbox[trs]
    edit_factor = (trs == "rot") and 1.0 or 0.01
    val[xyz] = val[xyz] + sign * edit_factor
    
    bbox[trs] = { val.x, val.y, val.z }
    --]]
  end

  function manager:update( event, args, num_args )
    if event == "new_bbox" then
      -- create new bounding box
      idx = #created_bboxs + 1
      created_bboxs[idx] = ddBBox.new()
      created_bboxs[idx].pos = {0.0, 0.0, 0.0}
      created_bboxs[idx].scale = {0.5, 0.5, 0.5}
      created_bboxs[idx].jnts = {-2, -2}

      --mirror = created_bboxs[idx].mirror
      --ddLib.print("Mirror: ", mirror.x, ", ", mirror.y, ", ", mirror.z)
    elseif event == "export_bbox" then
      if num_args == 1 then
        write_bbox_file(args["0"])
      end
    else
      -- regular update
      assets = BBoxSkinnedMesh_assets
      input = ddInput
      
      gap_click = gap_click + ddLib.ftime()
      gap_tap = gap_tap + ddLib.ftime()

      -- show modification window
      if bbox_select then
        ddBBox.modify_bbox(current_bbox - 1)
      end

      -- select a box
      if input.mouse_b_r and gap_click > 0.1 then
        gap_click = 0

        -- get ray 
        origin = assets.nav_agent:pos()
        dir = ddLib.raycast(assets.cam_01:id(), input.mouse_x, input.mouse_y)

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
        -- toggle off all
        if input.q then 
          gap_tap = 0
          self.ctrl[_T] = false
          self.ctrl[_R] = false
          self.ctrl[_S] = false
          self.ctrl[_X] = false
          self.ctrl[_Y] = false
          self.ctrl[_Z] = false
        end
        -- toggle translation
        if input.t then self.ctrl[_T] = true; gap_tap = 0 end
        -- toggle rotation
        if input.r then self.ctrl[_R] = true; gap_tap = 0 end
        -- toggle scale
        if input.e then self.ctrl[_S] = true; gap_tap = 0 end
        -- toggle X axis
        if input.x then self.ctrl[_X] = true; gap_tap = 0 end
        -- toggle Y axis
        if input.y then self.ctrl[_Y] = true; gap_tap = 0 end
        -- toggle Z axis
        if input.z then self.ctrl[_Z] = true; gap_tap = 0 end
      end
    end

    -- modifying the box
    --[[]]
    if not ddLib.mouse_over_UI() and bbox_select and input.mouse_b_m then
      -- get sign based on mouse movement (up & right vs down & left)
      sign = 0.0
      if input.mouse_x_delta > 0.0 then
        sign = -1.0
      elseif input.mouse_x_delta < 0.0 then
        sign = 1.0
      end

      -- get indices
      trs_index = -1
      xyz_index = -1
      
      -- trs
      if self.ctrl[_T] then
        trs_index = _T + 1
      elseif self.ctrl[_R] then
        trs_index = _R + 1
      elseif self.ctrl[_S] then
        trs_index = _S + 1
      end
      -- xyz
      if self.ctrl[_X] then
        xyz_index = _X + 1
      elseif self.ctrl[_Y] then
        xyz_index = _Y + 1
      elseif self.ctrl[_Z] then
        xyz_index = _Z + 1
      end

      if trs_index > -1 and xyz_index > -1 then
        self:modify_bbox(current_bbox, trs_index, xyz_index, sign)
      end
      
    end
    --]]
  end

  return manager

end