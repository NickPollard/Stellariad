-- SpaceSim main game lua script

--[[

This file contains the main game routines for the SpaceSim game.
Most game logic is written in Lua, whilst the core numerical processing (rendering, physics, animation etc.)
are handled by the Vitae engine in C.

Lua should be able to do everything C can, but where performance is necessary, code should be rewritten in
C and only controlled remotely by Lua

]]--

	pi = math.pi
	two_pi = 2.0 * pi

	local z_axis = Vector( 0.0, 0.0, 1.0, 0.0 )

-- Debug settings
	debug_spawning_disabled	= true
	debug_doodads_disabled	= false
	debug_player_immortal		= true
	debug_player_autofly		= true
	debug_player_immobile		= true
	debug_auto_start				= true
	debug_no_pause_fade			= true

-- Load Modules
	--package.path = "./SpaceSim/lua/?.lua;./SpaceSim/lua/compiled/?.lua"
	ai				= require "ai"
	array			= require "array"
	doodads		= require "doodads"
	entities	= require "entities"
	fx				= require "fx"
	future		= require "future"
	game			= require "game"
	library		= require "library"
	list			= require "list"
	option		= require "option"
	playr			= require "player"
	spawn			= require "spawn"
	timers		= require "timers"
	triggers	= require "triggers"
	ui				= require "ui"

-- player - this object contains general data about the player
	plyr = nil

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
	player_ship_model = "dat/model/ship_warthog.s"

	-- Weapons
	player_gun_cooldown		= 0.15
	player_missile_cooldown	= 1.0
	homing_missile_turn_rps = pi / 2
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

-- spawn properties
	spawn_offset = 0.0
	spawn_interval = 300.0
	spawn_distance = 900.0
	despawn_distance = 100.0			-- how far behind to despawn units
-- spawn tracking
	entities_spawned = 0.0

	tickers = list:empty() 

local spawning_active = false

function debugLocals( )
  local idx = 1
  while true do
    local ln, lv = debug.getlocal(2, idx)
    if ln ~= nil then
      --variables[ln] = lv
			vprint(ln .. lv)
    else
      break
    end
    idx = 1 + idx
  end
end

function safeCleanup( value, cleanup )
	if value then
		cleanup()
	end
	return nil
end

function setCollision_playerBullet( object )
	vbody_setLayers( object.body, collision_layer_bullet )
	vbody_setCollidableLayers( object.body, bitwiseOR( collision_layer_enemy, collision_layer_terrain ))
end

function setCollision_enemyBullet( object )
	vbody_setLayers( object.body, collision_layer_bullet )
	vbody_setCollidableLayers( object.body, bitwiseOR( collision_layer_player, collision_layer_terrain ))
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
	trail = "dat/model/missile_enemy_homing_trail.s",
	particle = "dat/script/lisp/red_bullet.s",
	speed = 100.0,
	collisionType = "player",
	time_to_live = 6.0
}

enemy_gunfire = { 
	model = "dat/model/bullet_player.s",
	speed = 150.0,
	collisionType = "enemy",
	time_to_live = 2.0
}

enemy_homing_missile = { 
	model = "dat/model/missile_enemy_homing.s",
	trail = "dat/model/missile_enemy_homing_trail.s",
	speed = 100.0,
	collisionType = "enemy",
	time_to_live = 3.0
}

function inTime( time, action )
	timers.add( timers.create( time, action ))
end

function triggerWhen( trigger, action )
	triggers.add( triggers.create( trigger, action ))
end
starting = true

-- Set up the Lua State
function init()
	vprint( "init" )
	spawn.init()

	starting = true
	--color = Vector( 1.0, 1.0, 1.0, 1.0 )
	--local vignette = vuiPanel_create( engine, "dat/img/vignette.tga", color, 0, 360, screen_width, 360 )
	
	ui.splash_intro()
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
	setup_debug_controls()
end

