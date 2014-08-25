-- Moonscript

list = require "list"
option = require "option"

proc = {}
proc.random = vrand_newSeq()

oneOf = (a, b) ->
	r = vrand( proc.random, 0.0, 1.0 )
	--vprint( "r" .. r )
	if r > 0.5 then
		--vprint("a")
		a
	else
		--vprint("b")
		b

spawner = ( s ) ->
	sp = {}
	sp.apply = s
	setmetatable(sp, proc.meta)
	sp

andSpawner = (spawnerA,spawnerB) -> spawner( () -> spawnerA()\append(spawnerB()))
orSpawner = (spawnerA,spawnerB) -> spawner( () ->
	--vprint( "or" )
	oneOf( spawnerA, spawnerB )())

proc.meta = {}
proc.meta.__pow = (a,b) ->
	--vprint( "pow" )
	orSpawner(a,b)
proc.meta.__call = (s) -> s.apply()
proc.meta.__concat = (a,b) ->
	--vprint( "concat" )
	andSpawner(a,b)


block = spawner(() -> list\cons("block.s", list\empty()))
empty = spawner(() -> list\empty())

local skyscraper
--skyscraper = spawner(() -> block()\append( (skyscraper .. empty)() ))

skyscraperS = spawner( () -> skyscraper()() )
skyscraper = () -> block .. (empty ^ skyscraperS)

proc.skyscraper = () ->
	skyscraper()()

-- return
proc
