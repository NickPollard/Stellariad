-- Moonscript
future = require "future"
color = require "color"

ui = {}

ui.show_splash = ( image, w, h ) -> ui.show_splash_withColor( image, w, h, color.white() )

ui.setPanelAlpha = ( p, a ) -> vuiPanel_setAlpha( p, a )
ui.panelFadeIn = ( p, t ) -> vuiPanel_fadeIn( p, t )

ui.show_splash_withColor = ( image, w, h, c ) ->
	centre = x: screen_width * 0.5, y: screen_height * 0.5
	vuiPanel_create( engine, image, c, centre.x - w * 0.5, centre.y - h * 0.5, w, h )

ui.splash = ( image, w, h ) ->
	vprint( "Splash" )
	f = future\new()
	centre = x: screen_width * 0.5, y: screen_height * 0.5
	vuiPanel_create_future( engine, image, color.white(), centre.x - w * 0.5, centre.y - h * 0.5, w, h, f )
	return f

ui.hide_splash = ( s ) -> vuiPanel_hide( engine, s )

ui.show_crosshair = () ->
	w = 128
	h = 128
	position = x: ( screen_width - w ) * 0.5, y: ( screen_height - h ) * 0.5
	vuiPanel_create( engine, "dat/img/crosshair_arrows.tga", Vector( 0.3, 0.6, 1.0, 0.8 ), position.x, position.y, w, h )

-- Return
ui
