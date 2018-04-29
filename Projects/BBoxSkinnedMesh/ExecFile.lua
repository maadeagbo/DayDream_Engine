do
  -- Make assets object local
  assets = BBoxSkinnedMesh_assets
  
  -- load skeleton
  ddAnimation.load_skeleton("sk_1", 
    PROJECT_DIR.."/BBoxSkinnedMesh/arissa/arissa.ddb")
  
  -- load animation clip
  ddAnimation.load_clip("walk", 
    PROJECT_DIR.."/BBoxSkinnedMesh/arissa/Walking_0.dda")
  
  -- load mesh
  assets.arissa_mesh = ddModel.new(
    PROJECT_DIR.."/BBoxSkinnedMesh/arissa/arissa.ddg")
  
  -- load agent
  assets.arissa = ddAgent.new("arissa", 0.0, "box")
  assets.arissa:set_pos(0.0, 0.0, 0.0)

  -- add mesh to agent
  assets.arissa:add_mesh(assets.arissa_mesh:id(), 0.1, 100.0)
  
  -- add skeleton to agent
  assets.arissa:set_skeleton("sk_1")
  -- add clip to agent
  a_walk_clip = assets.arissa:add_animation("arissa_walk", "walk")
  a_walk_clip[ddEnums.ACTIVE] = true
  a_walk_clip[ddEnums.LOCAL_TIME] = 1.0

  -- add specialized bounding box to agent
  assets.arissa:add_oobb(PROJECT_DIR.."/BBoxSkinnedMesh/output/first.ddx")
  
  ddLib.print("Arissa: ", tostring(assets.arissa))
end
