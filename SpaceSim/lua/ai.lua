local ai = {}

ai.count = 1

function ai.test( f )
	ai.count = ai.count + 1
	vprint( "count " .. ai.count )
	if ai.count > 10 then
		vprint( "##1" )
		return function ( x ) vprint( "End" ) return nil end
	else
		vprint( "##2" )
		return f
	end
end

--------------------------------------------------

function ai.dodge( )
	vprint( "dodge" )
end

function ai.shoot( )
	vprint( "shoot" )
end

function ai.state( tick, transition )
	return function ( entity, dt )
		tick( entity, dt )
		return transition()
	end
end

--[[
function ai.queue3( a, b, c )
	state_a = ai.state( a, function() return state_b end )
	state_b = ai.state( b, function() return state_c end )
	state_c = ai.state( c, function() return nil end )
	return state_a
end
--]]

function ai.queue( ... )
	return ai.queue_internal( arg )
end

function ai.queue_internal( args )
	vprint( "queue_internal" )
	local state = nil
	local next_state = nil
	if args.n > 0 then
		next_state = ai.queue_internal( array.tail( args )) 
		state = ai.state( args[1], function() return next_state end )
	else
		state = nil
	end
	return state
end

function ai.test_states()
	vprint( "### ai test states" )

	--[[
	func = ai.state_combinator( ai.dodge, ai.shoot )
	while true do
		func = func( func )
	end
	--]]

	--[[
	test_state = ai.queue3( function() vprint( "MoveTo" ) end, 
						function() vprint( "Attack" ) end, 
						function() vprint( "Exit" ) end )
						--]]

	test_state = ai.queue( function() vprint( "MoveTo" ) end, 
						function() vprint( "Attack" ) end, 
						function() vprint( "Defend" ) end, 
						function() vprint( "Exit" ) end )

	vprint( "queue_internal test" )
	i = 0
	while i < 10 do
		test_state = test_state()
		i = i + 1
	end
end

function ai.interceptor_behaviour_flee( interceptor, move_to, attack_target, attack_type, flee_to )
	local enter = nil
	local attack = nil
	local flee = nil
	local flee_distance = 100
	enter =	ai.state( entities.strafeTo( move_to.x, move_to.y, move_to.z, attack_target.x, move_to.y, attack_target.z ),		
							function () if entities.atPosition( interceptor, move_to.x, move_to.y, move_to.z, 5.0 ) then 
									return attack 
								else 
									return enter 
								end 
							end )

	attack = ai.state( attack_type( attack_target.x, attack_target.y, attack_target.z ),						
		function ()
			--[[
		local position = vtransform_getWorldPosition( player_ship.transform )
		local unused, player_v = vcanyon_fromWorld( position ) 
		position = vtransform_getWorldPosition( interceptor.transform )
		local unused, interceptor_v = vcanyon_fromWorld( position ) 
		if interceptor_v - player_v < flee_distance then return flee
		else return attack end 
		--]]
		return attack
		end )
	flee = ai.state( entities.strafeFrom( interceptor, flee_to.x, flee_to.y, flee_to.z ), function () return flee end )
	return enter
end

			--[[
function ai.interceptor_behaviour_flee( interceptor, move_to, attack_target, attack_type, flee_to )
	local enter = nil
	local attack = nil
	local flee = nil
	enter =	ai.state( entities.strafeTo( move_to.x, move_to.y, move_to.z, attack_target.x, move_to.y, attack_target.z ),		
		function () if entities.atPosition( interceptor, move_to.x, move_to.y, move_to.z, 5.0 ) then 
				return attack 
			else
				return enter 
			end
							end ) 
	attack = ai.state( attack_type( attack_target.x, attack_target.y, attack_target.z ),						
		function () 
			vtransform_getWorldPosition( player_ship.transform ):map( function( p )
				local unused, player_v = vcanyon_fromWorld( p ) 
				vtransform_getWorldPosition( interceptor.transform ):map( function( p_ )
					local unused, interceptor_v = vcanyon_fromWorld( p_ )  
					return interceptor_v - player_v < 100 and flee or attack
				end ):getOrElse( attack )
			end ):getOrElse( attack )
		end )

	flee = ai.state( entities.strafeFrom( interceptor, flee_to.x, flee_to.y, flee_to.z ), function () return flee end )
	return enter
end
--]]

function ai.strafer_behaviour( strafer, move_to )
	local strafe = nil
	strafe = ai.state( entities.strafeFrom( strafer, move_to.x, move_to.y, move_to.z ),
						function () return strafe end )
	return strafe
end

function ai.interceptor_behaviour( interceptor, move_to, attack_target, attack_type )
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

function ai.turret_state_inactive( turret, dt )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )

	if player_close then
		return ai.turret_state_active
	else
		return ai.turret_state_inactive
	end
end

function ai.turret_state_active( turret, dt )
	player_close = ( vtransform_distance( player_ship.transform, turret.transform ) < 200.0 )

	if turret.cooldown < 0.0 then
		turret_fire( turret )
		turret.cooldown = turret_cooldown
	end
	turret.cooldown = turret.cooldown - dt

	if player_close then
		return ai.turret_state_active
	else
		return ai.turret_state_inactive
	end
end

ai.dead = nil
ai.dead = ai.state( function () end, function () return ai.dead end )

function ai.tick( entity, dt )
	if entity.behaviour then
		entity.behaviour = entity.behaviour( entity, dt )
	end
end

return ai
