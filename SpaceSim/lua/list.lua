local list = {}

function list:fold( v, f )
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
	if t ~= nil and t.tail == nil and t.head == nil then
		l.tail = nil
	else
		l.tail = t
	end
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

function list:remove( entry )
	--return list:empty()
	--return list:filter( function ( e ) return (e ~= entry) end )
	return self:filter( function ( e ) return true end )
end

function list:filter( predicate )
	return self:fold( list:empty(), 
		function ( lst, item )
			return lst
			--[[
			if predicate( item ) then
				return list.cons( item, lst )
			else
				return lst
			end
				--]]
		end )
end

return list