function setup_debug_controls()
	--[[
	if touch_enabled then
		bloom_toggle = vcreateTouchPad( input, 0, 0, 150, 150 )
		local color = Vector( 0.15, 0.15, 0.15, 0.3 )
		local bloom_display = vuiPanel_create( engine, "dat/img/white.tga", color, 0, screen_height - 150, 150, 150 )
	end
	--]]
end

local restartFrame = nil


function gameplay_start()
	player_active = true
	inTime( 2.0, function () 
		player_ship.speed = player_ship_initial_speed
		ui.show( ui.crosshair )
		ui.panelFadeIn( ui.crosshair, 0.5 )
		--playership_addEngineGlows( player_ship )
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
	-- We create a player object which is a game-specific Lua class
	-- The player class itself creates several native C classes in the engine
	playr.ship_cleanup( player_ship )
	player_ship = playr.ship_create()

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

local touchpad = {}
function touchpad:new()
	local p = { pad = nil, onTouchHandlers = list:empty() }
	setmetatable(p, self)
	self.__index = self
	return p
end

function touchpad:onTouch( func )
	self.onTouchHandlers = list:cons( func, self.onTouchHandlers )
end

function touchpad:tick( dt )
	touched, x, y = vtouchPadTouched( player_ship.joypad )
	if not touch_enabled then
		touched = vkeyPressed( input, key.space )
	end
	if touched then
		self.onTouchHandlers:foreach( apply )
	end
end

function createTouchPad( input, x, y, w, h )
	local pad = touchpad:new()
	pad.pad = vcreateTouchPad( input, x, y, w, h )
	addTicker( pad )
	return pad
end

function test()
	--future.test()
	--list.test()
end

canyon = nil

function start()
	fx.preload()
	canyon = vcanyon_create(engine, scene)

	test()

	restart()
end

wave_interval_time = 10.0

function sign( x )
	if x > 0 then return 1.0 else return -1.0 end
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

function steering_input_keyboard()
	local yaw = 0.0
	local pitch = 0.0
	-- Steering
	if vkeyHeld( input, key.left ) then yaw = -1.0 end
	if vkeyHeld( input, key.right ) then yaw = 1.0 end
	if vkeyHeld( input, key.up ) then pitch = -1.0 end
	if vkeyHeld( input, key.down ) then pitch = 1.0 end
	return yaw, pitch
end

function clamp( min, max, value )
	return math.min( max, math.max( min, value ))
end

function lerp( a, b, k )
	return a + ( b - a ) * k
end

function debug_tick( dt )
	if vkeyPressed( input, key.c ) then toggle_camera() end
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

local paused = false
local pauseFrame = nil

-- Called once per frame to update the current Lua State
function tick( dt )
	if starting then
		starting = false
		start()
	end

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


	if player_active then playr.ship_tick( player_ship, dt ) end

	debug_tick( dt )

	timers.tick( dt )
	triggers.tick( dt )

	if spawning_active then
		update_spawns( canyon, player_ship.transform )
		update_despawns( canyon, player_ship.transform )
	end

	if not debug_doodads_disabled then
		doodads.update( canyon, player_ship.transform )
		doodads.updateDespawns( canyon, player_ship.transform )
	end

	tick_array( turrets, dt )
	tick_array( interceptors, dt )
	tick_array( missiles, dt )

	tickers:foreach( function( ticker )
		ticker:tick( dt )
	end )
end

function addTicker( ticker )
	tickers = tickers:prepend(ticker)
end

function removeTicker( ticker )
	tickers = tickers:remove( ticker )
end

-- Called on termination to clean up after itself
function terminate()
	plyr = nil
end

function tick_array( arr, dt )
	for element in array.iterator( arr ) do
		if element.tick then
			element.tick( element, dt )
		end
	end
end

local keyHandlers = { count = 0 }

function onKeyPress( input, key )
	local f = future:new()
	triggerWhen( function() return vkeyPressed( input, key ) end, function() f:complete( nil ) end )
	return f
end
