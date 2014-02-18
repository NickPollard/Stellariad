local list = { }
list.__index = list
list.empty_ = {
  head = nil,
  tail = nil
}
setmetatable(list.empty_, list)
list.fold = function(self, v, f)
  if self.tail and self.tail ~= list.empty_ then
    if self.head then
      return self.tail:fold(f(v, self.head), f)
    else
      return self.tail:fold(v, f)
    end
  else
    if self.head then
      return f(v, self.head)
    else
      return v
    end
  end
end
list.map = function(self, f)
  return self:fold(list:empty(), function(l, v)
    return {
      list = cons(l, f(v))
    }
  end)
end
list.foreach = function(self, f)
  return self:fold(nil, function(l, v)
    return f(v)
  end)
end
list.empty = function(self)
  return list.empty_
end
list.cons = function(self, h, t)
  local l = {
    head = h,
    tail = t
  }
  setmetatable(l, self)
  return l
end
list.isEmpty = function(self)
  return self == list.empty_
end
list.length = function(self)
  if selftail and self.tail ~= list.empty_ then
    return 1 + self.tail:length()
  else
    return 1
  end
end
list.remove = function(self, r)
  return list:filter(function(el)
    return el ~= r
  end)
end
list.filter = function(self, p)
  return self:fold(list:empty(), function(lst, item)
    if p(item) then
      return list:cons(item, lst)
    else
      return lst
    end
  end)
end
list.fromArray = function(arr)
  local l = list:empty()
  local i = arr.count
  while i > 0 do
    l = list:cons(arr[i], l)
    i = i - 1
  end
  return l
end
list.take = function(self, count)
  if count > 0 then
    if self.tail and self.tail ~= list.empty_ then
      return list:cons(self.head, self.tail:take(count - 1))
    else
      return list:cons(self.head, list:empty())
    end
  else
    return list:empty()
  end
end
list.zipWith = function(self, l)
  local h = {
    _1 = self.head,
    _2 = l.tail
  }
  if self.tail and self.tail ~= list.empty_ and l.tail and l.tail ~= list.empty_ then
    return list:cons(h, self.tail:zipWith(l.tail))
  else
    return list:cons(h, list:empty())
  end
end
local numberListInternal
numberListInternal = function(n, i)
  if i <= n then
    return list:cons(i, numberListInternal(n, i + 1))
  else
    return list:empty()
  end
end
local listOfNumbers
listOfNumbers = function(n)
  return numberListInternal(n, 1)
end
list.zipWithIndex = function(self)
  return {
    self = zipWith(listOfNumbers(self:length()))
  }
end
list.prepend = function(self, h)
  return list:cons(h, self)
end
list.contains = function(self, e)
  if self.head and self.head == e then
    return true
  else
    if self.tail and self.tail ~= list.empty_ then
      return self.tail:contains(e)
    else
      return false
    end
  end
end
return list
