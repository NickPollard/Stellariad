local fx = {}

function fx.spawn_missile_explosion( position )
	local t = vcreateTransform()
	local explosion = vparticle_oneShot( engine, t, "dat/vfx/particles/missile_explosion.s" )
	local glow = vparticle_oneShot( engine, t, "dat/vfx/particles/missile_explosion_glow.s" )
	vtransform_setWorldSpaceByTransform( t, position )
	inTime( 1.5, function () 
		vdestroyTransform( scene, t ) 
	end )
end

function fx.spawn_explosion( position )
	local t = vcreateTransform()
	local explosion = vparticle_oneShot( engine, t, "dat/vfx/particles/explosion.s" )
	local explosion_b = vparticle_oneShot( engine, t, "dat/vfx/particles/explosion_b.s" )
	local explosion_c = vparticle_oneShot( engine, t, "dat/vfx/particles/explosion_c.s" )
	local glow = vparticle_oneShot( engine, t, "dat/vfx/particles/explosion_glow.s" )
	vtransform_setWorldSpaceByTransform( t, position )
	inTime( 1.5, function () 
		vdestroyTransform( scene, t ) 
	end )
end

function fx.muzzle_flare( ship, offset )
	local t = vcreateTransform( ship.transform )
	vtransform_setLocalPosition( t, offset )
	local flare = vparticle_oneShot( engine, t, "dat/vfx/particles/muzzle_flare_pent.s" )
	--local glow = vparticle_oneShot( engine, t, "dat/vfx/particles/muzzle_flare_glow.s" )
	inTime( 0.5, function () 
		vdestroyTransform( scene, t )
	end )
end

function fx.muzzle_flare_large( ship, offset )
	local t = vcreateTransform( ship.transform )
	vtransform_setLocalPosition( t, offset )
	local flare = vparticle_oneShot( engine, t, "dat/vfx/particles/muzzle_flare_pent_large.s" )
	--local glow = vparticle_oneShot( engine, t, "dat/vfx/particles/muzzle_flare_glow_large.s" )
	inTime( 0.5, function () 
		vdestroyTransform( scene, t )
	end )
end

function fx.preload_particle( particle_file )
	local t = vcreateTransform()
	local particle = vparticle_create( engine, t, particle_file )
	vparticle_destroy( particle )
	vdestroyTransform( scene, t )
end

function fx.preload()
	fx.preload_particle( "dat/vfx/particles/missile_explosion.s" )
	fx.preload_particle( "dat/vfx/particles/explosion.s" )
	fx.preload_particle( "dat/vfx/particles/explosion_b.s" )
	fx.preload_particle( "dat/vfx/particles/explosion_c.s" )
	fx.preload_particle( "dat/vfx/particles/explosion_glow.s" )
	fx.preload_particle( "dat/vfx/particles/bullet.s" )
	
	vmodel_preload( "dat/model/bullet_player.s" )
	vmodel_preload( "dat/model/missile_enemy_homing.s" )
end

return fx
