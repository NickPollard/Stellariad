-- SpaceSim main game lua script

--[[

This file contains the main game routines for the SpaceSim game.
Most game logic is written in Lua, whilst the core numerical processing (rendering, physics, animation etc.)
are handled by the Vitae engine in C.

Lua should be able to do everything C can, but where performance is necessary, code should be rewritten in
C and only controlled remotely by Lua

]]--

	two_pi = 2.0 * math.pi

-- Debug settings
	debug_spawning_enabled	= false
	debug_doodads_enabled	= true

-- Load Modules
	package.path = "./SpaceSim/lua/?.lua"
	ai			= require "ai"
	array		= require "array"
	entities	= require "entities"
	fx			= require "fx"
	library		= require "library"
	spawn		= require "spawn"
	timers		= require "timers"
	triggers	= require "triggers"
	ui			= require "ui"

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
	camera		= "chase"
	flycam		= nil
	chasecam 	= nil

-- Entities
	missiles		= { count = 0 }
	turrets			= { count = 0 }
	interceptors	= { count = 0 }

-- Settings
	-- Weapons
	player_gun_cooldown		= 0.15
	player_missile_cooldown	= 1.0
	-- Flight
	player_ship_initial_speed	= 30.0
	player_ship_acceleration	= 1.0
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

-- Create a spacesim Game object
-- A gameobject has a visual representation (model), a physical entity for velocity and momentum (physic)
-- and a transform for locating it in space (transform)
function gameobject_create( model_file )
	local g = {}
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

function gameobject_createAt( model_file, matrix )
	local g = {}
	g.model = vcreateModelInstance( model_file )
	g.physic = vcreatePhysic()
	g.transform = vcreateTransform()
	vtransform_setWorldSpaceByMatrix( g.transform, matrix )
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
	if g.transform then
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
	player_fire_missile( ship )
	inTime( 0.1, function () player_fire_missile( ship ) end )
	inTime( 0.2, function () player_fire_missile( ship ) end )
	inTime( 0.3, function () player_fire_missile( ship ) end )
end

function player_fire_missile( ship )
	muzzle_position = Vector( 0.0, 0.0, 0.0, 1.0 );
	fire_missile( ship, muzzle_position, player_missile )
end

function missile_destroy( missile )
	gameobject_destroy( missile )
end

function missile_collisionHandler( missile, other )
	fx.spawn_missile_explosion( missile.transform )
	vphysic_setVelocity( missile.physic, Vector( 0.0, 0.0, 0.0, 0.0 ))
	missile_destroy( missile )
end

function setCollision_playerBullet( object )
	vbody_setLayers( object.body, collision_layer_bullet )
	vbody_setCollidableLayers( object.body, bitwiseOR( collision_layer_enemy, collision_layer_terrain ))
--	vbody_setCollidableLayers( object.body, collision_layer_enemy )
end

function setCollision_enemyBullet( object )
	vbody_setLayers( object.body, collision_layer_bullet )
	vbody_setCollidableLayers( object.body, collision_layer_player )
end

function create_projectile( source, offset, model, speed ) 
	-- Position it at the correct muzzle position and rotation
	local muzzle_world_pos = vtransformVector( source.transform, offset )
	local muzzle_matrix = vtransformWorldMatrix( source.transform )
	vmatrix_setTranslation ( muzzle_matrix, muzzle_world_pos )

	--vtransform_setWorldSpaceByMatrix( projectile.transform, muzzle_matrix )
	-- Create a new Projectile
	local projectile = gameobject_createAt( model, muzzle_matrix )
	
	projectile.tick = nil


	--vtransform_setWorldSpaceByTransform( projectile.transform, source.transform )
	--vtransform_setWorldPosition( projectile.transform, muzzle_world_pos )

	-- Apply initial velocity
	local source_velocity = Vector( 0.0, 0.0, speed, 0.0 )
	local world_v = vtransformVector( source.transform, source_velocity )
	vphysic_setVelocity( projectile.physic, world_v );

	-- Store the projectile so it doesn't get garbage collected
	array.add( missiles, projectile )

	return projectile
