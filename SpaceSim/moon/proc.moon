-- Moonscript

list = require "list"
option = require "option"

proc = {}
proc.random = vrand_newSeq()

oneOf = (a, b) ->
	r = vrand( proc.random, 0.0, 1.0 )
	if r > 0.5 then a else b

spawner = ( s ) ->
	sp = {}
	sp.apply = s
	setmetatable(sp, proc.meta)
	sp

andSpawner = (spawnerA,spawnerB) -> spawner( () -> spawnerA()\append(spawnerB()))
orSpawner = (spawnerA,spawnerB) -> spawner( () -> oneOf( spawnerA, spawnerB )())

proc.meta = {}
proc.meta.__pow = (a,b) -> orSpawner(a,b)
proc.meta.__call = (s) -> s.apply()
proc.meta.__concat = (a,b) -> andSpawner(a,b)

model = (m) -> spawner(() -> list(m))
s = (spwn) -> spawner( () -> spwn()())

-----------------------------------

--block = model("dat/model/block4.s") ^ model("dat/model/block3.s")
--smallblock = model("dat/model/smallblock.s")
--pyramid = model("dat/model/edge.s") ^ model("dat/model/pyramid2.s")

block = model("dat/model/block3.s") ^ model("dat/model/block3.s")
smallblock = model("dat/model/block3.s")
pyramid = model("dat/model/block3.s") ^ model("dat/model/block3.s")
empty = spawner(() -> list.empty())

------------------------------------

spire = () -> empty ^ (smallblock .. s(spire))
--skyscraper = () -> (block .. s(spire)) ^ (block .. s(skyscraper)) ^ (block .. block .. pyramid) ^ empty
skyscraper = () -> (block)

proc.skyscraper = () -> skyscraper()()

-- return
proc
