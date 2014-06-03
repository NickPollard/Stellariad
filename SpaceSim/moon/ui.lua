local future = require("future")
local color = require("color")
local ui = { }
ui.show_splash = function(image, w, h)
  return ui.show_splash_withColor(image, w, h, color.white())
end
ui.setPanelAlpha = function(p, a)
  return vuiPanel_setAlpha(p, a)
end
ui.panelFadeIn = function(p, t)
  return vuiPanel_fadeIn(p, t)
end
ui.panelFadeOut = function(p, t)
  return vuiPanel_fadeOut(p, t)
end
ui.show_splash_withColor = function(image, w, h, c)
  local centre = {
    x = screen_width * 0.5,
    y = screen_height * 0.5
  }
  return vuiPanel_create(engine, image, c, centre.x - w * 0.5, centre.y - h * 0.5, w, h)
end
ui.splash = function(image, w, h)
  vprint("Splash")
  local f = future:new()
  local centre = {
    x = screen_width * 0.5,
    y = screen_height * 0.5
  }
  vuiPanel_create_future(engine, image, color.white(), centre.x - w * 0.5, centre.y - h * 0.5, w, h, f)
  return f
end
ui.hide_splash = function(s)
  return vuiPanel_hide(engine, s)
end
ui.create_crosshair = function()
  local w = 128
  local h = 128
  local position = {
    x = (screen_width - w) * 0.5,
    y = (screen_height - h) * 0.5
  }
  local crosshair = vuiPanel_create(engine, "dat/img/crosshair_arrows.tga", Vector(0.3, 0.6, 1.0, 0.8), position.x, position.y, w, h)
  ui.hide_splash(crosshair)
  return crosshair
end
ui.splash_intro = function()
  vtexture_preload("dat/img/splash_author.tga")
  local bg = ui.show_splash("dat/img/black.tga", screen_width, screen_height)
  return ui.studio_splash():onComplete(function(s)
    return ui.author_splash():onComplete(function(a)
      return ui.skies_splash():onComplete(function(a)
        ui.panelFadeOut(bg, 0.5)
        ui.crosshair = ui.create_crosshair()
        return gameplay_start()
      end)
    end)
  end)
end
ui.skies_splash = function()
  local f = future:new()
  ui.splash("dat/img/splash_skies_modern.tga", screen_width, screen_height):onComplete(function(s)
    ui.panelFadeIn(s, 2.0)
    local touch_to_play = createTouchPad(input, 0, 0, screen_width, screen_height)
    return touch_to_play:onTouch(function()
      vprint("touch")
      ui.hide_splash(s)
      f:complete(nil)
      return removeTicker(touch_to_play)
    end)
  end)
  return f
end
ui.studio_splash = function()
  local f = future:new()
  ui.splash("dat/img/splash_vitruvian.tga", 512, 256):onComplete(function(s)
    ui.panelFadeIn(s, 1.5)
    inTime(2.0, function()
      return ui.panelFadeOut(s, 1.5)
    end)
    return inTime(3.5, function()
      ui.hide_splash(s)
      return f:complete(nil)
    end)
  end)
  return f
end
ui.author_splash = function()
  local f = future:new()
  ui.splash("dat/img/splash_author.tga", 512, 256):onComplete(function(s)
    ui.panelFadeIn(s, 1.5)
    inTime(2.0, function()
      return ui.panelFadeOut(s, 1.5)
    end)
    return inTime(3.5, function()
      ui.hide_splash(s)
      return f:complete(nil)
    end)
  end)
  return f
end
return ui