end

player_gunfire = { 
	model = "dat/model/bullet_player.s",
 	particle = "dat/vfx/particles/bullet.s",
	speed = 350.0,
	collisionType = "player",
	time_to_live = 2.0
}

player_missile = { 
	model = "dat/model/missile_enemy_homing.s",
 	particle = "dat/script/lisp/red_bullet.s",
	speed = 100.0,
	collisionType = "player",
	time_to_live = 3.0
}

enemy_gunfire = { 
	model = "dat/model/bullet_player.s",
	speed = 150.0,
	collisionType = "enemy",
	time_to_live = 2.0
}

enemy_homing_missile = { 
	model = "dat/model/missile_enemy_homing.s",
	speed = 150.0,
	collisionType = "enemy",
	time_to_live = 3.0
}

function fire_missile( source, offset, bullet_type )
	local projectile = create_projectile( source, offset, bullet_type.model, bullet_type.speed )
	if bullet_type.collisionType == "player" then
		setCollision_playerBullet( projectile )
	elseif bullet_type.collisionType == "enemy" then
		setCollision_enemyBullet( projectile )
	end
	vbody_registerCollisionCallback( projectile.body, missile_collisionHandler )
	inTime( bullet_type.time_to_live, function () missile_destroy( projectile ) end )
	return projectile
end

homing_missile_turn_angle_per_second = math.pi / 2
function homing_missile_tick( target_transform )
	return function ( missile, dt )
		if missile.physic and missile.transform then
			local current_position = vtransform_getWorldPosition( missile.transform )
			local target_position = vtransform_getWorldPosition( target_transform )
			local target_direction = vvector_normalize( vvector_subtract( target_position, current_position ))
			local current_dir = vquaternion_fromTransform( missile.transform )
			local target_dir = vquaternion_look( target_direction )
			--local new_dir = vquaternion_slerp( current_dir, target_dir, 1.0 * dt )
			local new_dir = vquaternion_slerpAngle( current_dir, target_dir, homing_missile_turn_angle_per_second * dt )
			local world_velocity = vquaternion_rotation( new_dir, Vector( 0.0, 0.0, enemy_homing_missile.speed, 0.0 ))
			vphysic_setVelocity( missile.physic, world_velocity )
			vtransform_setRotation( missile.transform, new_dir )
		end
	end
end

function fire_enemy_homing_missile( source, offset, bullet_type )
	local projectile = fire_missile( source, offset, bullet_type )
	projectile.tick = homing_missile_tick( player_ship.transform )
end

function inTime( time, action )
	timers.add( timers.create( time, action ))
end

function triggerWhen( trigger, action )
	triggers.add( triggers.create( trigger, action ))
end

function playership_cleanup( p )
	if p and p.camera_transform then
		vdestroyTransform( scene, p.camera_transform )
	end
end

-- Create a player. The player is a specialised form of Gameobject
function playership_create()
	local p = gameobject_create( "dat/model/ship_new.s" )
	p.speed = 0.0
	p.cooldown = 0.0
	p.missile_cooldown = 0.0
	p.yaw = 0
	p.target_yaw = 0
	p.pitch = 0
	p.roll = 0
	p.aileron_roll = false
	p.aileron_roll_time = 5.0
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
	spawn.init()

	starting = true
	color = Vector( 1.0, 1.0, 1.0, 1.0 )
	local vignette = vuiPanel_create( engine, "dat/img/vignette.tga", color, 0, 360, 1280, 360 )
	
	splash_intro()
end

function ship_collisionHandler( ship, collider )
	fx.spawn_explosion( ship.transform );
	ship_destroy( ship )
	ship.behaviour = ai.dead
end

function ship_destroy( ship )
	gameobject_destroy( ship )
end

function ship_delete( ship )
	gameobject_delete( ship )
	ship.behaviour = ai.dead
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

		player_ship.roll_left = vcreateTouchPad( input, 0, 720 - 150, 150, 150 )
		player_ship.roll_right = vcreateTouchPad( input, 1280 - 150, 720 - 150, 150, 150 )

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

