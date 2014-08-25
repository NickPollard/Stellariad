-- Moonscript

list = require "list"
option = require "option"

proc = {}
proc.random = vrand_newSeq()

oneOf = (a, b) ->
	r = vrand( proc.random, 0.0, 1.0 )
	if r > 0.5 then a else b

proc.meta = {}
proc.meta.__concat = oneOf
proc.meta.__call = (s) -> s.apply()

spawner = ( s ) ->
	sp = {}
	sp.apply = s
	setmetatable(sp, proc.meta)
	sp

block = spawner(() -> list\cons("block.s", list\empty()))
empty = spawner(() -> list\empty())

local skyscraper
skyscraper = spawner(() -> block()\append( (skyscraper .. empty)() ))

--skyscraper = spawner(() -> block ^ (skyscraper .. empty))

proc.skyscraper = () -> skyscraper()

-- return
proc
