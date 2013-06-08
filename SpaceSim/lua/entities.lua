local entities = {}

function entities.moveTo( x, y, z )
	return function ( entity, dt )
		entity_setSpeed( entity, interceptor_speed )
		local facing_position = Vector( x, y, z, 1.0 )
		vdebugdraw_cross( facing_position, 10.0 )
		vtransform_facingWorld( entity.transform, facing_position )
	end
end

function entities.strafeTo( target_x, target_y, target_z, facing_x, facing_y, facing_z )
	return function ( entity, dt )
		-- Move to correct position
		local target_position = Vector( target_x, target_y, target_z, 1.0 )
		local current_position = vtransform_getWorldPosition( entity.transform )
		local distance_remaining = vvector_distance( target_position, current_position )
		local speed = clamp( interceptor_min_speed, interceptor_speed, distance_remaining)

		local world_direction = vvector_normalize( vvector_subtract( target_position, current_position ))
		local world_velocity = vvector_scale( world_direction, speed )
		vphysic_setVelocity( entity.physic, world_velocity )

		-- Face correct direction
		local facing_position = Vector( facing_x, facing_y, facing_z, 1.0 )
		vtransform_facingWorld( entity.transform, facing_position )
		
		--vdebugdraw_cross( target_position, 10.0 )
	end
end

function entities.strafeFrom( this, target_x, target_y, target_z, facing_x, facing_y, facing_z )
	local initial_position = vtransform_getWorldPosition( this.transform )
	local x,y,z,w = vvector_values( initial_position )
	local original = { x = x, y = y, z = z, w = w }
	return function ( entity, dt )
		-- Move to correct position
		local target_position = Vector( target_x, target_y, target_z, 1.0 )
		local current_position = vtransform_getWorldPosition( entity.transform )
		local original_position = Vector( original.x, original.y, original.z, 1.0 )
		local distance_passed = vvector_distance( original_position, current_position )
		local speed = clamp( 1.0, interceptor_speed, distance_passed * 0.2 )

		local world_direction = vvector_normalize( vvector_subtract( target_position, current_position ))
		local world_velocity = vvector_scale( world_direction, speed )
		--vphysic_setVelocity( entity.physic, world_velocity )


		-- Face correct direction
			local current_position = vtransform_getWorldPosition( entity.transform )
			local target_direction = vvector_normalize( vvector_subtract( target_position, current_position ))
			local current_dir = vquaternion_fromTransform( entity.transform )
			local target_dir = vquaternion_look( target_direction )
			local turn_angle_per_second = (math.pi / 2) * 1.5
			local new_dir = vquaternion_slerpAngle( current_dir, target_dir, turn_angle_per_second * dt )
			local world_velocity = vquaternion_rotation( new_dir, Vector( 0.0, 0.0, speed, 0.0 ))
			vphysic_setVelocity( entity.physic, world_velocity )
			vtransform_setRotation( entity.transform, new_dir )

		--local facing_position = Vector( facing_x, facing_y, facing_z, 1.0 )
		--vtransform_facingWorld( entity.transform, facing_position )
	end
end

function entities.atPosition( entity, x, y, z, max_distance )
	local position = Vector( x, y, z, 1.0 )
	local entity_position = vtransform_getWorldPosition( entity.transform )
	local distance = vvector_distance( entity_position, position )
	return distance < max_distance
end

function entities.setSpeed( entity, speed )
	entity.speed = speed
	local entity_velocity = Vector( 0.0, 0.0, entity.speed, 0.0 )
	local world_velocity = vtransformVector( entity.transform, entity_velocity )
	vphysic_setVelocity( entity.physic, world_velocity )
end

return entities
