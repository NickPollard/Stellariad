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

function list.fromArray( arr )
	local l = list:empty()
	local i = arr.count
	while i > 0 do
		l = list.cons( arr[i], l )	
		i = i - 1
	end
	return l
end

function list:take( count )
	if count > 0 then
		if self.tail then
			return list.cons( self.head, self.tail:take( count - 1 ))
		else
			return list.cons( self.head, list:empty())
		end
	else
		return list:empty()
	end
end

function list.test()
	local l = list.cons( 1, list.cons( 2, list.cons( 3, list.cons( 4, nil ))))
	l:foreach( function( n ) vprint( "n: " .. n ) end )
	local l_ = l:take(2)
	l_:foreach( function( n ) vprint( "n: " .. n ) end )

	local l__ = l:zipWithIndex()
	l__:foreach( function( n ) vprint( "ns: " .. n[1] .. ":" .. n[2] ) end )
end

function list:zipWith( l )
	local h = {}
	h[1] = self.head
	h[2] = l.head
	if self.tail and l.tail then
		return list.cons( h, self.tail:zipWith( l.tail ))
	else
		return list.cons( h, list:empty())
	end
end

function listOfNumbers( n )
	return numberListInternal( n, 1 )
end

function numberListInternal( n, i )
	if i <= n then
		return list.cons( i, numberListInternal( n, i+1 ))
	else
		return list:empty()
	end
end

function list:zipWithIndex()
	return self:zipWith( listOfNumbers( self:length() ))
end

return list
