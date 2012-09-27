-- SpaceSim main game lua script

--[[

This file contains the main game routines for the SpaceSim game.
Most game logic is written in Lua, whilst the core numerical processing (rendering, physics, animation etc.)
are handled by the Vitae engine in C.

Lua should be able to do everything C can, but where performance is necessary, code should be rewritten in
C and only controlled remotely by Lua

]]--

-- Load Modules
	package.path = "./SpaceSim/lua/?.lua"
	tm = require "testmodule"
	ai = require "ai"

-- player - this object contains general data about the player
player = nil
-- player_ship - the actual ship entity flying around
player_ship = nil

-- Create a spacesim Game object
-- A gameobject has a visual representation (model), a physical entity for velocity and momentum (physic)
-- and a transform for locating it in space (transform)
function gameobject_create( model_file )
	g = {}
	g.model = vcreateModelInstance( model_file )
	g.physic = vcreatePhysic()
	g.transform = vcreateTransform()
	g.body = vcreateBodySphere( g )
	--g.body = vcreateBodyMesh( g, g.model )
	vmodel_setTransform( g.model, g.transform )
	vphysic_setTransform( g.physic, g.transform )
	vbody_setTransform( g.body, g.transform )
	vscene_addModel( scene, g.model )
	vphysic_activate( engine, g.physic )
	v = Vector( 0.0, 0.0, 0.0, 0.0 )
	vphysic_setVelocity( g.physic, v )

	return g
end

function gameobject_destroy( g )
	inTime( 0.2, function () vdeleteModelInstance( g.model ) 
							vdestroyTransform( scene, g.transform )
							vphysic_destroy( g.physic )
				end )
	vdestroyBody( g.body )
end

function spawn_missile_explosion( position )
	-- Attach a particle effect to the object
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/script/lisp/missile_explosion.s" )
	vtransform_setWorldSpaceByTransform( t, position )
end

function spawn_explosion( position )
	-- Attach a particle effect to the object
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/script/lisp/explosion.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_b.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_c.s" )
	-- Position it at the correct muzzle position and rotation
	vtransform_setWorldSpaceByTransform( t, position )
end

projectile_model = "dat/model/missile.s"
weapons_cooldown = 0.15

function player_fire( ship )
	if ship.cooldown <= 0.0 then
		muzzle_position = Vector( 1.2, 0.0, 0.0, 1.0 );
		fire_missile( ship, muzzle_position );
		muzzle_position = Vector( -1.2, 0.0, 0.0, 1.0 );
		fire_missile( ship, muzzle_position );
		ship.cooldown = weapons_cooldown
	end
end

function missile_destroy( missile )
	if not missile.destroyed then
		vdeleteModelInstance( missile.model ) 
		vphysic_destroy( missile.physic )
		vdestroyTransform( scene, missile.transform )
		vdestroyBody( missile.body )
		vparticle_destroy( missile.glow )
		missile.destroyed = true
	end
end

function missile_collisionHandler( missile, other )
	spawn_missile_explosion( missile.transform )
	vphysic_setVelocity( missile.physic, Vector( 0.0, 0.0, 0.0, 0.0 ))
	missile_destroy( missile )
end

missiles = { count = 0 }
bullet_speed = 250.0;
enemy_bullet_speed = 150.0;

function fire_missile( source, offset )
	-- Create a new Projectile
	local projectile = {}
	projectile = gameobject_create( projectile_model );
	vbody_setLayers( projectile.body, collision_layer_player )
	vbody_setCollidableLayers( projectile.body, collision_layer_enemy )
	vbody_registerCollisionCallback( projectile.body, missile_collisionHandler )

	-- Position it at the correct muzzle position and rotation
	muzzle_world_pos = vtransformVector( source.transform, offset )
	vtransform_setWorldSpaceByTransform( projectile.transform, source.transform )
	vtransform_setWorldPosition( projectile.transform, muzzle_world_pos )

	-- Attach a particle effect to the object
	projectile.glow = vparticle_create( engine, projectile.transform, "dat/script/lisp/bullet.s" )
	--inTime( 0.2, function () projectile.trail = vparticle_create( engine, projectile.transform, "dat/script/lisp/missile_particle.s" ) end )

	-- Apply initial velocity
	source_velocity = Vector( 0.0, 0.0, bullet_speed, 0.0 )
	world_v = vtransformVector( source.transform, source_velocity )
	vphysic_setVelocity( projectile.physic, world_v );

	projectile.destroyed = false

	-- Store the projectile so it doesn't get garbage collected
	missiles[missiles.count] = projectile
	missiles.count = missiles.count + 1
