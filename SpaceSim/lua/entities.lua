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
		vtransform_getWorldPosition( entity.transform ):foreach( function( p )
			local distance_remaining = vvector_distance( target_position, p )
			local speed = clamp( interceptor_min_speed, interceptor_speed, distance_remaining)

			local world_direction = vvector_normalize( vvector_subtract( target_position, p ))
			local world_velocity = vvector_scale( world_direction, speed )
			vphysic_setVelocity( entity.physic, world_velocity )

			-- Face correct direction
			local facing_position = Vector( facing_x, facing_y, facing_z, 1.0 )
			vtransform_facingWorld( entity.transform, facing_position )

			--vdebugdraw_cross( target_position, 10.0 )
		end )
	end
end

function entities.strafeFrom( this, target_x, target_y, target_z ) 
	return vtransform_getWorldPosition( this.transform ):map( function( p )
		local x,y,z,w = vvector_values( p )
		local original = { x = x, y = y, z = z, w = w }
		return function ( entity, dt )
			-- Move to correct position
			local target_position = Vector( target_x, target_y, target_z, 1.0 )
			vtransform_getWorldPosition( entity.transform ):foreach( function( current_position )
				entity.speed = clamp( 1.0, interceptor_speed, entity.speed + entity.acceleration * dt )
				-- Face correct direction
				local target_direction = vvector_normalize( vvector_subtract( target_position, current_position ))
				local current_dir = vquaternion_fromTransform( entity.transform )
				local target_dir = vquaternion_look( target_direction )
				local turn_angle_per_second = (math.pi / 2) * 1.5
				local new_dir = vquaternion_slerpAngle( current_dir, target_dir, turn_angle_per_second * dt )
				local world_velocity = vquaternion_rotation( new_dir, Vector( 0.0, 0.0, entity.speed, 0.0 ))
				vphysic_setVelocity( entity.physic, world_velocity )
				vtransform_setRotation( entity.transform, new_dir )
			end)
		end
	end):getOrElse( function( entity, dt ) end )
end

function entities.atPosition( entity, x, y, z, max_distance )
	local position = Vector( x, y, z, 1.0 )
	return vtransform_getWorldPosition( entity.transform ):map( function( p )
		local distance = vvector_distance( p, position )
		return distance < max_distance
	end ):getOrElse( false )
end

function entities.setSpeed( entity, speed )
	entity.speed = speed
	local entity_velocity = Vector( 0.0, 0.0, entity.speed, 0.0 )
	local world_velocity = vtransformVector( entity.transform, entity_velocity )
	vphysic_setVelocity( entity.physic, world_velocity )
end

return entities
