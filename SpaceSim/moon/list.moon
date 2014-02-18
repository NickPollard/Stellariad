list = {}
list.__index = list
list.empty_ = head: nil, tail: nil
setmetatable(list.empty_, list)

list.fold = ( v, f ) =>
	if self.tail and self.tail != list.empty_ then
		if self.head then
			self.tail\fold( f( v, self.head ), f)
		else
			self.tail\fold( v, f )
	else
		if self.head then f( v, self.head) else v

list.map = ( f ) => self\fold( list\empty(), ( l, v ) -> list:cons( l, f( v )))

list.foreach = ( f ) => self\fold( nil, ( l, v ) -> f( v ) )

list.empty = () => list.empty_

list.cons = ( h, t ) =>
	l = head: h, tail: t
	setmetatable( l, self )
	l

list.isEmpty = () => self == list.empty_

list.length = () =>
	if selftail and self.tail != list.empty_ then 1 + self.tail\length() else 1

list.remove = (r) => list\filter( (el) -> el != r )

list.filter = (p) => self\fold( list\empty(), (lst,item) -> if p( item ) then list\cons(item,lst) else lst )

list.fromArray = ( arr ) ->
	l = list\empty()
	i = arr.count
	while i > 0 do
		l = list\cons( arr[i], l )
		i = i - 1
	l

list.take = (count) =>
	if count > 0 then
		if self.tail and self.tail != list.empty_ then
			list\cons( self.head, self.tail\take(count - 1))
		else
			list\cons( self.head, list\empty())
	else
		list\empty()

list.zipWith = (l) =>
	h = _1: self.head, _2: l.tail
	if self.tail and self.tail != list.empty_ and l.tail and l.tail != list.empty_ then
		list\cons( h, self.tail\zipWith( l.tail ))
	else
		list\cons( h, list\empty())

numberListInternal = ( n, i ) ->
	if i <= n then
		list\cons( i, numberListInternal( n, i+1 ))
	else
		list\empty()

listOfNumbers = ( n ) ->
	numberListInternal( n, 1 )

list.zipWithIndex = () =>
	self:zipWith( listOfNumbers( self\length() ))
		
list.prepend = ( h ) => list\cons( h, self )

list.contains = ( e ) =>
	if self.head and self.head == e then true
	else if self.tail and self.tail != list.empty_ then self.tail\contains( e ) else false

list
