local list = {}

function list:fold( v, f )
	local len = self:length()
	if self.tail then
		return self.tail:fold( f( v, self.head ), f )
	else
		if self.head then
			return f( v, self.head )
		else
			return nil
		end
	end
end

function list:map( f )
	self:fold( list:empty(), function( l, v ) 
		list.cons( l, f( v ))
	end )
end

function list:foreach( f )
	self:fold( list:empty(), function( l, v ) 
		f( v )
		return nil 
	end )
end

function list:empty()
	local l = { head = nil, tail = nil }
	setmetatable(l, self)
	self.__index = self
	return l
end

function list.cons( h, t )
	local l = list:empty()
	l.head = h
	l.tail = t
	return l
end

function list:isEmpty( )
	return not self.head and not self.tail
end

function list:length( )
	if self.tail then 
		return 1 + list.length( self.tail) else 
		return 1 end
end

return list
