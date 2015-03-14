-- SpaceSim main game lua script

--[[

This file contains the main game routines for the SpaceSim game.
Most game logic is written in Lua, whilst the core numerical processing (rendering, physics, animation etc.)
are handled by the Vitae engine in C.

Lua should be able to do everything C can, but where performance is necessary, code should be rewritten in
C and only controlled remotely by Lua

]]--
	ship = "dat/model/ship_diamond.s"
	cube = "dat/model/cube.s"
	sphere = "dat/model/sphere.s"
	gun = "dat/model/gun.s"
	models = {}
	models[1] = sphere
	models[2] = cube
	models[3] = ship
	models[4] = gun
	models.count = 4
	modelIndex = 1

	pi = math.pi
	two_pi = 2.0 * pi

	local z_axis = Vector( 0.0, 0.0, 1.0, 0.0 )

-- Debug settings
	debug_spawning_disabled	= true
	debug_doodads_disabled	= true
	debug_player_immortal	= true
	debug_player_autofly	= false
	debug_player_immobile	= true
	debug_no_pause_fade = true

-- Load Modules
	--package.path = "./SpaceSim/lua/?.lua"
	ai				= require "ai"
	array			= require "array"
	entities	= require "entities"
	fx				= require "fx"
	future		= require "future"
	library		= require "library"
	list			= require "list"
	option		= require "option"
	spawn			= require "spawn"
	timers		= require "timers"
	triggers	= require "triggers"
	ui				= require "ui"

-- player - this object contains general data about the player
	player = nil

-- player_ship - the actual ship entity flying around
	player_ship = nil

-- Collision
	collision_layer_player	= 1
	collision_layer_enemy	= 2
	collision_layer_bullet	= 4
	collision_layer_terrain	= 8

-- Camera
	camera		= "fly"
	flycam		= nil
	chasecam 	= nil

-- Entities
	missiles		= { count = 0 }
	turrets			= { count = 0 }
	interceptors	= { count = 0 }
	all_doodads			= { count = 0 }

-- Settings
	-- Weapons
	player_gun_cooldown		= 0.15
	player_missile_cooldown	= 1.0
	homing_missile_turn_angle_per_second = pi / 2
	-- Flight
	if debug_player_immobile then
		player_ship_initial_speed	= 0.0
		player_ship_acceleration	= 0.0
	else
		player_ship_initial_speed	= 50.0
		player_ship_acceleration	= 1.0
	end
	player_ship_max_speed		= 150.0
	max_allowed_roll			= 1.5
	camera_roll_scale			= 0.1
	aileron_roll_duration		= 0.8
	-- Controls
	aileron_swipe 			= { 
		distance = 150.0,
		duration = 0.2,
		angle_tolerance = 0.1 
	}
	missile_swipe 			= { 
		distance = 150.0,
		duration = 0.2,
		angle_tolerance = 0.1
	}

-- Enemies
	turret_cooldown			= 0.5		-- (seconds)
	homing_missile_cooldown = 3.0		-- (seconds)

-- Doodads
	doodads = {}
	doodads.interval = 30.0

-- spawn properties
	spawn_offset = 0.0
	spawn_interval = 300.0
	spawn_distance = 900.0
	doodad_spawn_distance = 1500.0
	despawn_distance = 100.0			-- how far behind to despawn units
-- spawn tracking
	entities_spawned = 0.0
	doodads_spawned = 0.0

	tickers = list:empty() 


-- Create a spacesim Game object
-- A gameobject has a visual representation (model), a physical entity for velocity and momentum (physic)
-- and a transform for locating it in space (transform)
function gameobject_create( model_file )
	local g = {}
	g.model = vcreateModelInstance( model_file )
	g.physic = vcreatePhysic()
	g.transform = vcreateTransform()
	g.body = vcreateBodySphere( g )
	vmodel_setTransform( g.model, g.transform )
	vphysic_setTransform( g.physic, g.transform )
	vbody_setTransform( g.body, g.transform )
	vscene_addModel( scene, g.model )
	vphysic_activate( engine, g.physic )
	local v = Vector( 0.0, 0.0, 0.0, 0.0 )
	vphysic_setVelocity( g.physic, v )

	return g