end

function fire_enemy_missile( source, offset )
	-- Create a new Projectile
	local projectile = {}
	projectile = gameobject_create( projectile_model );
	vbody_setLayers( projectile.body, collision_layer_enemy )
	vbody_setCollidableLayers( projectile.body, collision_layer_player )
	vbody_registerCollisionCallback( projectile.body, missile_collisionHandler )

	-- Position it at the correct muzzle position and rotation
	muzzle_world_pos = vtransformVector( source.transform, offset )
	vtransform_setWorldSpaceByTransform( projectile.transform, source.transform )
	vtransform_setWorldPosition( projectile.transform, muzzle_world_pos )

	-- Attach a particle effect to the object
	projectile.glow = vparticle_create( engine, projectile.transform, "dat/script/lisp/bullet.s" )
	--inTime( 0.2, function () projectile.trail = vparticle_create( engine, projectile.transform, "dat/script/lisp/missile_particle.s" ) end )

	-- Apply initial velocity
	source_velocity = Vector( 0.0, 0.0, enemy_bullet_speed, 0.0 )
	world_v = vtransformVector( source.transform, source_velocity )
	vphysic_setVelocity( projectile.physic, world_v );

	-- Queue up delete
	projectile.destroyed = false
	inTime( 2.0, function () missile_destroy( projectile ) end )

	-- Store the projectile so it doesn't get garbage collected
	missiles[missiles.count] = projectile
	missiles.count = missiles.count + 1
end

timers = {}
timers.count = 0

function addTimer( timer )
	timers.count = timers.count + 1
	timers[timers.count] = timer
end

---[[
function timer_create( time, action )
	timer = {
		time = time,
		action = action,
	}
	return timer
end
--]]

function inTime( time, action )
	addTimer( timer_create( time, action ))
end

function iterator( t )
	local i = 0
	local n = t.count
	return function ()
		i = i + 1
		if i <= n then return t[i] end
	end
end

function filter( array, func )
	new_array = {}
	local count = 0
	for element in iterator( array ) do
		if func( element ) then
			count = count + 1
			new_array[count] = element
		end
	end
	new_array.count = count
	return new_array
end

function timers_tick( dt )
	for element in iterator( timers ) do
		element.time = element.time - dt
		if element.time < 0 then
			element.action()
		end
	end

	new_timers = filter( timers, function( e ) return ( e.time > 0 ) end )
	timers = new_timers;
end

-- Create a player. The player is a specialised form of Gameobject
function playership_create()
	local p = gameobject_create( "dat/model/ship_hd.s" )
	p.speed = 0.0
	p.cooldown = 0.0
	p.yaw = 0
	p.pitch = 0
	p.roll = 0
	p.camera_transform = vcreateTransform()
	
	-- Init Collision
	vbody_registerCollisionCallback( p.body, player_ship_collisionHandler )
	vbody_setLayers( p.body, collision_layer_player )
	vbody_setCollidableLayers( p.body, collision_layer_enemy )

	return p
end

starting = true

-- Set up the Lua State
function init()
	vprint( "init" )
	starting = true
	library, msg = loadfile( "SpaceSim/lua/library.lua" )
	library()

--	vprint( tostring( msg ))
end

camera = "chase"
flycam = nil
chasecam = nil

function ship_tick( ship )
	vprint( "ship_tick" )
	ship_v = Vector( 0.0, 0.0, ship.speed, 0.0 )
	world_v = vtransformVector( ship.transform, ship_v )
	vphysic_setVelocity( ship.physic, world_v )
end

function vrand( lower, upper )
	return math.random() * ( upper - lower ) + lower
end

