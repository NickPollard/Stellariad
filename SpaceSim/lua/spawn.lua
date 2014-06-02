local spawn = {}

spawn.random = 0 

spawn_max = 5

function spawn.init()
	spawn.random = vrand_newSeq()
end

function spawn.spawnSpacePositions( spawn_space )
	local positions = array.new()
	local x = -spawn_space.width
	while x <= spawn_space.width do
		local y = 0
		while y < spawn_space.height do
			local position = { x = x, y = y }
			array.add( positions, position )
			y = y + 1
		end
		x = x + 1
	end
	return positions
end

function spawn.availablePositions( positions, current_positions )
	local available_positions = array.filter( positions,
		function ( position )
			return not array.contains( current_positions, 
				function ( item )
					return item.x == position.x and item.y == position.y 
				end )
		end )
	return available_positions
end

function spawn.positionerTurret( spawn_space, current_positions )
	-- Pick the most central floor space
	local ranked_positions = array.rank( spawn_space.available_positions,
		function( position )
			return - math.abs( position.x ) - ( position.y ) * spawn_space.width
		end )
	array.add( current_positions, ranked_positions[1] )

	-- Update current positions
	local this_position = array.new( ranked_positions[1] )
	spawn_space.available_positions = spawn.availablePositions( spawn_space.available_positions, this_position )

	return current_positions
end

function spawn.positionerInterceptor( spawn_space, current_positions )
	-- Pick the tallest central space
	local ranked_positions = array.rank( spawn_space.available_positions,
		function( position )
			return - math.abs( position.x ) + ( position.y ) * spawn_space.width
		end )
	array.add( current_positions, ranked_positions[1] )

	-- Update current positions
	local this_position = array.new( ranked_positions[1] )
	spawn_space.available_positions = spawn.availablePositions( spawn_space.available_positions, this_position )

	return current_positions
end

function spawn.positionerStrafer( spawn_space, current_positions )
	-- Pick the tallest, widest space
	local ranked_positions = array.rank( spawn_space.available_positions,
		function( position )
			return math.abs( position.x ) + ( position.y ) * spawn_space.width
		end )
	array.add( current_positions, ranked_positions[1] )

	-- Update current positions
	local this_position = array.new( ranked_positions[1] )
	spawn_space.available_positions = spawn.availablePositions( spawn_space.available_positions, this_position )

	return current_positions
end

function spawn.positionerDefault( spawn_space, current_positions )
	local u_delta = 20.0
	local new_position = { u = current_positions[current_positions.count].u + u_delta, v = spawn_space.v }
	array.add( current_positions, new_position )

	-- Update current positions
	local this_position = array.new( new_position )
	spawn_space.available_positions = spawn.availablePositions( spawn_space.available_positions, this_position )

	return current_positions
end

function spawn.positionsForGroup( v, spawn_space, spawn_group_positioners )
	spawn_space.available_positions = spawn.spawnSpacePositions( spawn_space )
	local current_positions = array.new()
	local spawn_positions = array.foldl( spawn_group_positioners,
					function ( positioner, positions )
						local p = positioner( spawn_space, positions )
						return p
					end,
					current_positions )

	return spawn_positions
end

function spawn.spawnGroup( spawn_group, v )
	local spawn_space = { v = v, width = 3, height = 3, u_delta = 20.0, v_delta = 20.0, y_delta = 20.0 }
	local spawn_positions = spawn.positionsForGroup( v, spawn_space, spawn_group.positioners )

	local world_positions = array.map( spawn_positions, function( spawn_pos )
			local position = { u = spawn_pos.x * spawn_space.u_delta, v = v, y = spawn_pos.y * spawn_space.y_delta }
			return position
		end )

	array.zip( spawn_group.spawners, world_positions,	function ( func, arg )
															func( arg )
														end )
end

function spawn.spawnTurret( canyon, u, v )
	local spawn_height = 0.0

	-- position
	local x, y, z = vcanyon_position( canyon, u, v )
	local position = Vector( x, y + spawn_height, z, 1.0 )
	local turret = gameobject_create( "dat/model/gun_turret.s" )
	vtransform_setWorldPosition( turret.transform, position )

	-- Orientation
	local facing_x, facing_y, facing_z = vcanyon_position( canyon, u, v - 1.0 )
	local facing_position = Vector( facing_x, y + spawn_height, facing_z, 1.0 )
	vtransform_facingWorld( turret.transform, facing_position )

	-- Physics
	vbody_registerCollisionCallback( turret.body, turret_collisionHandler )
	vbody_setLayers( turret.body, collision_layer_enemy )
	vbody_setCollidableLayers( turret.body, collision_layer_player )

	turret.tick = turret_tick
	turret.cooldown = turret_cooldown

	-- TEMP tweak collision position
	turret.collision_transform = vcreateTransform( turret.transform )
	local position = Vector( 0.0, 6.0, 0.0, 1.0 )
	vtransform_setLocalPosition( turret.collision_transform, position )
	vbody_setTransform( turret.body, turret.collision_transform )

	-- ai
	turret.behaviour = ai.turret_state_inactive

	turrets.count = turrets.count + 1
	turrets[turrets.count] = turret
end

--[[
local interceptor_spawn_u_offset = -200
local interceptor_spawn_v_offset = -200
local interceptor_spawn_y_offset = 100
--]]
local interceptor_target_v_offset = 100

local interceptor_spawn_offset = {
	u = -200,
	v = -200,
	y = 100
}

local strafer_spawn_offset = {
	u = 200,
	v = 0,
	y = 30
}

-- Interceptor Stats
interceptor_min_speed = 3.0
interceptor_speed = 160.0
interceptor_weapon_cooldown = 0.4
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