end

function gameobject_createAt( model_file, matrix )
	local g = {}
	g.model = vcreateModelInstance( model_file )
	g.physic = vcreatePhysic()
	g.transform = vcreateTransform()
	vtransform_setWorldSpaceByMatrix( g.transform, matrix )
	g.body = vcreateBodySphere( g )
	vmodel_setTransform( g.model, g.transform )
	vphysic_setTransform( g.physic, g.transform )
	vbody_setTransform( g.body, g.transform )
	vscene_addModel( scene, g.model )
	vphysic_activate( engine, g.physic )
	local v = Vector( 0.0, 0.0, 0.0, 0.0 )
	vphysic_setVelocity( g.physic, v )

	return g
end

function gameobject_destroy( g )
	inTime( 0.2, function() gameobject_delete( g ) end )

	if g.body then
		vdestroyBody( g.body )
		g.body = nil
	end
end

function gameobject_delete( g )
	if g.model then
		vdeleteModelInstance( g.model )
		g.model = nil
	end
	if vtransform_valid(g.transform) then
		vdestroyTransform( scene, g.transform )
		g.transform = nil
	end
	if g.physic then
		vphysic_destroy( g.physic )
		g.physic = nil
	end
	if g.body then
		vdestroyBody( g.body )
		g.body = nil
	end
end

projectile_model = "dat/model/missile.s"

function player_fire( ship )
	if ship.cooldown <= 0.0 then
		local muzzle_position	= Vector( 1.2, 1.0, 1.0, 1.0 );
		fire_missile( ship, muzzle_position, player_gunfire )
		fx.muzzle_flare( ship, muzzle_position )
		muzzle_position			= Vector( -1.2, 1.0, 1.0, 1.0 );
		fire_missile( ship, muzzle_position, player_gunfire )
		fx.muzzle_flare( ship, muzzle_position )
		ship.cooldown = player_gun_cooldown
	end
end

function player_fire_missile_swarm( ship )
	vprint( "Missile swarm!" )
	local enemies = findClosestEnemies( ship.transform, 4 )
	enemies:zipWithIndex():foreach( function( target ) 
		inTime( (target[2] - 1) * 0.1, function() 
			player_fire_missile( ship, target[1] ) end ) end )
end

function findClosestEnemies( transform, count )
	local targets = array.filter( interceptors, 
	function ( interceptor )
		if not vtransform_valid( interceptor.transform ) then return 1000000.0 end
		local interceptor_position = vtransform_getWorldPosition( interceptor.transform )
		local current_position = vtransform_getWorldPosition( transform )
		interceptor_position:foreach( function( i_pos )
			current_position:foreach( function( c_pos )
				local displacement = vvector_subtract( i_pos, c_pos )
				local direction = vtransformVector( transform, z_axis )
				return vvector_dot( displacement, direction ) > 0 
			end)
		end)
	end )
	return list.fromArray( targets ):take(count)
end

function findClosestEnemy( transform )
	local enemies = findClosestEnemies( transform, 1 )
	return enemies.head
end

function player_fire_missile( ship, target )
	muzzle_position = Vector( 0.0, 0.0, 1.0, 1.0 );
	local missile = fire_missile( ship, muzzle_position, player_missile )
	missile.tick = homing_missile_tick( target.transform )
end

function safeCleanup( value, cleanup )
	if value then
		cleanup()
	end
	return nil
end

function inTime( time, action )
	timers.add( timers.create( time, action ))
end

function triggerWhen( trigger, action )
	triggers.add( triggers.create( trigger, action ))
end

function playership_cleanup( p )
	if p and vtransform_valid( p.camera_transform ) then
		vdestroyTransform( scene, p.camera_transform )
	end
end

-- Create a player. The player is a specialised form of Gameobject
function playership_create()
	local p = gameobject_create( models[modelIndex] )
	modelIndex = modelIndex % models.count + 1
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
	vbody_registerCollisionCallback( p.body, player_ship_collisionHandler )
	vbody_setLayers( p.body, collision_layer_player )
	vbody_setCollidableLayers( p.body, bitwiseOR( collision_layer_enemy, collision_layer_terrain ))

	return p
