-- SpaceSim main game lua script

--[[

This file contains the main game routines for the SpaceSim game.
Most game logic is written in Lua, whilst the core numerical processing (rendering, physics, animation etc.)
are handled by the Vitae engine in C.

Lua should be able to do everything C can, but where performance is necessary, code should be rewritten in
C and only controlled remotely by Lua

]]--

player = nil

-- Create a spacesim Game object
-- A gameobject has a visual representation (model), a physical entity for velocity and momentum (physic)
-- and a transform for locating it in space (transform)
function gameobject_create( model ) {
	local g = {}
	g.model = vitae_load_model( model )
	g.physic = vitae_create_physic()
	g.transform = vitae_create_transform()
	vitae_model_setTransform( g.model, g.transform )
	vitae_physic_setTransform( g.physic, g.transform )
	return g
}

function player_accelerate( p, a ) {
	vitae_physic_accelerate( p.physic, a )
}

function player_yaw( p, y ) {
	vitae_physic_yaw( p.physic, y )
}

function player_fire( p ) {

}

-- Create a player. The player is a specialised form of Gameobject
function player_create() {
	local p = game_object_create( "smoothsphere.obj" )
	vitae_register_keybind( "accelerate", "w", player_accelerate( p, acceleration ) )
	vitae_register_keybind( "decelerate", "s", player_accelerate( p, -acceleration ) )
	vitae_register_keybind( "yaw left", "a", player_yaw( p, yaw) )
	vitae_register_keybind( "yaw right", "d", player_yaw( p, -yaw) )

	vitae_register_keybind( "fire", "space", player_fire( p ) )
	return p
}


-- Set up the Lua State
function init() {
	-- We create a player object which is a game-specific Lua class
	-- The player class itself creates several native C classes in the engine
	player = player_create()
}

-- Called once per frame to update the current Lua State
function tick() {
}

-- Called on termination to clean up after itself
function terminate() {

	player = nil
}
