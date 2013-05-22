(model (mesh (filename "dat/model/ship_hd_2.obj" )
			(diffuse_texture "dat/img/ship_hd_green.tga" ))
		(transform (translation (vector 0.2 0.7 -2.1 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/script/lisp/engine_glow.s"))
					(particleEmitter (filename "dat/script/lisp/engine_trail.s")))
		(transform (translation (vector -0.2 0.7 -2.1 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/script/lisp/engine_glow.s"))
					(particleEmitter (filename "dat/script/lisp/engine_trail.s")))
		(transform (translation (vector 4.5 -0.1 -1.7 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/script/lisp/engine_glow.s"))
					(particleEmitter (filename "dat/script/lisp/engine_trail.s")))
		(transform (translation (vector -4.5 -0.1 -1.7 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/script/lisp/engine_glow.s"))
					(particleEmitter (filename "dat/script/lisp/engine_trail.s")))
)