ships = {}
ship_count = 0
--[[
function ship_spawner()
	local ship = gameobject_create( "dat/model/ship_hd.s" )
	vbody_registerCollisionCallback( ship.body, ship_collisionHandler )
	ship.speed = 30.0
	vtransform_yaw( ship.transform, 3.14 )
	x = vrand( -50.0, 50.0 )
	y = vrand( 0.0, 100.0 )
	position = Vector( x, y, 100.0, 1.0 )
	vtransform_setWorldPosition( ship.transform, position )

	ships[ ship_count ] = ship
	ship_count = ship_count + 1

	inTime( 0.1, function () ship_tick( ship ) end )
	inTime( 3, ship_spawner )
end
--]]

function collision( this, other )
		
end

function ship_collisionHandler( ship, collider )
	spawn_explosion( ship.transform );
	ship_destroy( ship )

	--[[
	if collider == bullet then
		ship_destroy( ship )
	end
	--]]
end

function ship_destroy( ship )
	gameobject_destroy( ship )
	-- spawn explosion
end

function setup_controls()
	-- Set up steering input for the player ship
	if touch_enabled then
		-- Steering
		local w = 720
		local h = 720
		local x = 1280 - w
		local y = 720 - h
		player_ship.joypad_mapper = drag_map()
		player_ship.joypad = vcreateTouchPad( input, x, y, w, h )
		player_ship.steering_input = steering_input_drag
		-- UI drawing is upside down compared to touchpad placement - what to do about this?

		-- Firing Trigger
		local x = 0
		local y = 0
		local w = 1280 - 720
		local h = 720
		player_ship.fire_trigger = vcreateTouchPad( input, x, y, w, h )
		--vcreateUIPanel( engine, x, 720-y-h, w, h )
	else
		player_ship.steering_input = steering_input_keyboard
	end
	-- Crosshair
	local w = 128
	local h = 128
	local x = 640 - ( w / 2 )
	local y = 360 - ( h / 2 ) - 40
	vcreateCrosshair( engine, x, y, w, h )
end

function player_ship_collisionHandler( ship, collider )
	-- stop the ship
	ship.speed = 0.0
	local no_velocity = Vector( 0.0, 0.0, 0.0, 0.0 )
	vphysic_setVelocity( ship.physic, no_velocity )

	-- destroy it
	spawn_explosion( ship.transform )

	-- not using gameobject_destroy as we need to sync transform dying with camera rejig
	inTime( 0.2, function () vdeleteModelInstance( ship.model ) 
							vphysic_destroy( ship.physic )
				end )
	vdestroyBody( ship.body )

	-- queue a restart
	inTime( 2.0, function ()
		vdestroyTransform( scene, ship.transform )
		restart() 
	end )
	vprint( "### 3" )
end

collision_layer_player = 1
collision_layer_enemy = 2

function restart()
	-- We create a player object which is a game-specific Lua class
	-- The player class itself creates several native C classes in the engine
	player_ship = playership_create()

	-- Init position
	local start_position = Vector( 0.0, 0.0, 20.0, 1.0 ) 
	vtransform_setWorldPosition( player_ship.transform, start_position )

	-- Init velocity
	player_ship.speed = 0.0
	local no_velocity = Vector( 0.0, 0.0, player_ship.speed, 0.0 )
	vphysic_setVelocity( player_ship.physic, no_velocity )
	inTime( 3.0, function () player_ship.speed = 30.0 end )

	setup_controls()
	--vtransform_yaw( player_ship.transform, math.pi * 2 * 1.32 );
	chasecam = vchasecam_follow( engine, player_ship.camera_transform )
	flycam = vflycam( engine )
	vscene_setCamera( chasecam )
end

function loadParticles( )
	local t = vcreateTransform()
	local particle
	particle = vparticle_create( engine, t, "dat/script/lisp/missile_explosion.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion_b.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion_c.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/bullet.s" )
	vparticle_destroy( particle )
	vmodel_preload( projectile_model )
end

function testSpawns()
	spawn_v = -3.0
	local width = 25.0
	while spawn_v < 3.0 do
		spawn_v = spawn_v + 0.3
		spawn_atCanyon( -width, spawn_v, "dat/model/skyscraper.s" )
		spawn_atCanyon( width, spawn_v, "dat/model/skyscraper.s" )
	end
end

function start()
	loadParticles()

	restart()

	--tm.test()
	--ai.test_combinator()
	ai.test_states()

	-- testSpawns()
	--ship_spawner()
end

wave_interval_time = 10.0

function sign( x )
	if x > 0 then
		return 1.0
	else
		return -1.0
	end
end

