local list = require("list")
local all_doodads = list:empty()
local trans_worldPos
trans_worldPos = function(t)
  return vtransform_getWorldPosition(t)
end
local update_doodad_despawns
update_doodad_despawns = function(t)
  return trans_worldPos(t):foreach(function(p)
    local despawn_upto = (vcanyonV_atWorld(p)) - despawn_distance
    local toRemove = all_doodads:filter(shouldDespawn)
    toRemove:foreach(function(d)
      return gameobject_delete(d)
    end)
    all_doodads = all_doodads:filter(function()
      return not shouldDespawn
    end)
  end)
end
local doodad_spawnIndex
doodad_spawnIndex = function(pos)
  return math.floor((pos - spawn_offset) / doodads.interval)
end
local spawnDoodad
spawnDoodad = function(u, v, model)
  local x, y, z = vcanyon_position(u, v)
  local position = Vector(x, y, z, 1.0)
  local doodad = gameobject_create(model)
  vtransform_setWorldPosition(doodad.transform, position)
  all_doodads = list.cons(doodad, all_doodads)
end
local spawnBunker
spawnBunker = function(u, v, model)
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
  local doodad = gameobject_create(model)
  vtransform_setWorldPosition(doodad.transform, position)
  return doodad
end
local doodads_spawnTree
doodads_spawnTree = function(u, v)
  return spawnDoodad(u, v, "dat/model/tree_fir.s")
end
local doodads_spawnSkyscraper
doodads_spawnSkyscraper = function(u, v)
  local r = vrand(doodads.random, 0.0, 1.0)
  local _exp_0 = r
  if (r < 0.2) == _exp_0 then
    return spawnDoodad(u, v, "dat/model/skyscraper_blocks.s")
  elseif (r < 0.4) == _exp_0 then
    return spawnDoodad(u, v, "dat/model/skyscraper_slant.s")
  elseif (r < 0.6) == _exp_0 then
    return spawnDoodad(u, v, "dat/model/skyscraper_towers.s")
  end
end
local doodads_spawnRange
doodads_spawnRange = function(near, far)
  local nxt = doodad_spawnIndex(near) + 1
  local v = nxt * doodads.interval
  local u_offset = 130.0
  while library.contains(v, near, far) do
    doodads_spawnTree(u_offset, v)
    doodads_spawnTree(u_offset + 30.0, v)
    doodads_spawnTree(u_offset + 60.0, v)
    doodads_spawnTree(-u_offset, v)
    doodads_spawnTree(-u_offset - 30.0, v)
    doodads_spawnTree(-u_offset - 60.0, v)
    nxt = nxt + 1
    v = nxt * doodads.interval
  end
end
local doodads_spawned = 0.0
local update_doodads
update_doodads = function(transform)
  return trans_worldPos(transform):foreach(function(p)
    local spawn_upto = (vcanyonV_atWorld(p)) + doodads_spawn_distance
    doodads_spawnRange(doodads_spawned, spawn_upto)
    doodads_spawned = spawn_upto
  end)
end
