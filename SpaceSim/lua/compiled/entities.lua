local entities = { }
entities.moveTo = function(x, y, z)
  return function(entity, dt)
    entity_setSpeed(entity, interceptor_speed)
    return vtransform_facingWorld(entity.transform, Vector(x, y, z, 1.0))
  end
end
entities.strafeTo = function(tX, tY, tZ, f_x, f_y, f_z)
  return function(entity, dt)
    return vtransform_getWorldPosition(entity.transform):foreach(function(p)
      local targetPos = Vector(tX, tY, tZ, 1.0)
      local speed = clamp(interceptor_min_speed, interceptor_speed, vvector_distance(targetPos, p))
      vphysic_setVelocity(entity.physic, vvector_scale(vvector_normalize(vvector_subtract(targetPos, p)), speed))
      return vtransform_facingWorld(entity.transform, Vector(f_x, f_y, f_z, 1.0))
    end)
  end
end
entities.strafeFrom = function(this, t_x, t_y, t_z)
  return vtransform_getWorldPosition(this.transform):map(function(p)
    return function(entity, dt)
      return vtransform_getWorldPosition(entity.transform):foreach(function(current_position)
        entity.speed = clamp(1.0, interceptor_speed, entity.speed + entity.acceleration * dt)
        local target = vvector_normalize(vvector_subtract(Vector(t_x, t_y, t_z, 1.0), current_position))
        local turn_rate = (math.pi / 2) * 1.5
        local dir = vquaternion_slerpAngle(vquaternion_fromTransform(entity.transform), vquaternion_look(target), turn_rate * dt)
        vphysic_setVelocity(entity.physic, vquaternion_rotation(dir, Vector(0.0, 0.0, entity.speed, 0.0)))
        return vtransform_setRotation(entity.transform, dir)
      end)
    end
  end):getOrElse(function(e, d) end)
end
entities.atPosition = function(e, x, y, z, threshold)
  return vtransform_getWorldPosition(e.transform):map(function(p)
    return vvector_distance(p, Vector(x, y, z, 1.0)) < threshold
  end):getOrElse(false)
end
entities.setSpeed = function(entity, speed)
  entity.speed = speed
  return vphysic_setVelocity(entity.physic, vtransformVector(entity.transform, Vector(0.0, 0.0, entity.speed, 0.0)))
end
entities.homing_missile_tick = function(target)
  return function(missile, dt)
    if missile.physic and vtransform_valid(missile.transform) then
      return vtransform_getWorldPosition(missile.transform):foreach(function(p)
        return target:filter(vtransform_valid):foreach(function(tt)
          return vtransform_getWorldPosition(tt):foreach(function(t)
            local current = vquaternion_fromTransform(missile.transform)
            local target_dir = vquaternion_look(vvector_normalize(vvector_subtract(t, p)))
            local dir = vquaternion_slerpAngle(current, target_dir, homing_missile_turn_rps * dt)
            vphysic_setVelocity(missile.physic, vquaternion_rotation(dir, Vector(0.0, 0.0, enemy_homing_missile.speed, 0.0)))
            return vtransform_setRotation(missile.transform, dir)
          end)
        end)
      end)
    end
  end
end
entities.missile_collisionHandler = function(missile, other)
  fx.spawn_missile_explosion(missile.transform)
  vphysic_setVelocity(missile.physic, Vector(0.0, 0.0, 0.0, 0.0))
  missile.body = safeCleanup(missile.body, function()
    return vdestroyBody(missile.body)
  end)
  missile.model = safeCleanup(missile.model, function()
    return vdeleteModelInstance(missile.model)
  end)
  missile.tick = nil
  return inTime(2.0, function()
    missile.trail = safeCleanup(missile.trail, function()
      return vdeleteModelInstance(missile.trail)
    end)
    return gameobject_delete(missile)
  end)
end
entities.findClosestEnemies = function(transform, count)
  local z_axis = Vector(0.0, 0.0, 1.0, 0.0)
  local targets = array.filter(interceptors, function(interceptor)
    if not vtransform_valid(interceptor.transform) then
      return false
    else
      local interceptor_position = vtransform_getWorldPosition(interceptor.transform)
      local current_position = vtransform_getWorldPosition(transform)
      return interceptor_position:map(function(iPos)
        return current_position:map(function(cPos)
          return vvector_dot(vvector_subtract(iPos, cPos), vtransformVector(transform, z_axis)) > 0
        end)
      end):getOrElse(false):getOrElse(false)
    end
  end)
  return list.fromArray(targets):take(count)
end
entities.findClosestEnemy = function(transform)
  return entities.findClosestEnemies(transform, 1):headOption()
end
local swarmLaunch
swarmLaunch = function(ship)
  return function(target)
    return inTime((target._2 - 1) * 0.1, function()
      return player_fire_missile(ship, option:some(target._1))
    end)
  end
end
entities.player_fire_missile_swarm = function(ship)
  return entities.findClosestEnemies(ship.transform, 4):zipWithIndex():foreach(swarmLaunch(ship))
end
return entities
