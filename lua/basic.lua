--- @file    basic.lua
--- @ingroup dcplaya_lua_basics
--- @author  vincent penne
--- @author  benjamin gerard
--- @brief   basic things used into other library (evt, keyboard_emu, gui)
---
--- $Id$
---

--
--- @defgroup dcplaya_lua_basics_linklist Doubly Linked List
--- @ingroup  dcplaya_lua_basics
--- @brief    doubly linked list support
---
--- @author    vincent penne
---
--- @{
---

--- insert a new element in list as first element (or as last simply by swaping
--- ofirst and olast, iprev and inext).
--- 
--- @param o       owner
--- @param ofirst  index in the owner pointing to first element of list
--- @param olast   index in the owner pointing to last element of list
--- @param i       item to insert
--- @param iprev   index in the item of prev element
--- @param inext   index in the item of next element
--- @param iowner  index in the item of owner element
---
function dlist_insert(o, ofirst, olast, i, iprev, inext, iowner)
   local f = o[ofirst]
   i[iprev] = nil
   i[inext] = f
   o[ofirst] = i
   if f then
      f[iprev] = i
      -- 	end
      -- 	if not o[olast] then
   else
      o[olast] = i
   end
   i[iowner] = o
end

--- remove an element from a list.
---
--- @param ofirst  index in the owner pointing to first element of list
--- @param olast   index in the owner pointing to last element of list
--- @param i       item to insert
--- @param iprev   index in the item of prev element
--- @param inext   index in the item of next element
--- @param iowner  index in the item of owner element
---
function dlist_remove(ofirst, olast, i, iprev, inext, iowner)
   local o = i[iowner]
   if not o then
      return
   end
   local p = i[iprev]
   local n = i[inext]
   if p then
      p[inext] = n
   else
      o[ofirst] = n
   end
   if n then
      n[iprev] = p
   else
      o[olast] = p
   end

   i[iowner] = nil
   i[iprev] = nil
   i[inext] = nil
end

---
--- @}
---

--- @defgroup dcplaya_lua_basics_table Table Operators
--- @ingroup  dcplaya_lua_basics
--- @brief    table operators
--- @warning this is not complete ...
---
--- @{
---

--- the ^ operator calculate the square distances between two tables.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table
--- @param   b  table
---
function table_sqrdist(a, b)
   local sum = 0
   local i, v
   for i, v in a do
      local d = v - b[i]
      sum = sum + d*d
   end
   return sum
end

--- the + operator.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table or any type that support '+' operator
--- @param   b  table or any type that support '+' operator
--- @return table
--- @warning At least one of the parameters must be a table
---
function table_add(a, b)
   local r = {}
   local i, v
   if type(b) == "table" then
      a, b = b, a
   end
   if type(b) == "table" then
      for i, v in a do
	 r[i] = v + b[i]
      end
   else
      for i, v in a do
	 r[i] = v + b
      end
   end
   return r
end

--- the - operator.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table or any type that support '-' operator
--- @param   b  table or any type that support '-' operator
--- @return table
--- @warning At least one of the parameters must be a table
---
function table_sub(a, b)
   local r = {}
   local i, v
   if type(a) == "table" then
      if type(b) == "table" then
	 for i, v in a do
	    r[i] = v - b[i]
	 end
      else
	 for i, v in a do
	    r[i] = v - b
	 end
      end
   else
      for i, v in b do
	 r[i] = a - v
      end
   end
   return r
end

--- the * operator.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table or any type that support '*' operator
--- @param   b  table or any type that support '*' operator
--- @return table
--- @warning At least one of the parameters must be a table
---
function table_mul(a, b)
   local r = {}
   local i, v
   if type(b) == "table" then
      a, b = b, a
   end
   if type(b) == "table" then
      -- two tables case
      for i, v in a do
	 r[i] = v * b[i]
      end
   else
      -- number * table case
      for i, v in a do
	 r[i] = v * b
      end
   end
   return r
end

--- the / operator.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table or any type that support '/' operator
--- @param   b  table or any type that support '/' operator
--- @return table
--- @warning At least one of the parameters must be a table
---
function table_div(a, b)
   local r = {}
   local i, v
   if type(a) == "table" then
      if type(b) == "table" then
	 -- two tables case
	 for i, v in a do
	    r[i] = v / b[i]
	 end
      else 
	 -- tables / number
	 for i, v in a do
	    r[i] = v / b
	 end
      end
   else
      -- number / table case
      for i, v in b do
	 r[i] = a / v
      end
   end
   return r
end

--- the unary - operator.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table
--- @return table
---
function table_minus(a)
   local r = nil
   local i, v
   
   if type(a) == "table" then
      r = {}
      for i, v in a do
	 r[i] = -v
      end
   end
   return r
end

--- Get maximum value of a table.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table
--- @return table element
--- @warning table elements must support the '>' operator.
---
function table_max(a)
   local imax = nil
   if type(a) == "table" and getn(a) > 0 then
      local i, v, max
      imax = 1
      max = a[1]
      for i, v in a do
	 if (v > max) then
	    imax = i
	    max = v
	 end
      end
   end
   return imax
end

--- Get minimum value of a table.
--- @ingroup dcplaya_lua_basics_table
---
--- @param   a  table
--- @return table element
--- @warning table elements must support the '<' operator.
---
function table_min(a)
   local imin = nil
   if type(a) == "table" and getn(a) > 0 then
      local i, v, min
      imin = 1
      min = a[1]
      for i, v in a do
	 if (v < min) then
	    imin = i
	    min = v
	 end
      end
   end
   return imin
end

---
--- @}
---