-- maps a touch input on the joypad into a joypad tilt
-- can have a defined deadzone in the middle (resulting in 0)
function joypad_mapSquare( width, height, deadzone_x, deadzone_y )
	return function( x, y ) 	
		center_x = width / 2
		center_y = height / 2
		x = sign( x - center_x ) * math.max( math.abs( x - center_x ) - deadzone_x, 0.0 ) / (( width - deadzone_x ) / 2 )
		y = sign( y - center_y ) * math.max( math.abs( y - center_y ) - deadzone_y, 0.0 ) / (( height - deadzone_y ) / 2 )
		return x,y
	end
end

function steering_input_joypad()
	-- Using Joypad
	local yaw = 0.0
	local pitch = 0.0
	touched, joypad_x, joypad_y = vtouchPadTouched( player_ship.joypad )
	if touched then
		yaw, pitch = player_ship.joypad_mapper( joypad_x, joypad_y )
		vprint( "inputs mapped " .. yaw .. " " .. pitch )
	end
	return yaw, pitch
end


function steering_input_drag()
	-- Using Joypad
	local yaw = 0.0
	local pitch = 0.0
	dragged, drag_x, drag_y = vtouchPadDragged( player_ship.joypad )
	if dragged then
		yaw, pitch = player_ship.joypad_mapper( drag_x, drag_y )
	end
	return yaw, pitch
end

function drag_map()
	return function( x, y )
		x_scale = 15.0
		y_scale = 15.0
		return x / x_scale, y / y_scale
	end
end


function steering_input_keyboard()
	local yaw = 0.0
	local pitch = 0.0
	-- Steering
	if vkeyHeld( input, key.left ) then
		yaw = -1.0
	end
	if vkeyHeld( input, key.right ) then
		yaw = 1.0
	end
	if vkeyHeld( input, key.up ) then
		pitch = -1.0
	end
	if vkeyHeld( input, key.down ) then
		pitch = 1.0
	end
	return yaw, pitch
end
function playership_weaponsTick( ship, dt )
	-- Gunfire
	local fired = false
	if touch_enabled then
		fired, joypad_x, joypad_y = vtouchPadTouched( ship.fire_trigger )
	else
		fired = vkeyPressed( input, key.space )
	end
	if fired then
		player_fire( ship )
	end
	ship.cooldown = ship.cooldown - dt
end

function clamp( min, max, value )
	return math.min( max, math.max( min, value ))
end

function lerp( a, b, k )
	return a + ( b - a ) * k
end

function playership_tick( ship, dt )
	yaw_per_second = 1.5 
	pitch_per_second = 1.5

	local input_yaw = 0.0
	local input_pitch = 0.0
	input_yaw, input_pitch = ship.steering_input()

	-- set to -1.0 to invert
	invert_pitch = 1.0
	pitch = invert_pitch * input_pitch * pitch_per_second * dt;
	yaw = input_yaw * yaw_per_second * dt;

	ship.yaw = ship.yaw + yaw
	ship.pitch = ship.pitch + pitch
	target_roll = yaw * -60.0;
	max_roll = 1.0
	max_roll_delta = 2.0 * dt
	delta = target_roll - ship.roll
	roll_delta = clamp( -max_roll_delta, max_roll_delta, delta )
	roll = ship.roll + roll_delta
	ship.roll = clamp( -max_roll, max_roll, roll )
	
	vtransform_eulerAngles( ship.transform, ship.yaw, ship.pitch, ship.roll )
	-- Camera transform shares ship position and yaw, pitch; but not roll
	vtransform_setWorldSpaceByTransform( ship.camera_transform, ship.transform )
	vtransform_eulerAngles( ship.camera_transform, ship.yaw, ship.pitch, 0.0 )

	-- throttle
	width = 100
	acceleration = 1.0
	delta_speed = acceleration * dt;
	ship.speed = ship.speed + delta_speed

	playership_weaponsTick( ship, dt )

	-- Physics
	world_v = vtransformVector( ship.transform, Vector( 0.0, 0.0, ship.speed, 0.0 ))
	vphysic_setVelocity( ship.physic, world_v )
end

function toggle_camera()
	if camera == "chase" then
		vprint( "Activate Flycam" )
		camera = "fly"
		vscene_setCamera( flycam )
	else
		vprint( "Activate Chasecam" )
		camera = "chase"
		vscene_setCamera( chasecam )
	end
