-- Player functionality

player = {}

player.ship_weaponsTick = ( ship, dt ) ->
	fired = false
	if touch_enabled
		fired, joypad_x, joypad_y = vtouchPadTouched( ship.fire_trigger )
	else
		fired = vkeyPressed( input, key.space )
	if fired then player.fire( ship )
	missile_fired = if touch_enabled
		missile_fired = vgesture_performed( player_ship.fire_trigger, player_ship.missile_swipe )
	else
		missile_fired = vkeyPressed( input, key.q )
	if missile_fired and ship.missile_cooldown <= 0.0
		if ship.aileron_roll
			player.fire_missile_swarm( ship )
		else
			player.fire_missile( ship, entities.findClosestEnemy( ship.transform ))
		ship.missile_cooldown = player_missile_cooldown
	ship.cooldown = ship.cooldown - dt
	ship.missile_cooldown = ship.missile_cooldown - dt

player.autofly = ( ship ) ->
	input_yaw = 0.0
	input_pitch = 0.0
	vtransform_getWorldPosition( ship.transform )\foreach( ( p ) ->
		current_u, current_v = vcanyon_fromWorld( canyon, p )
		target_v = current_v + 50
		x, y, z = vcanyon_position( canyon, 0, target_v )
		target_pos = Vector( x, y, z, 1.0 )
		m = vmatrix_facing( target_pos, p )
		target_yaw, target_pitch, target_roll = vmatrix_toEulerAngles( m )
		input_yaw = target_yaw - ship.yaw
		input_pitch = target_pitch - ship.pitch)
	return input_yaw, input_pitch

player.ship_tick = ( ship, dt ) ->
	if ship == nil or ship.transform == nil then return
	yaw_per_second = 1.5
	pitch_per_second = 1.5

	input_yaw, input_pitch = if debug_player_autofly then player.autofly( ship ) else ship.steering_input()

	-- set to -1.0 to invert
	invert_pitch = 1.0
	pitch = invert_pitch * input_pitch * pitch_per_second * dt
	yaw_delta = input_yaw * yaw_per_second * dt
	ship.pitch = ship.pitch + pitch

	strafe = 0.0

	if not ship.aileron_roll then
		aileron_roll_left = false
		aileron_roll_right = false
		if touch_enabled
			aileron_roll_left = vgesture_performed( player_ship.joypad, player_ship.aileron_swipe_left )
			aileron_roll_right = vgesture_performed( player_ship.joypad, player_ship.aileron_swipe_right )
		else
			aileron_roll_left = vkeyPressed( input, key.a )
			aileron_roll_right = vkeyPressed( input, key.d )
		if aileron_roll_left then player.ship_aileronRoll( ship, 1.0 )
		elseif aileron_roll_right then player.ship_aileronRoll( ship, -1.0 )

	ship.yaw_history[ship.yaw_history_index ] = ship.yaw
	ship.yaw_history_index = ( ship.yaw_history_index ) % 10 + 1

	camera_roll = 0.0
	if ship.aileron_roll
		-- strafe
		roll_rate = player.ship_strafeRate( ship.aileron_roll_time / aileron_roll_duration )
		strafe_speed = -1500.0 * roll_rate
		strafe = strafe_speed * dt * ship.aileron_roll_multiplier

		-- roll
		library.rolling_average.add( ship.target_roll, ship.aileron_roll_target )
		integral_total = 1.8 -- This is such a fudge - should be integral [0->1] of sin( pi^2 - x^2 / pi ) ( which is 1.58605 )
		roll_delta = roll_rate * dt * ship.aileron_roll_amount * integral_total
		ship.roll = ship.roll + roll_delta

		ship.aileron_roll_time = ship.aileron_roll_time - dt
		ship.aileron_roll = player.ship_aileronRollActive( ship )
		camera_roll = max_allowed_roll * camera_roll_scale * ship.aileron_roll_multiplier

		player.yaw( ship, dt )
	else
		ship.target_yaw = ship.target_yaw + yaw_delta
		target_yaw_delta = player.yaw( ship, dt )

		-- roll
		target_roll = player.ship_rollFromYawRate( ship, target_yaw_delta )
		library.rolling_average.add( ship.target_roll, target_roll )
		roll_delta = player.ship_rollDeltaFromTarget( library.rolling_average.sample( ship.target_roll ), ship.roll )
		ship.roll = ship.roll + roll_delta
		roll_offset = library.modf( ship.roll + pi, two_pi ) - pi
		camera_roll = roll_offset * camera_roll_scale
	
	vtransform_eulerAngles( ship.transform, ship.yaw, ship.pitch, ship.roll )
	-- Camera transform shares ship position and yaw, pitch; but not roll
	vtransform_setWorldSpaceByTransform( ship.camera_transform, ship.transform )
	camera_target_position = vtransformVector( ship.transform, Vector( 0.0, 0.0, 20.0, 1.0 ))
	vtransform_setWorldPosition( ship.camera_transform, camera_target_position )
	vtransform_eulerAngles( ship.camera_transform, ship.yaw, ship.pitch, camera_roll )

	-- throttle
	width = 100
	delta_speed = player_ship_acceleration * dt
	ship.speed = math.min( ship.speed + delta_speed, player_ship_max_speed )

	player.ship_weaponsTick( ship, dt )

	-- Physics
	forward_v = vtransformVector( ship.transform, Vector( 0.0, 0.0, ship.speed, 0.0 ))
	strafe_v = vtransformVector( ship.camera_transform, Vector( strafe, 0.0, 0.0, 0.0 ))
	world_v = vvector_add( forward_v, strafe_v )
	if ship.physic then vphysic_setVelocity( ship.physic, world_v )