--
--- @defgroup  dcplaya_lua_basics  Basic Components
--- @ingroup   dcplaya_lua
--- @brief     LUA low level tool-kit.
---
--- @author    benjamin gerard
--- @author    vincent penne
---
--- @{
---

--- Duplicate any type.
---
--- @param  v  anything to duplicate
--- @return duplication of v
---
function dup(v)
   local t = type(v)
   
   if t == "table" then
      local tbl = {}
      local i,w
      for i,w in v do
	 rawset(tbl,i,dup(w))
      end
      if tag(tbl) ~= tag(v) then settag(tbl,tag(v)) end
      return tbl
   else
      return v
   end
end

--- Get a lua compatible string describing this object.
---
--- @param   v       Object to dump
--- @param   name    Optional name of v
--- @param   indent  Indent level
--- @return  string
--- @warning This is a recursive "dangerous" function.
---
function type_dump(v, name, indent)
   if not indent then indent = 0 end
   local t = type(v)
   local istr = strrep(" ",indent*2)
   local s = istr
   if type(name) == "string" then
      s = s..format("[%q]=",name)
   end
   
   if t == "number" then
      s=s..v
   elseif t == "string" then
      s=s..format("%q",v)
   elseif t == "table" then
      s=s.."{\n"
      local i,w
      for i,w in v do
	 s=s..type_dump(w,i,indent+1)..",\n"
      end
      s=s..istr.."}"
   elseif t == "function" then
      s=s..tostring(nil) -- getinfo(v).name
   else
      local a = tostring(v)
      if type(a) == "string"  then s=s..a else s=s.."???" end
   end
   return s
end

--- Print a lua compatible string describing this object.
--- @see type_dump()
---
function dump(v, name, indent)
   print(type_dump(v,name,indent))
end

--- Clip a value.
---
--- @param  v  Value to clip
--- @param  min  Optional minimum clip value.
--- @param  max  Optional maximum clip value.
--- @return clipped value
--- @warning if respectively min / max is not nil, v<min / v>max must be a
---          valid operation
function clip_value(v,min,max)
   if min and v < min then v = min end
   if max and v > max then v = max end
   return v
end

--- Set a vextex.
---
--- @param  vect  Vector (matrix line)
--- @param  from  Table containing vector components
---
function set_vertex(vect, from)
   local i,v
   for i,v in from do
      if v then vect[i]=v end
   end
end

--- printf like function.
---
--- @param  ...  format string and arguments needed by format string.
--- @warning Add a trailing newline.
function printf(...)
   print(call(format,arg))
end

--- Console clear.
---
---  The clearconsole() function send a "\027E" string to the console
---  with the rawprint() function. That will clear the console.
---
--- @code
function clearconsole()
   if type(rawprint) == "function" then
      rawprint("\027E")
   end
end
--- @endcode
---
--- @see dcplaya_mutermdef_devel for other terminal escape codes.
---
--: clearconsole();

addhelp("clearconsole","cls","console",
	'clearconsole() : Clear the console.')

---
--- @}
---

settagmethod(tag( {} ), "add", table_add)
settagmethod(tag( {} ), "sub", table_sub)
settagmethod(tag( {} ), "mul", table_mul)
settagmethod(tag( {} ), "div", table_div)
settagmethod(tag( {} ), "pow", table_sqrdist)
settagmethod(tag( {} ), "unm", table_minus)

basic_loaded=1
return basic_loaded
