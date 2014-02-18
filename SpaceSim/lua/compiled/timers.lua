local timers = { }
timers.timers = list:empty()
timers.add = function(t)
  timers.timers = timers.timers:prepend(t)
end
timers.create = function(time, action)
  local t = {
    time = time,
    action = action
  }
  return t
end
timers.tick = function(dt)
  timers.timers:foreach(function(t)
    t.time = t.time - dt
    if t.time < 0 then
      return t.action()
    end
  end)
  timers.timers = timers.timers:filter(function(t)
    return t.time >= 0
  end)
end
return timers
