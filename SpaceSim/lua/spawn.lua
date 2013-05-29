local spawn = {}

spawn.random = 0 

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
	-- Pick the most tall central space
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

function spawn.spawnTurret( u, v )
	local spawn_height = 0.0

	-- position
	local x, y, z = vcanyon_position( u, v )
	local position = Vector( x, y + spawn_height, z, 1.0 )
	local turret = gameobject_create( "dat/model/gun_turret.s" )
	vtransform_setWorldPosition( turret.transform, position )

	-- Orientation
	local facing_x, facing_y, facing_z = vcanyon_position( u, v - 1.0 )
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
	turret.behaviour = turret_state_inactive

	turrets.count = turrets.count + 1
	turrets[turrets.count] = turret
end

local interceptor_spawn_u_offset = -200
local interceptor_spawn_v_offset = -200
local interceptor_spawn_y_offset = 100
local interceptor_target_v_offset = 100

function spawn.spawnInterceptor( u, v, height, player_speed, model, attack_type )
	local trigger_v = v - ( 300.0 + player_speed * 5 )
	triggerWhen( function()
			local position = vtransform_getWorldPosition( player_ship.transform )
			local unused, player_v = vcanyon_fromWorld( position ) 
			return player_v > trigger_v
		end,
		function()
			local spawn_x, spawn_y, spawn_z = vcanyon_position( u + interceptor_spawn_u_offset, v + interceptor_spawn_v_offset )
			local spawn_position = Vector( spawn_x, spawn_y + interceptor_spawn_y_offset, spawn_z, 1.0 )
			local x, y, z = vcanyon_position( u, v + interceptor_target_v_offset )
			move_to = { x = x, y = y + height, z = z }
	
			local interceptor = create_interceptor( model )
	
			vtransform_setWorldPosition( interceptor.transform, spawn_position )
			local x, y, z = vcanyon_position( u, v + interceptor_target_v_offset - 100.0 )
			local attack_target = { x = x, y = move_to.y, z = z }
			interceptor.behaviour = interceptor_behaviour( interceptor, move_to, attack_target, attack_type )
		end
		)
end

function spawn.spawnGroupForIndex( i, player_ship )
	return spawn.generateSpawnGroupForDifficulty( i, player_ship )
end


function spawn.randomEnemy( player_speed )
	local r = vrand( spawn.random, 0.0, 1.0 )
	if r > 0.75 then
		return function( coord ) spawn.spawnInterceptor( coord.u, coord.v, coord.y, player_speed, "dat/model/ship_red.s", interceptor_attack_homing ) end, spawn.positionerInterceptor
	elseif r > 0.0 then
		return function( coord ) spawn.spawnInterceptor( coord.u, coord.v, coord.y, player_speed, "dat/model/ship_green.s", interceptor_attack_gun ) end, spawn.positionerInterceptor
	else
		return function( coord ) spawn.spawnTurret( coord.u, coord.v ) end, spawn.positionerTurret
	end
end

function spawn.generateSpawnGroupForDifficulty( difficulty, player_ship )
	-- For now, assume difficulty is number of units to spawn
	local count = math.min( difficulty, 6 )
	local group = {}
	group.spawners = { count = count }
	group.positioners = { count = count }
	local i = 1
	while i <= count do
		group.spawners[i], group.positioners[i] = spawn.randomEnemy( player_ship.speed )
		i = i + 1
	end
	return group
end

return spawn
