local entities = { }
entities.moveTo = function(x, y, z)
  return function(entity, dt)
    entity_setSpeed(entity, interceptor_speed)
    return vtransform_facingWorld(entity.transform, Vector(x, y, z, 1.0))
  end
end
entities.strafeTo = function(t_x, t_y, t_z, f_x, f_y, f_z)
  return function(entity, dt)
    local target = Vector(t_x, t_y, t_z, 1.0)
    return vtransform_getWorldPosition(entity.transform):foreach(function(p)
      local speed = clamp(interceptor_min_speed, interceptor_speed, vvector_distance(target, p))
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
return entities