end

starting = true

-- Set up the Lua State
function init()
	vprint( "init" )
	spawn.init()
	doodads.random = vrand_newSeq()
	--vskybox_create( engine )

	starting = true
	color = Vector( 1.0, 1.0, 1.0, 1.0 )
	
	inTime( 1.0, function () 
				gameplay_start()
			end )
	--splash_intro_new()
end

function ship_destroy( ship )
	array.remove( interceptors, ship )
	gameobject_destroy( ship )
end

function ship_delete( ship )
	array.remove( interceptors, ship )
	gameobject_delete( ship )
	ship.behaviour = ai.dead
end

function doodad_delete( doodad )
	array.remove( all_doodads, doodad )
	gameobject_delete( doodad )
end

function setup_controls()
	-- Set up steering input for the player ship
	if touch_enabled then
		-- Steering
		local w = 720
		local h = screen_height
		local x = screen_width - w
		local y = screen_height - h
		player_ship.joypad_mapper = drag_map()
		player_ship.joypad = vcreateTouchPad( input, x, y, w, h )
		player_ship.steering_input = steering_input_drag
		-- UI drawing is upside down compared to touchpad placement - what to do about this?

		-- Firing Trigger
		local x = 0
		local y = 0
		local w = screen_width - 720
		local h = screen_height
		player_ship.fire_trigger = vcreateTouchPad( input, x, y, w, h )

		local swipe_left = { direction = Vector( -1.0, 0.0, 0.0, 0.0 ) }
		local swipe_right = { direction = Vector( 1.0, 0.0, 0.0, 0.0 ) }
		player_ship.aileron_swipe_left = vgesture_create( aileron_swipe.distance, aileron_swipe.duration, swipe_left.direction, aileron_swipe.angle_tolerance )
		player_ship.aileron_swipe_right = vgesture_create( aileron_swipe.distance, aileron_swipe.duration, swipe_right.direction, aileron_swipe.angle_tolerance )
		local missile_swipe_direction = Vector( 0.0, 1.0, 0.0, 0.0 )
		player_ship.missile_swipe = vgesture_create( missile_swipe.distance, missile_swipe.duration, missile_swipe_direction, missile_swipe.angle_tolerance )
	else
		player_ship.steering_input = steering_input_keyboard
	end
end

function player_ship_collisionHandler( ship, collider )
	if not debug_player_immortal then
		-- stop the ship
		ship.speed = 0.0
		local no_velocity = Vector( 0.0, 0.0, 0.0, 0.0 )
		vphysic_setVelocity( ship.physic, no_velocity )

		-- destroy it
		fx.spawn_explosion( ship.transform )

		-- not using gameobject_destroy as we need to sync transform dying with camera rejig
		inTime( 0.2, function () vdeleteModelInstance( ship.model ) 
			vphysic_destroy( ship.physic )
			ship.physic = nil
		end )
		vdestroyBody( ship.body )

		-- queue a restart
		inTime( 2.0, function ()
			vprint( "Restarting" )
			vdestroyTransform( scene, ship.transform )
			restart() 
			gameplay_start()
		end )
	end
end

function gameplay_start()
	player_active = true
	inTime( 2.0, function () 
		player_ship.speed = player_ship_initial_speed
		if not debug_spawning_disabled then
			spawning_active = true
		end
		entities_spawned = 0.0
	end )
end

function restart()
	spawning_active = false
	player_active = false
	entities_despawnAll()

	vscene_setSkyColor( scene, Vector(0.3, 0.4, 0.5, 1.0))
	vscene_setFogColor( scene, Vector(0.4, 0.4, 0.3, 1.0))
	vscene_setSunColor( scene, Vector(0.0, 0.0, 0.0, 1.0))

	-- We create a player object which is a game-specific Lua class
	-- The player class itself creates several native C classes in the engine
	if player_ship then
		gameobject_destroy( player_ship )
	end
	playership_cleanup( player_ship )
	player_ship = playership_create()

	-- Init position
	local start_position = Vector( 0.0, 0.0, 3.5, 1.0 ) 
	vtransform_setWorldPosition( player_ship.transform, start_position )

	-- Init velocity
	player_ship.speed = 0.0
	local no_velocity = Vector( 0.0, 0.0, player_ship.speed, 0.0 )
	vphysic_setVelocity( player_ship.physic, no_velocity )

	player_ship.target_roll = library.rolling_average.create( 5 )

	chasecam = vchasecam_follow( engine, player_ship.camera_transform )
	flycam = vflycam( engine )
	vscene_setCamera( flycam )
	setup_controls()
