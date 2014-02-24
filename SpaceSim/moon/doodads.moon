-- Moonscript

--package.path = "./SpaceSim/lua/?.lua;./SpaceSim/lua/compiled/?.lua"
list = require "list"
option = require "option"

doodads = {}

-- config
doodads.spawn_distance = 900.0
doodads.random = vrand_newSeq()
doodads.interval = 30.0

all_doodads = list\empty()

trans_worldPos = ( t ) -> vtransform_getWorldPosition( t )

shouldDespawn = ( upto ) ->
	(doodad) ->
		vtransform_getWorldPosition( doodad.transform )\map( (p) ->
			unused,v = vcanyon_fromWorld( p )
			v < upto )\getOrElse true

--doodads.updateDespawns = ( t ) ->
	--trans_worldPos( t )\foreach( (p) ->
		--upto = (vcanyonV_atWorld p) - despawn_distance
		--toRemove = all_doodads\filter shouldDespawn(upto)
		--all_doodads = all_doodads\filter( (d) -> not toRemove\contains(d) )
		--toRemove\foreach doodads.delete)

-- TODO - this should be using some better kind of structure - spacial partitioning of some kind
doodads.updateDespawns = ( t ) ->
	trans_worldPos( t )\foreach( (p) ->
		upto = (vcanyonV_atWorld(canyon,p)) - despawn_distance
		all_doodads = all_doodads\filter( (d) ->
			if shouldDespawn(upto)(d) then
				doodads.delete(d)
				false
			else
				true))

doodads.spawnIndex = ( pos ) -> math.floor( (pos - spawn_offset) / doodads.interval )

doodads.create = ( model_file ) ->
	g = model: vcreateModelInstance( model_file ), transform: vcreateTransform()
	vmodel_setTransform( g.model, g.transform )
	vscene_addModel( scene, g.model )
	g

doodads.delete = ( g ) ->
	if g.model then
		vdeleteModelInstance( g.model )
		g.model = nil
	if vtransform_valid(g.transform) then
		vdestroyTransform( scene, g.transform )
		g.transform = nil

doodads.spawnDoodad = ( u, v, model ) ->
	x, y, z = vcanyon_position( u, v )
	doodad = doodads.create( model )
	vtransform_setWorldPosition( doodad.transform, Vector( x, y, z, 1.0 ))
	all_doodads = list\cons( doodad, all_doodads )

doodads.spawnBunker = ( u, v, model ) ->
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
	doodad = doodads.create( model )
	vtransform_setWorldPosition( doodad.transform, position )
	doodad



doodads.spawnTree = (u, v) -> doodads.spawnDoodad( u, v, "dat/model/tree_fir.s" )

doodads.spawnSkyscraper = (u, v) ->
	r = vrand( doodads.random, 0.0, 1.0 )
	doodad = switch r
		when r < 0.2 then option\some("dat/model/skyscraper_blocks.s")
		when r < 0.4 then option\some("dat/model/skyscraper_slant.s")
		when r < 0.6 then option\some("dat/model/skyscraper_towers.s")
		else option\none()
	doodad\foreach (d) -> doodads.spawnDoodad( u, v, d )
		
doodads.spawnRange = ( near, far ) ->
	nxt = doodads.spawnIndex( near ) + 1
	v = nxt * doodads.interval
	u_offset = 130.0
	while library.contains( v, near, far ) do
		doodads.spawnTree( u_offset, v )
		doodads.spawnTree( u_offset + 30.0, v )
		doodads.spawnTree( u_offset + 60.0, v )
		doodads.spawnTree( -u_offset, v )
		doodads.spawnTree( -u_offset - 30.0, v )
		doodads.spawnTree( -u_offset - 60.0, v )
		nxt += 1
		v = nxt * doodads.interval

doodads.spawned = 0.0

doodads.update = ( transform ) ->
	trans_worldPos( transform )\foreach( ( p ) ->
		spawn_upto = (vcanyonV_atWorld(canyon,p)) + doodads.spawn_distance
		doodads.spawnRange( doodads.spawned, spawn_upto )
		doodads.spawned = spawn_upto)

doodads
