-- Moonscript

--package.path = "./SpaceSim/lua/?.lua;./SpaceSim/lua/compiled/?.lua"
list = require "list"

all_doodads = list\empty()

trans_worldPos = ( t ) -> vtransform_getWorldPosition( t )

update_doodad_despawns = ( t ) ->
	trans_worldPos( t )\foreach( (p) ->
		despawn_upto = (vcanyonV_atWorld p) - despawn_distance
		toRemove = all_doodads\filter( shouldDespawn )
		toRemove\foreach( (d) -> gameobject_delete( d ))
		all_doodads = all_doodads\filter( () -> not shouldDespawn ))

doodad_spawnIndex = ( pos ) -> math.floor( (pos - spawn_offset) / doodads.interval )

spawnDoodad = ( u, v, model ) ->
	x, y, z = vcanyon_position( u, v )
	position = Vector( x, y, z, 1.0 )
	doodad = gameobject_create( model )
	vtransform_setWorldPosition( doodad.transform, position )
	all_doodads = list.cons( doodad, all_doodads )

spawnBunker = ( u, v, model ) ->
	-- Try varying the V
	highest = x: 0, y: -10000, z: 0
	radius = 100.0
	step = radius / 5.0
	i = v - radius
	while i < v + radius do
		x,y,z = vcanyon_position( u, i )
		if y > hightext.y then
			highest.x = x
			highest.y = y
			highest.z = z
		i += step
	x,y,z = vcanyon_position( u, v )
	position = Vector( x, y, z, 1.0 )
	doodad = gameobject_create( model )
	vtransform_setWorldPosition( doodad.transform, position )
	doodad



doodads_spawnTree = (u, v) -> spawnDoodad( u, v, "dat/model/tree_fir.s" )

doodads_spawnSkyscraper = (u, v) ->
	r = vrand( doodads.random, 0.0, 1.0 )
	switch r
		when r < 0.2 then spawnDoodad( u, v, "dat/model/skyscraper_blocks.s" )
		when r < 0.4 then spawnDoodad( u, v, "dat/model/skyscraper_slant.s" )
		when r < 0.6 then spawnDoodad( u, v, "dat/model/skyscraper_towers.s" )
		
doodads_spawnRange = ( near, far ) ->
	nxt = doodad_spawnIndex( near ) + 1
	v = nxt * doodads.interval
	u_offset = 130.0
	while library.contains( v, near, far ) do
		doodads_spawnTree( u_offset, v )
		doodads_spawnTree( u_offset + 30.0, v )
		doodads_spawnTree( u_offset + 60.0, v )
		doodads_spawnTree( -u_offset, v )
		doodads_spawnTree( -u_offset - 30.0, v )
		doodads_spawnTree( -u_offset - 60.0, v )
		nxt += 1
		v = nxt * doodads.interval

doodads_spawned = 0.0

update_doodads = ( transform ) ->
	trans_worldPos( transform )\foreach( ( p ) ->
		spawn_upto = (vcanyonV_atWorld p) + doodads_spawn_distance
		doodads_spawnRange( doodads_spawned, spawn_upto )
		doodads_spawned = spawn_upto)