local type_interceptor = {
	speed = 0.0,
	acceleration = 30.0,
	model = "dat/model/ship_green.s",
	attack_type = interceptor_attack_gun
}

local type_interceptor_missile = {
	speed = 0.0,
	acceleration = 30.0,
	model = "dat/model/ship_red.s",
	attack_type = interceptor_attack_homing
}

local type_strafer = {
	speed = 50.0,
	acceleration = 10.0,
	model = "dat/model/ship_red.s"
}

function spawn.triggeredSpawn( canyon, v, player_speed, func )
	local trigger_v = v + vrand( spawn.random, 0.0, 2.0 ) - ( 300.0 + player_speed * 5 )
	triggerWhen( function()
		return vtransform_getWorldPosition( player_ship.transform ):map( function( p )
			local unused, player_v = vcanyon_fromWorld( canyon, p )
			return player_v > trigger_v
		end ):getOrElse( false )
	end,
	func )
end

function spawn.spawnInterceptor( canyon, u, v, height, enemy_type )
	local mirror = vrand( spawn.random, 0.0, 1.0 ) > 0.5 and -1.0 or 1.0
	local spawn_x, spawn_y, spawn_z = vcanyon_position( canyon, u + mirror * interceptor_spawn_offset.u, v + interceptor_spawn_offset.v )
	spawn_y = spawn_y + interceptor_spawn_offset.y
	local spawn_position = Vector( spawn_x, spawn_y, spawn_z, 1.0 )
	local x, y, z = vcanyon_position( canyon, u, v + interceptor_target_v_offset )
	move_to = { x = x, y = y + height, z = z }

	local interceptor = spawn.create_interceptor( enemy_type )

	vtransform_setWorldPosition( interceptor.transform, spawn_position )
	local x, y, z = vcanyon_position( canyon, u, v + interceptor_target_v_offset - 100.0 )
	local attack_target = { x = x, y = move_to.y, z = z }
	local flee_to = { x = spawn_x, y = spawn_y + interceptor_spawn_offset.y * 2, z = spawn_z }
	interceptor.behaviour = ai.interceptor_behaviour_flee( interceptor, move_to, attack_target, enemy_type.attack_type, flee_to )
end

function spawn.spawnStrafer( canyon, u, v, height, enemy_type )
	local mirror = vrand( spawn.random, 0.0, 1.0 ) > 0.5 and -1.0 or 1.0
	local spawn_x, spawn_y, spawn_z = vcanyon_position( canyon, 0 + mirror * strafer_spawn_offset.u, v + strafer_spawn_offset.v )
	spawn_y = spawn_y + height
	local spawn_position = Vector( spawn_x, spawn_y, spawn_z, 1.0 )
	local move_x, move_y, move_z = vcanyon_position( canyon, 0 + mirror * -1.0 * strafer_spawn_offset.u, v + strafer_spawn_offset.v )
	move_y = move_y + height
	local move_to = { x = move_x, y = move_y, z = move_z }

	local strafer = spawn.create_interceptor( enemy_type )
	vtransform_setWorldPosition( strafer.transform, spawn_position )
	strafer.behaviour = ai.strafer_behaviour( strafer, move_to )
end

function spawn.spawnGroupForIndex( canyon, i, player_ship )
	return spawn.generateSpawnGroupForDifficulty( canyon, i, player_ship )
end



function spawn.randomEnemy( canyon, player_speed )
	local r = vrand( spawn.random, 0.0, 1.0 )
	if r > 0.8 then
		return function( coord ) spawn.triggeredSpawn( canyon, coord.v, player_speed, function() spawn.spawnStrafer( canyon, coord.u, coord.v, coord.y, type_strafer ) end) end, spawn.positionerStrafer
	elseif r > 0.5 then
		return function( coord ) spawn.triggeredSpawn( canyon, coord.v, player_speed, function() spawn.spawnInterceptor( canyon, coord.u, coord.v, coord.y, type_interceptor_missile ) end) end, spawn.positionerInterceptor
	elseif r > 0.0 then
		return function( coord ) spawn.triggeredSpawn( canyon, coord.v, player_speed, function() spawn.spawnInterceptor( canyon, coord.u, coord.v, coord.y, type_interceptor ) end) end, spawn.positionerInterceptor
	else
		return function( coord ) spawn.spawnTurret( canyon, coord.u, coord.v ) end, spawn.positionerTurret
	end
end

function spawn.generateSpawnGroupForDifficulty( canyon, difficulty, player_ship )
	-- For now, assume difficulty is number of units to spawn
	local count = math.min( difficulty, spawn_max )
	local group = {}
	group.spawners = { count = count }
	group.positioners = { count = count }
	local i = 1
	while i <= count do
		group.spawners[i], group.positioners[i] = spawn.randomEnemy( canyon, player_ship.speed )
		i = i + 1
	end
	return group
end

function spawn.create_interceptor( enemy_type )
	local interceptor = spawn.create_enemy( enemy_type )
	array.add( interceptors, interceptor )
	return interceptor
end

function spawn.create_enemy( enemy_type )
	local enemy = gameobject_create( enemy_type.model )
	
	-- Collision
	vbody_registerCollisionCallback( enemy.body, ship_collisionHandler )
	vbody_setLayers( enemy.body, collision_layer_enemy )
	vbody_setCollidableLayers( enemy.body, collision_layer_player )

	-- Movement
	enemy.speed = enemy_type.speed
	enemy.acceleration = enemy_type.acceleration

	-- Activate
	enemy.cooldown = 0.0
	enemy.tick = ai.tick

	return enemy
end

return spawn
