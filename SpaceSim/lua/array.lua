local array = {}

function array.add( arr, object )
	arr.count = arr.count + 1
	arr[arr.count] = object
end

function array.remove( arr, object )
	vprint( "Array remove " )
	--vprint( "Array remove " .. object )
	local i = 1
	local found = -1
	while i <= arr.count do
		--vprint( "[" .. i .. "]" .. arr[i] )
		if arr[i] == object then
			vprint( "found" )
			found = i
			break
		end
		i = i + 1
	end
	vprint( "end remove" )
	if found ~= -1 then
		vprint( "Removing " .. found .. " from array" )
		arr[i] = arr[arr.count]
		arr[arr.count] = nil
		arr.count = arr.count - 1
	end
end

function array.clear( arr )
	arr.count = 0
end

function array.iterator( t )
	local i = 0
	local n = t.count
	return function ()
		i = i + 1
		if i <= n then return t[i] end
	end
end

function array.filter( arr, func )
	new_arr = {}
	local count = 0
	for element in array.iterator( arr ) do
		if func( element ) then
			count = count + 1
			new_arr[count] = element
		end
	end
	new_arr.count = count
	return new_arr
end

function array.tail( arr )
	local new_arr = {}
	for i,v in ipairs( arr ) do
		if i > 1 then
			table.insert( new_arr, v )
		end
	end
	new_arr.count = arr.count - 1
	return new_arr
end

function array.head( arr )
	return arr[1]
end

function array.new( ... )
	local args = { ... }
	local arr = { count = 0 }
	for i,n in ipairs( args ) do
		arr.count = arr.count + 1
		arr[arr.count] = n
	end
	return arr
end

function array.foldr( arr, folder, final )
	if arr.count == 0 then
		return final
	elseif arr.count > 1 then
		return folder( array.head( arr ), array.foldr( array.tail( arr ), folder, final ))
	else
		return folder( array.head( arr ), final )
	end
end

function array.foldl( arr, folder, final )
	local arr_reversed = array.reverse( arr )
	return array.foldr( arr_reversed, folder, final )
end

function array.reverse( arr )
	local arr_reversed = { count = arr.count }
	local i = 0
	while i < arr.count + 1 do
		arr_reversed[i] = arr[arr.count - i + 1]
		i = i + 1
	end
	return arr_reversed
end

function array.map( arr, func )
	local new_arr = array.new()
	array.foldl( arr, 
		function ( item, arr )
			array.add( arr, func( item ))
			return arr
		end,
		new_arr )
	return new_arr
end

function array.sort( arr, sort_func )
	local sorted_arr = {}
	for i in array.iterator( arr ) do
		table.insert( sorted_arr, i )
	end
	table.sort( sorted_arr, sort_func )
	sorted_arr.count = arr.count
	return sorted_arr
end

function array.rank( arr, rank_func )
	local ranks = array.map( arr, function( item )
			local new_item = { item = item, score = rank_func( item ) }
			return new_item
		end )
	local sorted = array.sort( ranks, function ( a, b )
			return a.score > b.score
		end
	)
	return array.map( sorted, function( item )
			return item.item
		end )
end

function array.contains( arr, match_func )
	local result = array.foldr( arr, function( item, found )
			return match_func( item ) or found
		end,
		false )
	return result
end

function array.zip( a, b, func )
	local results = array.new()
	array.foldl( a, function ( item, other )
			array.add( results, func( item, array.head( other )))
			return array.tail( other )
		end,
		b )
	return results
end

function array.sum( arr )
	return array.foldr( arr, function( a, b )
		return a + b
	end,
	0 )
end

return array
