(model (mesh (filename "dat/model/warthog.obj" )
			#(diffuse_texture "dat/img/ship_warthog.tga" )
			(diffuse_texture "dat/img/metal_simple.tga" )
				(shader "reflective")
				)
		(transform (translation (vector 1.6 1.7 -2.5 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_glow.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_trail.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_blur.s")))
		(transform (translation (vector -1.6 1.7 -2.5 1.0 ))
					(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_glow.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_trail.s"))
					(particleEmitter (filename "dat/vfx/particles/engine_blur.s")))
		#(transform (translation (vector 0.0 -2.0 -3.1 1.0 ))
					#(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					#(particleEmitter (filename "dat/vfx/particles/engine_glow.s"))
					#(particleEmitter (filename "dat/vfx/particles/engine_trail.s")))
		#(transform (translation (vector -4.5 -0.1 -1.7 1.0 ))
					#(ribbonEmitter (filename "dat/vfx/ribbons/ribbon_engine_trail.s"))
					#(particleEmitter (filename "dat/vfx/particles/engine_glow.s"))
					#(particleEmitter (filename "dat/vfx/particles/engine_trail.s")))
)