function gameplay_start()
	player_active = true
	inTime( 2.0, function () 
		player_ship.speed = player_ship_initial_speed
		--playership_addEngineGlows( player_ship )
		if debug_spawning_enabled then
			spawning_active = true
		end
		entities_spawned = 0.0
		doodads_spawned = 0.0
	end )
end

function restart()
	spawning_active = false
	player_active = false
	entities_despawnAll()
	-- We create a player object which is a game-specific Lua class
	-- The player class itself creates several native C classes in the engine
	playership_cleanup( player_ship )
	player_ship = playership_create()

	-- Init position
	local start_position = Vector( 0.0, 0.0, 20.0, 1.0 ) 
	vtransform_setWorldPosition( player_ship.transform, start_position )

	-- Init velocity
	player_ship.speed = 0.0
	local no_velocity = Vector( 0.0, 0.0, player_ship.speed, 0.0 )
	vphysic_setVelocity( player_ship.physic, no_velocity )

	player_ship.target_roll = library.rolling_average.create( 5 )

	chasecam = vchasecam_follow( engine, player_ship.camera_transform )
	flycam = vflycam( engine )
	vscene_setCamera( chasecam )
	setup_controls()
end

function loadParticles( )
	local t = vcreateTransform()
	local particle
	particle = vparticle_create( engine, t, "dat/vfx/particles/missile_explosion.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion_b.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/script/lisp/explosion_c.s" )
	vparticle_destroy( particle )
	particle = vparticle_create( engine, t, "dat/vfx/particles/bullet.s" )
	vparticle_destroy( particle )
	vdestroyTransform( scene, t )
	vmodel_preload( projectile_model )
	vmodel_preload( "dat/model/bullet_player.s" )
	vmodel_preload( "dat/model/missile_enemy_homing.s" )
end

function splash_intro()
	vtexture_preload( "dat/img/splash_author.tga" )
	local studio_splash = ui.show_splash( "dat/img/splash_vitruvian.tga" )
	inTime( 2.0, function () 
		ui.hide_splash( studio_splash ) 
		local author_splash = ui.show_splash( "dat/img/splash_author.tga" )
		inTime( 2.0, function ()
			ui.hide_splash( author_splash ) 
			ui.show_crosshair()
			gameplay_start()
		end )
	end )
end

function test()
end

function start()
	loadParticles()

	test()

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
	local missile_fired = false
	if touch_enabled then
		missile_fired = vgesture_performed( player_ship.fire_trigger, player_ship.missile_swipe )
	else
		missile_fired = vkeyPressed( input, key.q )
	end
	if missile_fired then
		if ship.missile_cooldown <= 0.0 then
			if ship.aileron_roll then
				player_fire_missile_swarm( ship )
			else
				player_fire_missile( ship )
			end
			ship.missile_cooldown = player_missile_cooldown
		end
	end
	ship.cooldown = ship.cooldown - dt
	ship.missile_cooldown = ship.missile_cooldown - dt
end

function clamp( min, max, value )
	return math.min( max, math.max( min, value ))
end

function lerp( a, b, k )
	return a + ( b - a ) * k
end

function ship_aileronRoll( ship, multiplier )
	local aileron_roll_delta = 2 * math.pi * multiplier
	ship.aileron_roll = true
	ship.aileron_roll_time = aileron_roll_duration
	ship.aileron_roll_multiplier = multiplier
	ship.aileron_roll_target = library.roundf( ship.roll + aileron_roll_delta + math.pi, 2.0 * math.pi )
	ship.aileron_roll_amount = ship.aileron_roll_target - ship.roll
	-- preserve heading from when we enter the roll
	ship.target_yaw = ship.yaw
end

function ship_aileronRollActive( ship ) 
	local roll_offset = library.modf( ship.roll + math.pi, 2 * math.pi ) - math.pi
	return not ( ship.aileron_roll_time < 0.0 and math.abs( roll_offset ) < 0.4 )
end

