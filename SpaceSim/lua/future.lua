local future = {}

-- require modules
	list = require "list"

function apply( f )
	inTime( 3.0, function () player_ship.speed = 30.0 end )
	f()
end

function applyTo( v )
	return function( func ) func( v ) end
end

function future:complete( v )
	self.value = v
	self.onCompleteHandlers:foreach( applyTo( v )) 
end

function future:onComplete( func )
	vprint( "onComplete" )
	if self.value then
		func( self.value )
	else
		self.onCompleteHandlers = list.cons( func, self.onCompleteHandlers )
	end
end

function future:map( func )
	local f = future:new()
	self:onComplete( function( arg ) f:complete( func( arg )) end )
	return f
end

function future:new()
	local f = { value = nil, onCompleteHandlers = list:empty() }
	setmetatable(f, self)
	self.__index = self
	return f
end

function future:andThen( func )
	return self:map( func )
end

function future.test()
	local f = future:new()
	vprint( "Begin future test." )
	f:onComplete( function( args ) vprint( "onComplete" ) end )
	vprint("complete")
	f:complete( nil )

	local f_a = future:new()
	f_a:andThen( function( args ) vprint( "First" ) end ):andThen( function( args ) vprint( "Second" ) end ):andThen( function( args ) vprint( "Third" ) end )
	f_a:complete( nil )
end

return future
