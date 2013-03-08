local ui = {}

future = require "future"

function ui.show_splash( image, w, h )
	local color = Vector( 1.0, 1.0, 1.0, 1.0 )
	local screen_w = 1280
	local screen_h = 720
	local centre_x = screen_w * 0.5
	local centre_y = screen_h * 0.5
	local splash = vuiPanel_create( engine, image, color, 
		centre_x - w * 0.5, centre_y - h * 0.5, 
		w, h )
	return splash
end

function ui.show_splash_future( image, w, h )
	local f = future:new()
	local splash = ui.show_splash( image, w, h )
	f:complete( splash )
	return f
end

function ui.hide_splash( splash )
	vuiPanel_hide( engine, splash )
end

function ui.show_crosshair()
	-- Crosshair
	local w = 128
	local h = 128
	local x = 640 - ( w / 2 )
	local y = 360 - ( h / 2 ) - 40
	local color = Vector( 0.3, 0.6, 1.0, 0.8 )
	vuiPanel_create( engine, "dat/img/crosshair_arrows.tga", color, x, y, w, h )
end


return ui