function ship_rollFromYawRate( ship, yaw_delta )
	local last_roll = library.rolling_average.sample( ship.target_roll ) or 0.0
	local offset = math.floor(( last_roll + math.pi ) / two_pi ) * two_pi
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
	return clamp( 0.0, 1.0, math.sin( math.pi + ratio*ratio / math.pi - 2.0 * ratio ))
end

function playership_tick( ship, dt )
	local yaw_per_second = 1.5 
	local pitch_per_second = 1.5

	local input_yaw, input_pitch = ship.steering_input()

	-- set to -1.0 to invert
	local invert_pitch = 1.0
	local pitch = invert_pitch * input_pitch * pitch_per_second * dt;
	local yaw_delta = input_yaw * yaw_per_second * dt;

	-- pitch
	ship.pitch = ship.pitch + pitch

	local strafe = 0.0

	if not ship.aileron_roll then
		local aileron_roll_left = vgesture_performed( player_ship.joypad, player_ship.aileron_swipe_left )
		local aileron_roll_right = vgesture_performed( player_ship.joypad, player_ship.aileron_swipe_right )
		
		if aileron_roll_left then
			ship_aileronRoll( ship, 1.0 )
		elseif aileron_roll_right then
			ship_aileronRoll( ship, -1.0 )
		end
	end

	local camera_roll = 0.0
	if ship.aileron_roll then
		-- strafe
		local roll_rate = ship_strafeRate( ship.aileron_roll_time / aileron_roll_duration )
		local strafe_speed = -1500.0 * roll_rate
		strafe = strafe_speed * dt * ship.aileron_roll_multiplier

		-- roll
		library.rolling_average.add( ship.target_roll, ship.aileron_roll_target )
		--local roll_delta = ship_rollDeltaFromTarget( library.rolling_average.sample( ship.target_roll ), ship.roll )
		local integral_total = 1.8 -- This is such a fudge - should be integral [0->1] of sin( pi^2 - x^2 / pi ) ( which is 1.58605 )
		local roll_delta = roll_rate * dt * ship.aileron_roll_amount * integral_total
		ship.roll = ship.roll + roll_delta

		ship.aileron_roll_time = ship.aileron_roll_time - dt
		ship.aileron_roll = ship_aileronRollActive( ship )
		camera_roll = max_allowed_roll * camera_roll_scale * ship.aileron_roll_multiplier
	else

		-- yaw
		ship.target_yaw = ship.target_yaw + yaw_delta
		local target_yaw_delta = ship.target_yaw - ship.yaw
		local max_yaw_delta = 2.0 * math.abs( ship.roll ) * 1.3 * dt
		yaw_delta = clamp( -max_yaw_delta, max_yaw_delta, target_yaw_delta )
		ship.yaw = ship.yaw + yaw_delta

		-- roll
		local target_roll = ship_rollFromYawRate( ship, target_yaw_delta )
		library.rolling_average.add( ship.target_roll, target_roll )
		local roll_delta = ship_rollDeltaFromTarget( library.rolling_average.sample( ship.target_roll ), ship.roll )
		ship.roll = ship.roll + roll_delta
		local roll_offset = library.modf( ship.roll + math.pi, 2 * math.pi ) - math.pi
		camera_roll = roll_offset * camera_roll_scale
	end
	
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

	playership_weaponsTick( ship, dt )

	-- Physics
	local forward_v = vtransformVector( ship.transform, Vector( 0.0, 0.0, ship.speed, 0.0 ))
	local strafe_v = vtransformVector( ship.camera_transform, Vector( strafe, 0.0, 0.0, 0.0 ))
	local world_v = vvector_add( forward_v, strafe_v )

	if ship.physic then
		vphysic_setVelocity( ship.physic, world_v )
	end
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

spawning_active = false

-- Called once per frame to update the current Lua State
function tick( dt )
	if starting then
		starting = false
		start()
	end

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

	if debug_doodads_enabled then
		update_doodads( player_ship.transform )
	end

	tick_array( turrets, dt )
	tick_array( interceptors, dt )
	tick_array( missiles, dt )
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



