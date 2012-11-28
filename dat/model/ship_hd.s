(model	(object_process (meshLoadFile		"dat/model/ship_hd_2.obj" )
			 (quote (attribute "diffuse_texture" "dat/img/ship_hd_blue.tga")))
		(transform	
			(quote ((attribute "translation" (vector 0.2 0.7 -2.1 1.0))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_glow.s"))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_trail.s")))))
		(transform	(quote ((attribute "translation" (vector -0.2 0.7 -2.1 1.0))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_glow.s"))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_trail.s")))))
		(transform	(quote ((attribute "translation" (vector 4.5 -0.1 -1.7 1.0))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_glow.s"))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_trail.s")))))
		(transform	(quote ((attribute "translation" (vector -4.5 -0.1 -1.7 1.0))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_glow.s"))
					(attribute "particle" (particleLoad "dat/script/lisp/engine_trail.s")))))
)
