local fx = {}

function fx.spawn_missile_explosion( position )
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/vfx/particles/missile_explosion.s" )
	vparticle_create( engine, t, "dat/vfx/particles/missile_explosion_glow.s" )
	vtransform_setWorldSpaceByTransform( t, position )
end

function fx.spawn_explosion( position )
	local t = vcreateTransform()
	vparticle_create( engine, t, "dat/script/lisp/explosion.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_b.s" )
	vparticle_create( engine, t, "dat/script/lisp/explosion_c.s" )
	vtransform_setWorldSpaceByTransform( t, position )
end

function fx.muzzle_flare( ship, offset )
	local t = vcreateTransform( ship.transform )
	vtransform_setLocalPosition( t, offset )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare.s" )
	vparticle_create( engine, t, "dat/vfx/particles/muzzle_flare_glow.s" )
end

return fx