end

function debug_tick()
	if vkeyPressed( input, key.c ) then
		toggle_camera()
	end
end

function sky_tick( camera, dt )
	camera_position = vcamera_position( camera )
	u,v = vworldPositionFromCanyon( camera_position )
	--vdynamicSky_blend( v )
end

-- Called once per frame to update the current Lua State
function tick( dt )
	if starting then
		starting = false
		start()
	end

	sky_tick( chasecam, dt )

	playership_tick( player_ship, dt )

	debug_tick()

	timers_tick( dt )

	update_spawns( player_ship )

	tick_array( turrets, dt )
	tick_array( interceptors, dt )
end

-- Called on termination to clean up after itself
function terminate()
	player = nil
end

function delay( time, command )
	if time <= 0 then
		command()
	else
		print( string.format( "Delay timer: %d", time ))
		delay( time-1, command )
	end
end

function spawn_index( pos )
	return math.floor( ( pos - spawn_offset ) / spawn_interval )
end

function spawn_pos( i )
	return i * spawn_interval + spawn_offset;
end

turrets = { count = 0 }
interceptors = { count = 0 }


turret_cooldown = 0.4

function turret_fire( turret )
	muzzle_position = Vector( 4.0, 6.0, 0.0, 1.0 )
	fire_enemy_missile( turret, muzzle_position )
	muzzle_position = Vector( -4.0, 6.0, 0.0, 1.0 )
	fire_enemy_missile( turret, muzzle_position )
end

function turret_tick( turret, dt )
	turret.state = turret.state( turret, dt )
end

function tick_array( array, dt )
	for element in iterator( array ) do
		element.tick( element, dt )
	end
end

function spawn_turret( u, v )
	local spawn_height = 20.0
	vprint( "Spawn_turret, v = " .. v )

	-- position
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y + spawn_height, z, 1.0 )
	local turret = gameobject_create( "dat/model/gun_turret.s" )
	vtransform_setWorldPosition( turret.transform, position )

	-- Orientation
	local facing_x, facing_y, facing_z = vcanyon_position( u, v - ( 1.0 / canyon_v_scale ))
	local facing_position = Vector( facing_x, y + spawn_height, facing_z, 1.0 )
	vtransform_facingWorld( turret.transform, facing_position )

	-- Physics
	vbody_registerCollisionCallback( turret.body, target_collisionHandler )
	vbody_setLayers( turret.body, collision_layer_enemy )
	vbody_setCollidableLayers( turret.body, collision_layer_player )

	turret.tick = turret_tick
	turret.cooldown = turret_cooldown

	-- ai
	turret.state = turret_state_inactive

	turrets[turrets.count] = turret
	turrets.count = turrets.count + 1
end

function spawn_target( v )
	local spawn_height = 20.0
	vprint( "Spawn_target, v = " .. v )

	-- position
	local u = 0.0
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y + spawn_height, z, 1.0 )
	local target = gameobject_create( "dat/model/target.s" )
	vtransform_setWorldPosition( target.transform, position )

	-- Orientation
	local facing_x, facing_y, facing_z = vcanyon_position( u, v - ( 1.0 / canyon_v_scale ))
	local facing_position = Vector( facing_x, y + spawn_height, facing_z, 1.0 )
	vtransform_facingWorld( target.transform, facing_position )

	-- Physics
	vbody_registerCollisionCallback( target.body, target_collisionHandler )
	vbody_setLayers( target.body, collision_layer_enemy )
	vbody_setCollidableLayers( target.body, collision_layer_player )
end

function target_collisionHandler( target, collider )
	spawn_explosion( target.transform )
	gameobject_destroy( target )
end

function spawn_atCanyon( u, v, model )
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y, z, 1.0 )
	local obj = gameobject_create( model )
	vtransform_setWorldPosition( obj.transform, position )
end

-- spawn properties
spawn_offset = 0.0
spawn_interval = 200.0
spawn_distance = 300.0
-- spawn tracking
last_spawn = 0.0
last_spawn_index = 0


function contains( value, range_a, range_b )
	range_max = math.max( range_a, range_b )
	range_min = math.min( range_a, range_b )
	return ( value < range_max ) and ( value >= range_min )