end

local touchpad = {}
function touchpad:new()
	local p = { pad = nil, onTouchHandlers = list:empty() }
	setmetatable(p, self)
	self.__index = self
	return p
end

function touchpad:onTouch( func )
	self.onTouchHandlers = list.cons( func, self.onTouchHandlers )
end

function touchpad:tick( dt )
	touched, x, y = vtouchPadTouched( player_ship.joypad )
	if not touch_enabled then
		touched = vkeyPressed( input, key.space )
	end
	if touched then
		vprint( "touched" )
		self.onTouchHandlers:foreach( apply )
	end
end

function createTouchPad( input, x, y, w, h )
	local pad = touchpad:new()
	pad.pad = vcreateTouchPad( input, x, y, w, h )
	addTicker( pad )
	return pad
end

function start()
	fx.preload()

	restart()
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
	local touched, joypad_x, joypad_y = vtouchPadTouched( player_ship.joypad )
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

function clamp( min, max, value )
	return math.min( max, math.max( min, value ))
end

function lerp( a, b, k )
	return a + ( b - a ) * k
end

function ship_aileronRollActive( ship ) 
	local roll_offset = library.modf( ship.roll + pi, two_pi ) - pi
	return not ( ship.aileron_roll_time < 0.0 and math.abs( roll_offset ) < 0.4 )
end

function ship_rollFromYawRate( ship, yaw_delta )
	local last_roll = library.rolling_average.sample( ship.target_roll ) or 0.0
	local offset = math.floor(( last_roll + pi ) / two_pi ) * two_pi
	local yaw_to_roll = -45.0
	return clamp( -max_allowed_roll, max_allowed_roll, yaw_delta * yaw_to_roll ) + offset
end

function ship_rollDeltaFromTarget( target, current )
	local target_delta = target - current
	local max_roll_delta = math.min( 4.0 * dt, math.abs( target_delta / 2.0 ))
	return clamp( -max_roll_delta, max_roll_delta, target_delta )
end

-- Distorted sin curve for quicker attack and longer decay
-- sin ( (pi-x)^2 / pi )
function ship_strafeRate( ratio )
	return clamp( 0.0, 1.0, math.sin( pi + ratio*ratio / pi - 2.0 * ratio ))
end

function yaw( ship, dt )
	local target_yaw_delta = ship.target_yaw - ship.yaw
	--local max_yaw_delta = 2.0 * math.abs( ship.roll ) * 1.3 * dt
	local max_yaw_delta = 2.0
	local actual_yaw_delta = clamp( -max_yaw_delta, max_yaw_delta, target_yaw_delta )
	ship.yaw = ship.yaw + actual_yaw_delta
	return target_yaw_delta
end

