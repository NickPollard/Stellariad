local fx = {}

function fx.spawn_missile_explosion( position )
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/vfx/particles/missile_explosion.s" )
	vparticle_create( engine, t, "dat/vfx/particles/missile_explosion_glow.s" )
	vtransform_setWorldSpaceByTransform( t, position )
	inTime( 5.0, function () vdestroyTransform( scene, t ) end )
end

function fx.spawn_explosion( position )
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/script/lisp/explosion.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_b.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_c.s" )
	vtransform_setWorldSpaceByTransform( t, position )
	inTime( 5.0, function () vdestroyTransform( scene, t ) end )
end

function fx.muzzle_flare( ship, offset )
	local t = vcreateTransform( ship.transform )
	vtransform_setLocalPosition( t, offset )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare.s" )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare_glow.s" )
	inTime( 2.0, function () vdestroyTransform( scene, t ) end )
end

function fx.muzzle_flare_large( ship, offset )
	local t = vcreateTransform( ship.transform )
	vtransform_setLocalPosition( t, offset )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare_large.s" )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare_glow_large.s" )
	inTime( 2.0, function () vdestroyTransform( scene, t ) end )
end

return fx
