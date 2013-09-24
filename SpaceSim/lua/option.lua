local option = {}

function option:some( v )
	local o = { isSome = true, value = v }
	setmetatable(o, self)
	self.__index = self
	return o
end

function option:none()
	local o = { isSome = false, value = nil }
	setmetatable(o, self)
	self.__index = self
	return o
end

function option:map( f )
	if self.isSome then
		return option:some( f( self:get() ))
	else
		return option:none()
	end
end

function option:foreach( f )
	if self.isSome == true then
		f( self:get() )
	end
end

function option:get()
	if not self.isSome then
		vprint( "Error: trying to Get from a None Option" )
		vassert( false )
	end
	return self.value
end

function option:orElse( b )
	if self.isSome then
		return self
	else 
		return b
	end
end

function option:getOrElse( b )
	if self.some then
		return self:get()
	else
		return b
	end
end

return option