function playership_tick( ship, dt )
	if ship == nil or ship.transform == nil then return end
	local yaw_per_second = 0.5
	local pitch_per_second = 1.5

	local input_yaw = 0.0
	local input_pitch = 0.0

	-- set to -1.0 to invert
	local invert_pitch = 1.0
	local pitch = invert_pitch * input_pitch * pitch_per_second * dt;
	local yaw_delta = 1.0 * yaw_per_second * dt;

	-- pitch
	ship.pitch = ship.pitch + pitch

	local strafe = 0.0

	ship.yaw_history[ship.yaw_history_index ] = ship.yaw
	ship.yaw_history_index = ( ship.yaw_history_index ) % 10 + 1

	local camera_roll = 0.0
	if ship.aileron_roll then
		-- strafe
		local roll_rate = ship_strafeRate( ship.aileron_roll_time / aileron_roll_duration )
		local strafe_speed = -1500.0 * roll_rate
		strafe = strafe_speed * dt * ship.aileron_roll_multiplier

		-- roll
		library.rolling_average.add( ship.target_roll, ship.aileron_roll_target )
		local integral_total = 1.8 -- This is such a fudge - should be integral [0->1] of sin( pi^2 - x^2 / pi ) ( which is 1.58605 )
		local roll_delta = roll_rate * dt * ship.aileron_roll_amount * integral_total
		ship.roll = ship.roll + roll_delta

		ship.aileron_roll_time = ship.aileron_roll_time - dt
		ship.aileron_roll = ship_aileronRollActive( ship )
		camera_roll = max_allowed_roll * camera_roll_scale * ship.aileron_roll_multiplier

		yaw(ship,dt)

	else
		ship.target_yaw = ship.target_yaw + yaw_delta
		local target_yaw_delta = yaw( ship, dt )

		-- roll
		local target_roll = ship_rollFromYawRate( ship, target_yaw_delta )
		library.rolling_average.add( ship.target_roll, target_roll )
		local roll_delta = ship_rollDeltaFromTarget( library.rolling_average.sample( ship.target_roll ), ship.roll )
		ship.roll = ship.roll + roll_delta
		local roll_offset = library.modf( ship.roll + pi, two_pi ) - pi
		camera_roll = roll_offset * camera_roll_scale
	end

	ship.roll = 0.0
	
	vtransform_eulerAngles( ship.transform, ship.yaw, ship.pitch, ship.roll )
	-- Camera transform shares ship position and yaw, pitch; but not roll
	vtransform_setWorldSpaceByTransform( ship.camera_transform, ship.transform )
	local camera_target_position = vtransformVector( ship.transform, Vector( 0.0, 0.0, 20.0, 1.0 ))
	vtransform_setWorldPosition( ship.camera_transform, camera_target_position )
	vtransform_eulerAngles( ship.camera_transform, ship.yaw, ship.pitch, camera_roll )

	-- throttle
	width = 100
	delta_speed = player_ship_acceleration * dt;
	ship.speed = math.min( ship.speed + delta_speed, player_ship_max_speed )

	--playership_weaponsTick( ship, dt )

	-- Physics
	local forward_v = vtransformVector( ship.transform, Vector( 0.0, 0.0, ship.speed, 0.0 ))
	local strafe_v = vtransformVector( ship.camera_transform, Vector( strafe, 0.0, 0.0, 0.0 ))
	local world_v = vvector_add( forward_v, strafe_v )
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
	if vkeyPressed( input, key.space ) then
		restart()
		gameplay_start()
	end
end

spawning_active = false

function tickPause( dt )
	local togglePause = vkeyPressed( input, key.p )
	if togglePause then
		if paused then
			paused = false
			if pauseFrame then ui.hide_splash( pauseFrame ) end
			vunpause( engine )
		else
			paused = true
			vpause( engine )
			if pauseFrame then ui.hide_splash( pauseFrame ) end
			local alpha = 0.3	
			if not debug_no_pause_fade then
				pauseFrame = ui.show_splash_withColor( "dat/img/black.tga", screen_width, screen_height, Vector( 1.0, 1.0, 1.0, alpha ))
			end
		end
	end
end

-- Called once per frame to update the current Lua State
function tick( dt )
	if starting then
		starting = false
		start()
	end

	tickPause( dt )

	if player_active then
		playership_tick( player_ship, dt )
	end

	debug_tick()

	timers.tick( dt )
	triggers.tick( dt )

	if spawning_active then
		update_spawns( player_ship.transform )
		update_despawns( player_ship.transform )
	end

	if not debug_doodads_disabled then
		update_doodads( player_ship.transform )
		update_doodad_despawns( player_ship.transform )
	end

	tick_array( turrets, dt )
	tick_array( interceptors, dt )
	tick_array( missiles, dt )

	tickers:foreach( function( ticker )
		ticker:tick( dt )
	end )
end

function addTicker( ticker )
	tickers = list.cons( ticker, tickers )
end

function removeTicker( ticker )
	tickers = tickers:remove( ticker )
end

-- Called on termination to clean up after itself
function terminate()
	player = nil
end

function tick_array( arr, dt )
	for element in array.iterator( arr ) do
		if element.tick then
			element.tick( element, dt )
		end
	end
end