turret_cooldown = 0.4

function turret_fire( turret )
	-- right
	local muzzle_position = Vector( 4.6, 5.0, 5.0, 1.0 )
	fx.muzzle_flare_large( turret, muzzle_position )
	fire_missile( turret, muzzle_position, enemy_gunfire )
	-- left
	muzzle_position = Vector( -4.6, 5.0, 5.0, 1.0 )
	fx.muzzle_flare_large( turret, muzzle_position )
	fire_missile( turret, muzzle_position, enemy_gunfire )
end

function turret_tick( turret, dt )
	turret.behaviour = turret.behaviour( turret, dt )
end

function tick_array( arr, dt )
	for element in array.iterator( arr ) do
		if element.tick then
			element.tick( element, dt )
		end
	end
end

function turret_collisionHandler( target, collider )
	fx.spawn_explosion( target.transform )
	gameobject_destroy( target )
	target.behaviour = ai.dead
end

function spawn_atCanyon( u, v, model )
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y, z, 1.0 )
	local obj = gameobject_create( model )
	vtransform_setWorldPosition( obj.transform, position )
end

-- spawn properties
spawn_offset = 0.0
spawn_interval = 300.0
spawn_distance = 900.0
doodad_spawn_distance = 1500.0
despawn_distance = 100.0 -- how far behind to despawn units
-- spawn tracking
entities_spawned = 0.0
doodads_spawned = 0.0

-- Spawn all entities in the given range
function entities_spawnRange( near, far )
	local i = spawn_index( near ) + 1
	local spawn_v = i * spawn_interval
	while library.contains( spawn_v, near, far ) do
		local interceptor_offset_u = 20.0
		spawn.spawnGroup( spawn.spawnGroupForIndex( i ), spawn_v )
		i = i + 1
		spawn_v = i * spawn_interval
	end
end

function entities_despawnAll()
	for unit in array.iterator( interceptors ) do
		ship_delete( unit )
	end
end

function spawn_doodad( u, v, model )
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y, z, 1.0 )
	local doodad = gameobject_create( model )
	vtransform_setWorldPosition( doodad.transform, position )
end

function spawn_bunker( u, v, model )
	-- Try varying the v
	local highest = { x = 0, y = -10000, z = 0 }
	local radius = 100.0
	local step = radius / 5.0
	local i = v - radius
	while i < v + radius do
		local x,y,z = vcanyon_position( u , i )
		if y > highest.y then
			highest.x = x
			highest.y = y
			highest.z = z
		end
		i = i + step
	end
	local x,y,z = vcanyon_position( u , v )
	local position = Vector( x, y, z, 1.0 )
	local doodad = gameobject_create( model )
	vtransform_setWorldPosition( doodad.transform, position )
	return doodad
end

function doodads_spawnRange( near, far )
	local i = spawn_index( near ) + 1
	local spawn_v = i * spawn_interval
	while library.contains( spawn_v, near, far ) do
		local doodad_offset_u = 130.0
		local doodad = spawn_bunker( doodad_offset_u, spawn_v, "dat/model/borehole.s" )
		i = i + 1
		spawn_v = i * spawn_interval
	end
end

function update_doodads( transform )
	local pos = vtransform_getWorldPosition( transform )
	local u,v = vcanyon_fromWorld( pos )
	local spawn_up_to = v + doodad_spawn_distance
	doodads_spawnRange( doodads_spawned, spawn_up_to )
	doodads_spawned = spawn_up_to
end

-- Spawn all entities that need to be spawned this frame
function update_spawns( transform )
	local pos = vtransform_getWorldPosition( transform )
	local u,v = vcanyon_fromWorld( pos )
	local spawn_up_to = v + spawn_distance
	entities_spawnRange( entities_spawned, spawn_up_to )
	entities_spawned = spawn_up_to;
end