player.fire_missile_swarm = ( ship ) -> entities.findClosestEnemies( ship.transform, 4 )\zipWithIndex()\foreach( swarmLaunch(ship) )

player.ship_aileronRoll = ( ship, multiplier ) ->
	aileron_roll_delta = two_pi * multiplier
	ship.aileron_roll = true
	ship.aileron_roll_time = aileron_roll_duration
	ship.aileron_roll_multiplier = multiplier
	ship.aileron_roll_target = library.roundf( ship.roll + aileron_roll_delta + pi, two_pi )
	ship.aileron_roll_amount = ship.aileron_roll_target - ship.roll
	-- preserve heading from when we enter the roll
	ship.target_yaw = ship.yaw
	index = (ship.yaw_history_index - 10) % 10 + 1

player.ship_aileronRollActive = ( ship ) ->
	roll_offset = library.modf( ship.roll + pi, two_pi ) - pi
	not ( ship.aileron_roll_time < 0.0 and math.abs( roll_offset ) < 0.4 )

player.ship_rollFromYawRate = ( ship, yaw_delta ) ->
	last_roll = library.rolling_average.sample( ship.target_roll ) or 0.0
	offset = math.floor(( last_roll + pi ) / two_pi ) * two_pi
	yaw_to_roll = -45.0
	clamp( -max_allowed_roll, max_allowed_roll, yaw_delta * yaw_to_roll ) + offset

player.ship_rollDeltaFromTarget = ( target, current ) ->
	target_delta = target - current
	max_roll_delta = math.min( 4.0 * dt, math.abs( target_delta / 2.0 ))
	clamp( -max_roll_delta, max_roll_delta, target_delta )

-- Distorted sin curve for quicker attack and longer decay
-- sin ( (pi-x)^2 / pi )
player.ship_strafeRate = ( ratio ) -> clamp( 0.0, 1.0, math.sin( pi + ratio*ratio / pi - 2.0 * ratio ))

player.yaw = ( ship, dt ) ->
	target_yaw_delta = ship.target_yaw - ship.yaw
	max_yaw_delta = 2.0 * math.abs( ship.roll ) * 1.3 * dt
	actual_yaw_delta = clamp( -max_yaw_delta, max_yaw_delta, target_yaw_delta )
	ship.yaw = ship.yaw + actual_yaw_delta
	target_yaw_delta

player.ship_cleanup = ( p ) -> if p and vtransform_valid( p.camera_transform ) then vdestroyTransform( scene, p.camera_transform )

-- Create a player. The player is a specialised form of Gameobject
player.ship_create = () ->
	p = game.create( player_ship_model )
	p.speed = 0.0
	p.cooldown = 0.0
	p.missile_cooldown = 0.0
	p.yaw = 0
	p.target_yaw = 0
	p.pitch = 0
	p.roll = 0
	p.aileron_roll = false
	p.aileron_roll_time = 5.0
	p.yaw_history = {}
	p.yaw_history_index = 1
	p.camera_transform = vcreateTransform()
	
	-- Init Collision
	vbody_registerCollisionCallback( p.body, player.ship_collisionHandler )
	vbody_setLayers( p.body, collision_layer_player )
	vbody_setCollidableLayers( p.body, bitwiseOR( collision_layer_enemy, collision_layer_terrain ))
	p

player.ship_collisionHandler = ( ship, collider ) ->
	if not debug_player_immortal
		-- stop the ship
		ship.speed = 0.0
		no_velocity = Vector( 0.0, 0.0, 0.0, 0.0 )
		vphysic_setVelocity( ship.physic, no_velocity )

		fx.spawn_explosion( ship.transform )

		-- not using gameobject_destroy as we need to sync transform dying with camera rejig
		inTime( 0.2, () ->
			vdeleteModelInstance( ship.model )
			vphysic_destroy( ship.physic )
			ship.physic = nil)
		vdestroyBody( ship.body )

		-- queue a restart
		inTime( 2.0, () ->
			alpha = 0.3
			restartFrame = ui.show_splash_withColor( "dat/img/black.tga", screen_width, screen_height, Vector( 1.0, 1.0, 1.0, alpha ))
			onKeyPress( input, key.space )\onComplete( () ->
				ui.hide_splash( restartFrame )
				vprint( "Restarting" )
				vdestroyTransform( scene, ship.transform )
				restart()
				gameplay_start()))

player.fire = ( ship ) ->
	if ship.cooldown <= 0.0 then
		muzzle_position	= Vector( 1.2, 1.0, 1.0, 1.0 )
		entities.fire_missile( ship, muzzle_position, player_gunfire )
		fx.muzzle_flare( ship, muzzle_position )
		muzzle_position			= Vector( -1.2, 1.0, 1.0, 1.0 )
		entities.fire_missile( ship, muzzle_position, player_gunfire )
		fx.muzzle_flare( ship, muzzle_position )
		ship.cooldown = player_gun_cooldown

player.fire_missile = ( ship, target ) ->
	missile = entities.fire_missile( ship, Vector( 0.0, 0.0, 1.0, 1.0 ), player_missile )
	missile.tick = entities.homing_missile_tick( target\map((t) -> return t.transform ))

player
