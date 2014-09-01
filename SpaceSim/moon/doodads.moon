-- Moonscript

--package.path = "./SpaceSim/lua/?.lua;./SpaceSim/lua/compiled/?.lua"
list = require "list"
option = require "option"
proc = require "proc"

doodads = {}

-- config
doodads.spawn_distance = 900.0
doodads.random = vrand_newSeq()
doodads.interval = 30.0

all_doodads = list\empty()

trans_worldPos = ( t ) -> vtransform_getWorldPosition( t )

shouldDespawn = ( canyon, upto ) ->
	(doodad) ->
		vtransform_getWorldPosition( doodad.transform )\map( (p) ->
			x,y,z,w = vvector_values(p)
			z < upto )\getOrElse true

-- TODO - this should be using some better kind of structure - spacial partitioning of some kind
doodads.updateDespawns = ( canyon, t ) ->
	if canyon != nil
		trans_worldPos( t )\foreach( (p) ->
			upto = (vcanyonV_atWorld(canyon,p)) - despawn_distance
			x,y,z = vcanyon_position( canyon, 0.0, upto )
			all_doodads = all_doodads\filter( (d) ->
				if shouldDespawn(canyon, z)(d) then
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

doodads.spawnDoodad = ( canyon, u, v, model ) ->
	x, y, z = vcanyon_position( canyon, u, v )
	doodad = doodads.create( model )
	vtransform_setWorldPosition( doodad.transform, Vector( x, y, z, 1.0 ))
	all_doodads = list\cons( doodad, all_doodads )

doodads.spawnDoodadY = ( canyon, u, v, y_off, model ) ->
	x, y, z = vcanyon_position( canyon, u, v )
	doodad = doodads.create( model )
	vtransform_setWorldPosition( doodad.transform, Vector( x, y + y_off, z, 1.0 ))
	all_doodads = list\cons( doodad, all_doodads )

doodads.spawnBunker = ( canyon, u, v, model ) ->
	-- Try varying the V
	highest = x: 0, y: -10000, z: 0
	radius = 100.0
	step = radius / 5.0
	i = v - radius
	while i < v + radius do
		x,y,z = vcanyon_position( canyon, u, i )
		if y > hightext.y then
			highest.x = x
			highest.y = y
			highest.z = z
		i += step
	x,y,z = vcanyon_position( canyon, u, v )
	position = Vector( x, y, z, 1.0 )
	doodad = doodads.create( model )
	vtransform_setWorldPosition( doodad.transform, position )
	doodad

doodads.spawnSkyscraper = (canyon, u, v) ->
	p = proc.skyscraper()
	p\zipWithIndex()\foreach( (p) ->
		model = p._1
		height = p._2 * 15.2
		--vprint(model .. " " .. height)
		doodads.spawnDoodadY(canyon, u, v, height, model ))

	--r = vrand( doodads.random, 0.0, 1.0 )
	--doodad = if r < 0.2 then option\some("dat/model/skyscraper_blocks.s")
			--elseif r < 0.4 then option\some("dat/model/skyscraper_slant.s")
			--elseif r < 0.6 then option\some("dat/model/skyscraper_towers.s")
			--else option\none()
	--doodad\foreach( (d) -> doodads.spawnDoodad( canyon, u, v, d ))
		
doodads.spawnRange = ( canyon, near, far ) ->
	nxt = doodads.spawnIndex( near ) + 1
	v = nxt * doodads.interval
	u_offset = 130.0
	model = "dat/model/tree_fir.s"
	while library.contains( v, near, far ) do
		doodads.spawnSkyscraper( canyon, u_offset, v )
		doodads.spawnSkyscraper( canyon, -u_offset, v )
		doodads.spawnSkyscraper( canyon, u_offset + 30.0, v)
		doodads.spawnSkyscraper( canyon, u_offset + 60.0, v)
		doodads.spawnSkyscraper( canyon, -u_offset - 30.0, v)
		doodads.spawnSkyscraper( canyon, -u_offset - 60.0, v)

		--doodads.spawnDoodad( canyon, u_offset, v, "dat/model/skyscraper_blocks.s" )
		--doodads.spawnDoodad( canyon, u_offset, v, model )
		--doodads.spawnDoodad( canyon, u_offset + 30.0, v, model )
		--doodads.spawnDoodad( canyon, u_offset + 60.0, v, model )
		--doodads.spawnDoodad( canyon, -u_offset, v, model )
		--doodads.spawnDoodad( canyon, -u_offset - 30.0, v, model )
		--doodads.spawnDoodad( canyon, -u_offset - 60.0, v, model )
		nxt += 1
		v = nxt * doodads.interval

doodads.spawned = 0.0

doodads.update = ( canyon, transform ) ->
	if canyon != nil
		trans_worldPos( transform )\foreach( ( p ) ->
			spawn_upto = (vcanyonV_atWorld(canyon,p)) + doodads.spawn_distance
			doodads.spawnRange( canyon, doodads.spawned, spawn_upto )
			doodads.spawned = spawn_upto)

-- return
doodads