function update_despawns( transform ) 
	local pos = vtransform_getWorldPosition( transform )
	local u,v = vcanyon_fromWorld( pos )
	local despawn_up_to = v - despawn_distance

	for unit in array.iterator( interceptors ) do
		-- TODO remove them properly
		if unit.transform then
			unit_pos = vtransform_getWorldPosition( unit.transform )
			u,v = vcanyon_fromWorld( unit_pos )
			if v < despawn_up_to then
				ship_delete( unit )
				unit = nil
			end
		end
	end

	for unit in array.iterator( turrets ) do
		-- TODO remove them properly
		if unit.transform then
			unit_pos = vtransform_getWorldPosition( unit.transform )
			u,v = vcanyon_fromWorld( unit_pos )
			if v < despawn_up_to then
				ship_delete( unit )
				unit = nil
			end
		end
	end
end

function turret_state_inactive( turret, dt )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )

	if player_close then
		return turret_state_active
	else
		return turret_state_inactive
	end
end

function turret_state_active( turret, dt )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )

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

-- Interceptor Stats
interceptor_min_speed = 3.0
interceptor_speed = 160.0
interceptor_weapon_cooldown = 0.4
homing_missile_cooldown = 1.2

function interceptor_attack_gun( x, y, z )
	return function ( interceptor, dt )
		local facing_position = Vector( x, y, z, 1.0 )
		vtransform_facingWorld( interceptor.transform, facing_position )
		entities.setSpeed( interceptor, 0.0 )

		if interceptor.cooldown < 0.0 then
			interceptor_fire( interceptor )
			interceptor.cooldown = interceptor_weapon_cooldown
		end
		interceptor.cooldown = interceptor.cooldown - dt
	end
end

function interceptor_attack_homing( x, y, z )
	return function ( interceptor, dt )
		local facing_position = Vector( x, y, z, 1.0 )
		vtransform_facingWorld( interceptor.transform, facing_position )
		entities.setSpeed( interceptor, 0.0 )

		if interceptor.cooldown < 0.0 then
			interceptor_fire_homing( interceptor )
			interceptor.cooldown = homing_missile_cooldown
		end
		interceptor.cooldown = interceptor.cooldown - dt
	end
end

function interceptor_fire( interceptor )
	-- Right
	local muzzle_position = Vector( 1.2, 1.0, 1.0, 1.0 );
	fx.muzzle_flare_large( interceptor, muzzle_position )
	fire_missile( interceptor, muzzle_position, enemy_gunfire )
	-- Left
	muzzle_position	= Vector( -1.2, 1.0, 1.0, 1.0 );
	fx.muzzle_flare_large( interceptor, muzzle_position )
	fire_missile( interceptor, muzzle_position, enemy_gunfire )
end

function interceptor_fire_homing( interceptor )
	fire_enemy_homing_missile( interceptor, Vector( 0.0, 0.0, 0.0, 1.0 ), enemy_homing_missile )
end

function create_interceptor()
	local interceptor = gameobject_create( "dat/model/ship_new.s" )
	interceptor.cooldown = 0.0
	
	-- Init Collision
	vbody_registerCollisionCallback( interceptor.body, ship_collisionHandler )
	vbody_setLayers( interceptor.body, collision_layer_enemy )
	vbody_setCollidableLayers( interceptor.body, collision_layer_player )

	-- Activate
	interceptor.tick = interceptor_tick
	array.add( interceptors, interceptor )
	return interceptor
end

function interceptor_behaviour( interceptor, move_to, attack_target, attack_type )
	local enter = nil
	local attack = nil
	enter =		ai.state( entities.strafeTo( move_to.x, move_to.y, move_to.z, attack_target.x, move_to.y, attack_target.z ),		
							function () if entities.atPosition( interceptor, move_to.x, move_to.y, move_to.z, 5.0 ) then 
									return attack 
								else 
									return enter 
								end 
							end )

	attack =	ai.state( attack_type( attack_target.x, attack_target.y, attack_target.z ),						function () return attack end )
	return enter
end

function interceptor_tick( interceptor, dt )
	if interceptor.behaviour then
		interceptor.behaviour = interceptor.behaviour( interceptor, dt )
	end
end

