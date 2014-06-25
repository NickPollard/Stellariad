local future = {}

-- require modules
	list = require "list"

function apply( func )
	func()
end

function applyTo( v )
	return function( func ) func( v ) end
end

function future:complete( v )
	self.value = v
	self.completed = true
	self.onCompleteHandlers:foreach( applyTo( v )) 
end

function future:onComplete( func )
	if self.completed then
		func( self.value )
	else
		self.onCompleteHandlers = list:cons( func, self.onCompleteHandlers )
	end
end

function future:map( func )
	local f = future:new()
	self:onComplete( function( arg ) f:complete( func( arg )) end )
	return f
end

function future:new()
	local f = { value = nil, completed = false, onCompleteHandlers = list:empty() }
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
