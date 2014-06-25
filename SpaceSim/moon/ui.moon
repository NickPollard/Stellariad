-- Moonscript
future = require "future"
color = require "color"

ui = {}

ui.show_splash = ( image, w, h ) -> ui.show_splash_withColor( image, w, h, color.white() )

ui.setPanelAlpha = ( p, a ) -> vuiPanel_setAlpha( p, a )
ui.panelFadeIn = ( p, t ) -> vuiPanel_fadeIn( p, t )
ui.panelFadeOut = ( p, t ) -> vuiPanel_fadeOut( p, t )

ui.show_splash_withColor = ( image, w, h, c ) ->
	centre = x: screen_width * 0.5, y: screen_height * 0.5
	vuiPanel_create( engine, image, c, centre.x - w * 0.5, centre.y - h * 0.5, w, h )

ui.splash = ( image, w, h ) ->
	f = future\new()
	centre = x: screen_width * 0.5, y: screen_height * 0.5
	vuiPanel_create_future( engine, image, color.white(), centre.x - w * 0.5, centre.y - h * 0.5, w, h, f )
	f

ui.hide_splash = ( s ) -> vuiPanel_hide( engine, s )
ui.show = ( s ) -> vuiPanel_show( engine, s )

ui.create_crosshair = () ->
	w = 128
	h = 128
	position = x: ( screen_width - w ) * 0.5, y: ( screen_height - h ) * 0.5
	crosshair = vuiPanel_create( engine, "dat/img/crosshair_arrows.tga", Vector( 0.3, 0.6, 1.0, 0.8 ), position.x, position.y, w, h )
	ui.hide_splash( crosshair )
	crosshair

ui.splash_intro = () ->
	vtexture_preload( "dat/img/splash_author.tga" )
	bg = ui.show_splash( "dat/img/black.tga", screen_width, screen_height )
	ui.studio_splash()\onComplete( (s) -> ui.author_splash()\onComplete( (a) -> ui.skies_splash()\onComplete( (a) ->
				ui.panelFadeOut( bg, 0.5 )
				ui.crosshair = ui.create_crosshair()
				gameplay_start())))

ui.skies_splash = () ->
	f = future\new()
	ui.splash( "dat/img/splash_skies_modern.tga", screen_width, screen_height )\onComplete( ( s ) ->
			ui.panelFadeIn( s, 2.0 )
			if debug_auto_start
				ui.hide_splash( s )
				f\complete( nil )
			touch_to_play = createTouchPad( input, 0, 0, screen_width, screen_height )
			touch_to_play\onTouch( ()->
				ui.hide_splash( s )
				f\complete( nil )
				removeTicker( touch_to_play )))
	f

ui.studio_splash = () ->
	f = future\new()
	ui.splash( "dat/img/splash_vitruvian.tga", 512, 256 )\onComplete( ( s ) ->
		ui.panelFadeIn( s, 1.5 )
		inTime( 2.0, () -> ui.panelFadeOut( s, 1.5 ))
		inTime( 3.5, () ->
			ui.hide_splash( s )
			f\complete( nil )))
	f

ui.author_splash = () ->
	f = future\new()
	ui.splash( "dat/img/splash_author.tga", 512, 256 )\onComplete( ( s ) ->
		ui.panelFadeIn( s, 1.5 )
		inTime( 2.0, () -> ui.panelFadeOut( s, 1.5 ))
		inTime( 3.5, () ->
			ui.hide_splash( s )
			f\complete( nil )))
	f

-- Return
ui
