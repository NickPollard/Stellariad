-- Moonscript
entities = {}

entities.moveTo = ( x, y, z ) ->
	( entity, dt ) ->
		entity_setSpeed( entity, interceptor_speed )
		vtransform_facingWorld( entity.transform, Vector( x, y, z, 1.0 ))

entities.strafeTo = ( t_x, t_y, t_z, f_x, f_y, f_z ) ->
	( entity, dt ) ->
		vtransform_getWorldPosition( entity.transform )\foreach( (p) ->
			speed = clamp( interceptor_min_speed, interceptor_speed, vvector_distance( Vector( t_x, t_y, t_z, 1.0), p ))
			vtransform_facingWorld( entity.transform, Vector( f_x, f_y, f_z, 1.0 )))

entities.strafeFrom = ( this, t_x, t_y, t_z ) ->
	vtransform_getWorldPosition( this.transform )\map( (p) ->
		( entity, dt ) ->
			vtransform_getWorldPosition( entity.transform )\foreach( ( current_position ) ->
				entity.speed = clamp( 1.0, interceptor_speed, entity.speed + entity.acceleration * dt )
				target = vvector_normalize( vvector_subtract( Vector( t_x, t_y, t_z, 1.0), current_position ))
				turn_rate = (math.pi / 2) * 1.5
				dir = vquaternion_slerpAngle( vquaternion_fromTransform( entity.transform ), vquaternion_look( target ), turn_rate * dt )
				vphysic_setVelocity( entity.physic, vquaternion_rotation( dir, Vector( 0.0, 0.0, entity.speed, 0.0 )))
				vtransform_setRotation( entity.transform, dir )))\getOrElse( ( e, d ) -> )

entities.atPosition = ( e, x, y, z, threshold ) ->
	vtransform_getWorldPosition( e.transform )\map( ( p ) -> vvector_distance( p, Vector( x, y, z, 1.0 )) < threshold)\getOrElse false

entities.setSpeed = ( entity, speed ) ->
	entity.speed = speed
	vphysic_setVelocity( entity.physic, vtransformVector( entity.transform, Vector( 0.0, 0.0, entity.speed, 0.0 )))

entities
