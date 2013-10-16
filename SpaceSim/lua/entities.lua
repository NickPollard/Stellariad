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
		local target_position = Vector( target_x, target_y, target_z, 1.0 )
		vtransform_getWorldPosition( entity.transform ):foreach( function( p )
			local speed = clamp( interceptor_min_speed, interceptor_speed, vvector_distance( target_position, p ))
			vphysic_setVelocity( entity.physic, vvector_scale( vvector_normalize( vvector_subtract( target_position, p )), speed ))
			vtransform_facingWorld( entity.transform, Vector( facing_x, facing_y, facing_z, 1.0 ))
		end )
	end
end

function entities.strafeFrom( this, t_x, t_y, t_z ) 
	return vtransform_getWorldPosition( this.transform ):map( function( p )
		return function ( entity, dt )
			-- Move to correct position
			vtransform_getWorldPosition( entity.transform ):foreach( function( current_position )
				entity.speed = clamp( 1.0, interceptor_speed, entity.speed + entity.acceleration * dt )
				-- Face correct direction
				local target = vvector_normalize( vvector_subtract( Vector( t_x, t_y, t_z, 1.0), current_position ))
				local turn_rate = (math.pi / 2) * 1.5
				local dir = vquaternion_slerpAngle( vquaternion_fromTransform( entity.transform ), vquaternion_look( target ), turn_rate * dt )
				vphysic_setVelocity( entity.physic, vquaternion_rotation( dir, Vector( 0.0, 0.0, entity.speed, 0.0 )))
				vtransform_setRotation( entity.transform, dir )
			end)
		end
	end):getOrElse( function( entity, dt ) end )
end

function entities.atPosition( e, x, y, z, threshold )
	return vtransform_getWorldPosition( e.transform ):map( function( p )
		return vvector_distance( p, Vector( x, y, z, 1.0 )) < threshold
	end ):getOrElse( false )
end

function entities.setSpeed( entity, speed )
	entity.speed = speed
	vphysic_setVelocity( entity.physic, vtransformVector( entity.transform, Vector( 0.0, 0.0, entity.speed, 0.0 )))
end

return entities
