game = {}

-- Create a spacesim Game object
-- A gameobject has a visual representation (model), a physical entity for velocity and momentum (physic)
-- and a transform for locating it in space (transform)
game.create = ( model_file ) -> game.createAt( model_file, nil )

game.createAt = ( model_file, matrix ) ->
	g = {}
	g.model = vcreateModelInstance( model_file )
	g.physic = vcreatePhysic()
	g.transform = vcreateTransform()
	if matrix != nil then vtransform_setWorldSpaceByMatrix( g.transform, matrix )
	g.body = vcreateBodySphere( g )
	vmodel_setTransform( g.model, g.transform )
	vphysic_setTransform( g.physic, g.transform )
	vbody_setTransform( g.body, g.transform )
	vscene_addModel( scene, g.model )
	vphysic_activate( engine, g.physic )
	v = Vector( 0.0, 0.0, 0.0, 0.0 )
	vphysic_setVelocity( g.physic, v )
	g

game.destroy = ( g ) ->
	inTime( 0.2, () -> game.delete( g ))
	if g.body
		vdestroyBody( g.body )
		g.body = nil

game.delete = ( g ) ->
	if g.model then vdeleteModelInstance( g.model )
	if vtransform_valid(g.transform) then vdestroyTransform( scene, g.transform )
	if g.physic then vphysic_destroy( g.physic )
	if g.body then vdestroyBody( g.body )
	g.model = nil
	g.transform = nil
	g.physic = nil
	g.body = nil

game
