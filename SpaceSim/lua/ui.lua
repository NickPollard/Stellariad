local ui = {}

future = require "future"

function ui.show_splash( image, w, h )
	local color = Vector( 1.0, 1.0, 1.0, 1.0 )
	local centre_x = screen_width * 0.5
	local centre_y = screen_height * 0.5
	local splash = vuiPanel_create( engine, image, color, 
		centre_x - w * 0.5, centre_y - h * 0.5, 
		w, h )
	return splash
end

function ui.splash( image, w, h )
	local f = future:new()
	local color = Vector( 1.0, 1.0, 1.0, 1.0 )
	vprint( "width: " .. screen_width )
	vprint( "height: " .. screen_height )
	local centre_x = screen_width * 0.5
	local centre_y = screen_height * 0.5
	local splash = vuiPanel_create_future( engine, image, color, 
		centre_x - w * 0.5, centre_y - h * 0.5, 
		w, h, f)
	--f:complete( splash )
	return f
end

function ui.hide_splash( splash )
	vuiPanel_hide( engine, splash )
end

function ui.show_crosshair()
	-- Crosshair
	local w = 128
	local h = 128
	local x = (screen_width / 2) - ( w / 2 )
	local y = (screen_height / 2 ) - ( h / 2 ) - 40
	local color = Vector( 0.3, 0.6, 1.0, 0.8 )
	vuiPanel_create( engine, "dat/img/crosshair_arrows.tga", color, x, y, w, h )
end


return ui
