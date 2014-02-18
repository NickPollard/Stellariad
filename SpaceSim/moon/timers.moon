timers = {}

timers.timers = list\empty()

timers.add = (t) -> timers.timers = timers.timers\prepend( t )

timers.create = ( time, action ) ->
	t = time: time, action: action
	t

timers.tick = ( dt ) ->
	timers.timers\foreach( (t) ->
		t.time = t.time - dt
		if t.time < 0 then
			t.action())
	timers.timers = timers.timers\filter( (t) -> t.time >= 0 )

timers
