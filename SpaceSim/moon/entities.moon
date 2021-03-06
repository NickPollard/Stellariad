-- Moonscript
entities = {}

entities.moveTo = ( x, y, z ) ->
	( entity, dt ) ->
		entity_setSpeed( entity, interceptor_speed )
		vtransform_facingWorld( entity.transform, Vector( x, y, z, 1.0 ))

entities.strafeTo = ( tX, tY, tZ, f_x, f_y, f_z ) ->
	( entity, dt ) ->
		vtransform_getWorldPosition( entity.transform )\foreach( (p) ->
			targetPos = Vector( tX, tY, tZ, 1.0 )
			speed = clamp( interceptor_min_speed, interceptor_speed, vvector_distance( targetPos, p ))
			vphysic_setVelocity( entity.physic, vvector_scale( vvector_normalize( vvector_subtract( targetPos, p )), speed ))
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

entities.homing_missile_tick = ( target ) ->
	( missile, dt ) ->
		if missile.physic and vtransform_valid( missile.transform )
			vtransform_getWorldPosition( missile.transform )\foreach(( p ) ->
				target\filter(vtransform_valid)\foreach((tt) ->
					vtransform_getWorldPosition( tt )\foreach( ( t ) ->
						current = vquaternion_fromTransform( missile.transform )
						target_dir = vquaternion_look( vvector_normalize( vvector_subtract( t, p )))
						dir = vquaternion_slerpAngle( current, target_dir, homing_missile_turn_rps * dt )
						vphysic_setVelocity( missile.physic, vquaternion_rotation( dir, Vector( 0.0, 0.0, enemy_homing_missile.speed, 0.0 )))
						vtransform_setRotation( missile.transform, dir ))))

entities.missile_collisionHandler = ( missile, other ) ->
	fx.spawn_missile_explosion( missile.transform )
	vphysic_setVelocity( missile.physic, Vector( 0.0, 0.0, 0.0, 0.0 ))
	missile.body = safeCleanup( missile.body, () -> vdestroyBody( missile.body ))
	missile.model = safeCleanup( missile.model, () -> vdeleteModelInstance( missile.model ))
	missile.tick = nil
	inTime( 2.0, () ->
		missile.trail = safeCleanup( missile.trail, () -> vdeleteModelInstance( missile.trail ))
		game.delete( missile ))

entities.findClosestEnemies = ( transform, count ) ->
	z_axis = Vector( 0.0, 0.0, 1.0, 0.0 )
	targets = array.filter( interceptors, ( interceptor ) ->
		if not vtransform_valid( interceptor.transform )
			false
		else
			interceptor_position = vtransform_getWorldPosition( interceptor.transform )
			current_position = vtransform_getWorldPosition( transform )
			interceptor_position\map(( iPos ) ->
				current_position\map(( cPos ) ->
					vvector_dot( vvector_subtract( iPos, cPos ), vtransformVector( transform, z_axis )) > 0))\getOrElse(false)\getOrElse(false))
	list.fromArray(targets)\take(count)

entities.findClosestEnemy = ( transform ) -> entities.findClosestEnemies( transform, 1 )\headOption()

swarmLaunch = (ship) ->
	(target) -> inTime( (target._2 - 1) * 0.1, () -> player_fire_missile( ship, option\some(target._1 )))

--entities.player_fire_missile_swarm = ( ship ) -> entities.findClosestEnemies( ship.transform, 4 )\zipWithIndex()\foreach( swarmLaunch(ship) )

entities.fire_missile = ( source, offset, bullet_type ) ->
	projectile = entities.create_projectile( source, offset, bullet_type.model, bullet_type.speed )

	if bullet_type.trail then
		projectile.trail = vcreateModelInstance( bullet_type.trail )
		vmodel_setTransform( projectile.trail, projectile.transform )
		vscene_addModel( scene, projectile.trail )

	if bullet_type.collisionType == "player" then setCollision_playerBullet( projectile )
	elseif bullet_type.collisionType == "enemy" then setCollision_enemyBullet( projectile )
	vbody_registerCollisionCallback( projectile.body, entities.missile_collisionHandler )

	inTime( bullet_type.time_to_live, () ->
		game.destroy( projectile )
		projectile.trail = safeCleanup( projectile.trail, () -> vdeleteModelInstance( projectile.trail ) ))
	projectile

entities.fire_enemy_homing_missile = ( source, offset, bullet_type ) ->
	projectile = entities.fire_missile( source, offset, bullet_type )
	projectile.tick = entities.homing_missile_tick( option\some(player_ship.transform) )

entities.interceptor_fire_homing = ( interceptor ) -> entities.fire_enemy_homing_missile( interceptor, Vector( 0.0, 0.0, 0.0, 1.0 ), enemy_homing_missile )

entities.turret_collisionHandler = ( target, collider ) ->
	fx.spawn_explosion( target.transform )
	game.destroy( target )
	target.behaviour = ai.dead

entities.interceptor_fire = ( interceptor ) ->
	muzzle_position = Vector( 3.2, 0.0, 1.0, 1.0 )
	fx.muzzle_flare_large( interceptor, muzzle_position )
	entities.fire_missile( interceptor, muzzle_position, enemy_gunfire )
	muzzle_position	= Vector( -3.2, 0.0, 1.0, 1.0 )
	fx.muzzle_flare_large( interceptor, muzzle_position )
	entities.fire_missile( interceptor, muzzle_position, enemy_gunfire )

entities.turret_fire = ( turret ) ->
	muzzle_position = Vector( 4.6, 5.0, 5.0, 1.0 )
	fx.muzzle_flare_large( turret, muzzle_position )
	entities.fire_missile( turret, muzzle_position, enemy_gunfire )
	muzzle_position = Vector( -4.6, 5.0, 5.0, 1.0 )
	fx.muzzle_flare_large( turret, muzzle_position )
	entities.fire_missile( turret, muzzle_position, enemy_gunfire )

entities.turret_tick = ( turret, dt ) -> turret.behaviour = turret.behaviour( turret, dt )

entities.create_projectile = ( source, offset, model, speed ) ->
	-- Position it at the correct muzzle position and rotation
	muzzle_world_pos = vtransformVector( source.transform, offset )
	muzzle_matrix = vtransformWorldMatrix( source.transform )
	vmatrix_setTranslation( muzzle_matrix, muzzle_world_pos )

	-- Create a new Projectile
	projectile = game.createAt( model, muzzle_matrix )
	projectile.tick = nil

	-- Apply initial velocity
	source_velocity = Vector( 0.0, 0.0, speed, 0.0 )
	world_v = vtransformVector( source.transform, source_velocity )
	vphysic_setVelocity( projectile.physic, world_v )

	-- Store the projectile so it doesn't get garbage collected
	array.add( missiles, projectile )
	projectile

entities.ship_collisionHandler = ( ship, collider ) ->
	fx.spawn_explosion( ship.transform )
	entities.ship_destroy( ship )
	ship.behaviour = ai.dead

entities.ship_destroy = ( ship ) ->
	array.remove( interceptors, ship )
	game.destroy( ship )

entities.ship_delete = ( ship ) ->
	array.remove( interceptors, ship )
	game.delete( ship )
	ship.behaviour = ai.dead

entities