end

canyon_v_scale = 1.0

-- Spawn all entities in the given range
function entities_spawnAll( near, far )
	near = near / canyon_v_scale
	far = far / canyon_v_scale
	i = spawn_index( near ) + 1
	--vprint( "Evaluating spawn index: " .. i )
	spawn_v = i * spawn_interval
	--vprint( "spawn_v " .. spawn_v .. ", near " .. near .. ", far " .. far )
	while contains( spawn_v, near, far ) do
		if ( i > last_spawn_index ) then
			vprint( "Spawning at " .. spawn_v .. "!" )
			--spawn_target( spawn_v )
			local turret_offset_u = 20.0
			--spawn_turret( turret_offset_u, spawn_v )
			--spawn_turret( -turret_offset_u, spawn_v )
			spawn_interceptor( 0, spawn_v )
			last_spawn_index = i
			i = i + 1
			spawn_v = i * spawn_interval
		end
	end
end

-- Spawn all entities that need to be spawned this frame
function update_spawns( ship )
	ship_pos = vtransform_getWorldPosition( ship.transform )
	u,v = vworldPositionFromCanyon( ship_pos )
	--x,y,z,w = vvector_values( ship_pos )
	far = v + spawn_distance
	entities_spawnAll( last_spawn, far )
	last_spawn = far;
end

function turret_state_inactive( turret, dt )
	--vprint( "inactive" )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )
	--[[
	if player_close then
		vprint( "player close true" )
	else
		vprint( "player close false" )
	end
	--]]

	if player_close then
		return turret_state_active
	else
		return turret_state_inactive
	end
end

function turret_state_active( turret, dt )
	--vprint( "active" )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )
	--[[
	if player_close then
		vprint( "player close true" )
	else
		vprint( "player close false" )
	end
	--]]

	if turret.cooldown < 0.0 then
		turret_fire( turret )
		turret.cooldown = turret_cooldown
	end
	turret.cooldown = turret.cooldown - dt

	if player_close then
		return turret_state_active
	else
		return turret_state_inactive
	end
end

function entity_setSpeed( entity, speed )
	entity.speed = speed
	entity_velocity = Vector( 0.0, 0.0, entity.speed, 0.0 )
	world_velocity = vtransformVector( entity.transform, entity_velocity )
	vphysic_setVelocity( entity.physic, world_velocity )
end

function entity_moveTo( position )
	return function ( entity )
		--entity.transform = direction_to_position
		entity_setSpeed( entity, 10.0 )
	end
end

function entity_attack()
	return function ( entity )
		entity_setSpeed( entity, 0.0 )
	end
end

function entity_atPosition( entity, position, max_distance )
	distance = vtransform_distance( entity.transform, position )
	return distance < max_distance
end

function spawn_interceptor( u, v )
	local y_height = 20
	local u_offset = -40
	local y_offset = 40
	local x, y, z = vcanyon_position( u + u_offset, v )
	spawn_position = Vector( x, y + y_height + y_offset, z, 1.0 )
	local x, y, z = vcanyon_position( u, v )
	attack_position = Vector( x, y + y_height, z, 1.0 )
	retreat_position = spawn_position
	
	local interceptor = gameobject_create( "dat/model/ship_hd.s" )
	vtransform_setWorldPosition( interceptor.transform, spawn_position )

	--[[
	enter = nil
	attack = nil
	exit = nil
	enter =		ai.state( entity_moveTo( attack_position ),		function () if entity_atPosition( entity, attack_position, 10.0 ) then return attack else return enter end end )
	attack =	ai.state( entity_attack,						function () if time_in_state > 5 then return exit else return attack end end )
	exit = 		ai.state( entity_moveTo( retreat_position ),	function () return exit )
	--]]
	enter =		ai.state( entity_moveTo( attack_position ),		
							function () if entity_atPosition( entity, attack_position, 10.0 ) then 
									return nil 
								else 
									return enter 
								end 
							end )
	interceptor.behaviour = enter
	interceptor.tick = interceptor_tick

	interceptors[interceptors.count] = interceptor
	interceptors.count = interceptors.count + 1
end

function interceptor_tick( interceptor, dt )
	--vprint( "Interceptor tick." )
	interceptor.behaviour = interceptor.behaviour( interceptor, dt )
end
