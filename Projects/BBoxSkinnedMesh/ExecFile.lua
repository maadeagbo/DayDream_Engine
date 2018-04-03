do
  -- Make assets object local
  assets = BBoxSkinnedMesh_assets
    
  -- animated mesh
  assets.arissa_mesh = ddModel.new("C:\\Users\\Moses\\Documents\\DayDream_Engine\\Projects\\BBoxSkinnedMesh\\arissa.ddg")
  assets.arissa = ddAgent.new("arissa", 0.0, "box")
  --assets.arissa:set_scale(0.01, 0.01, 0.01)
  assets.arissa:set_pos(0.0, 0.0, 0.0)
  assets.arissa:add_mesh(assets.arissa_mesh:id(), 0.1, 100.0)
  ddLib.print("Arissa: ", tostring(assets.arissa))
end