local list = require("list")
local option = require("option")
local doodads = { }
doodads.spawn_distance = 900.0
doodads.random = vrand_newSeq()
doodads.interval = 30.0
local all_doodads = list:empty()
local trans_worldPos
trans_worldPos = function(t)
  return vtransform_getWorldPosition(t)
end
local shouldDespawn
shouldDespawn = function(upto)
  return function(doodad)
    return vtransform_getWorldPosition(doodad.transform):map(function(p)
      local unused, v = vcanyon_fromWorld(p)
      return v < upto
    end):getOrElse(true)
  end
end
doodads.updateDespawns = function(t)
  return trans_worldPos(t):foreach(function(p)
    local upto = (vcanyonV_atWorld(p)) - despawn_distance
    all_doodads = all_doodads:filter(function(d)
      if shouldDespawn(upto)(d) then
        doodads.delete(d)
        return false
      else
        return true
      end
    end)
  end)
end
doodads.spawnIndex = function(pos)
  return math.floor((pos - spawn_offset) / doodads.interval)
end
doodads.create = function(model_file)
  local g = {
    model = vcreateModelInstance(model_file),
    transform = vcreateTransform()
  }
  vmodel_setTransform(g.model, g.transform)
  vscene_addModel(scene, g.model)
  return g
end
doodads.delete = function(g)
  if g.model then
    vdeleteModelInstance(g.model)
    g.model = nil
  end
  if vtransform_valid(g.transform) then
    vdestroyTransform(scene, g.transform)
    g.transform = nil
  end
end
doodads.spawnDoodad = function(u, v, model)
  local x, y, z = vcanyon_position(u, v)
  local doodad = doodads.create(model)
  vtransform_setWorldPosition(doodad.transform, Vector(x, y, z, 1.0))
  all_doodads = list:cons(doodad, all_doodads)
end
doodads.spawnBunker = function(u, v, model)
  local highest = {
    x = 0,
    y = -10000,
    z = 0
  }
  local radius = 100.0
  local step = radius / 5.0
  local i = v - radius
  while i < v + radius do
    local x, y, z = vcanyon_position(u, i)
    if y > hightext.y then
      highest.x = x
      highest.y = y
      highest.z = z
    end
    i = i + step
  end
  local x, y, z = vcanyon_position(u, v)
  local position = Vector(x, y, z, 1.0)
  local doodad = doodads.create(model)
  vtransform_setWorldPosition(doodad.transform, position)
  return doodad
end
doodads.spawnTree = function(u, v)
  return doodads.spawnDoodad(u, v, "dat/model/tree_fir.s")
end
doodads.spawnSkyscraper = function(u, v)
  local r = vrand(doodads.random, 0.0, 1.0)
  local doodad
  local _exp_0 = r
  if (r < 0.2) == _exp_0 then
    doodad = option:some("dat/model/skyscraper_blocks.s")
  elseif (r < 0.4) == _exp_0 then
    doodad = option:some("dat/model/skyscraper_slant.s")
  elseif (r < 0.6) == _exp_0 then
    doodad = option:some("dat/model/skyscraper_towers.s")
  else
    doodad = option:none()
  end
  return doodad:foreach(function(d)
    return doodads.spawnDoodad(u, v, d)
  end)
end
doodads.spawnRange = function(near, far)
  local nxt = doodads.spawnIndex(near) + 1
  local v = nxt * doodads.interval
  local u_offset = 130.0
  while library.contains(v, near, far) do
    doodads.spawnTree(u_offset, v)
    doodads.spawnTree(u_offset + 30.0, v)
    doodads.spawnTree(u_offset + 60.0, v)
    doodads.spawnTree(-u_offset, v)
    doodads.spawnTree(-u_offset - 30.0, v)
    doodads.spawnTree(-u_offset - 60.0, v)
    nxt = nxt + 1
    v = nxt * doodads.interval
  end
end
doodads.spawned = 0.0
doodads.update = function(transform)
  return trans_worldPos(transform):foreach(function(p)
    local spawn_upto = (vcanyonV_atWorld(p)) + doodads.spawn_distance
    doodads.spawnRange(doodads.spawned, spawn_upto)
    doodads.spawned = spawn_upto
  end)
end
return doodads
