--- @ingroup dcplaya_lua_colors
--- @file    color.lua
--- @author  benjamin gerard
--- @date    2002/10/14
--- @brief   color type.
---
--- $Id$
---

-- Loaded required libraries
--
if not dolib("basic") then return end

-- Create a new tag for color (once).
--
if not color_tag then color_tag = newtag() end

-- Create name to index table for .a .r .g .b access
--
if not color_name_to_index then
   color_name_to_index = { ["a"]=1, ["r"]=2, ["g"]=3, ["b"]=4 }
end

--- @defgroup dcplaya_lua_colors Color operations
--- @ingroup  dcplaya_lua_basics
--- @brief    color operations
--- @author  benjamin gerard
--- @{

--- Color type.
---
--- struct color : vector {
---   static tag color_tag;              ///< tag for color table.
---   static table color_name_to_index;  ///< convert [a,r,g,b] index to number
---   number a;                          ///< alpha component (index 1)
---   number r;                          ///< red component (index 2)
---   number g;                          ///< green component (index 3)
---   number b;                          ///< blue component (index 4)
--- };

--- Create a new color object.
---
---   Creates a new color object from either four numbers or a table of four
---   numbers or a string. Default value is 1 for all components.
---
---   @b table can be any table. rawget() function is used to retrieve color
---   components with index 1,2,3,4 for respectively a,r,g,b components.
---
---   @b string := "COLOR-NAME". Picks the rgb_color_table[COLOR-NAME], if
---      it exists. This conversion is prior to the hexadecimal string
---      conversion which mean it is possible to short circuit an hexadecimal
---      value to another color.
---
---   @b string := "[#][AA]RRGGBB". This is an hexacimal description of the
---   color. It may start by an optional '#' char. Alpha componant is
---   optionnal and its default value is 1.
---
---   @warning With table or string creation methods the noclip parameter
---            is in 2nd position.
---
--- @param   a        alpha component or a table or a string.
--- @param   r        red component
--- @param   g        green component
--- @param   b        blue component
--- @param   noclip   prevents [0..1] clipping is not nil.
---
--- @return a new color table.
--- 
function color_new(a,r,g,b,noclip)
   if type(a) == "string" then
      noclip = r
      if strsub(a,1,1) ~= "#" then
	 a = type(rgb_color_table) == "table" and rgb_color_table[a]
      else
	 local s = strsub(a,2)
	 local len = strlen(s)
	 a = 1
	 if len > 6 then
	    a = floor(mod(tonumber(strsub(s,1,len-6), 16) or 255,256)) / 255
	    s = strsub(s,len-6+1)
	 end
	 local n = tonumber(s,16) or 16777215
	 b = floor(mod(n,256)) / 255
	 g = floor(mod(n/256,256)) / 255
	 r = floor(mod(n/65536,256)) / 255
      end
   end

   if type(a) == "table" then
      b = rawget(a,4)
      g = rawget(a,3)
      r = rawget(a,2)
      a = rawget(a,1)
      noclip = r
   end

   local c = { a or 1, r or 1, g or 1, b or 1 }
   settag(c,color_tag)
   if not noclip then color_clip(c) end
   return c
end

--- Copy a color.
--- 
--- @param  c        destination color or nil for a new color.
--- @param  s        source color.
--- @param  noalpha  Prevents alpha component copy if not nil.
--- @return c or a new color table.
---
function color_copy(c,s,noalpha)
   if not c then
      -- no alpha does not make sense in that case
      return color_new(s)
   end
   if tag(c) == color_tag then
      local v
      if not noalpha then
	 v = rawget(s,1)
	 if v then rawset(c,1,v) end
      end
      v = rawget(s,2)
      if v then rawset(c,2,v) end
      v = rawget(s,3)
      if v then rawset(c,3,v) end
      v = rawget(s,4)
      if v then rawset(c,4,v) end
      return c
   end
end

--- Clip color components.
---
--- @param  c    Color to clip
--- @param  min  Minimum clipping value or nil for no minimum
--- @param  max  Maximum clipping value or nil for no maximum
--- @return c
---
function color_clip(c, min, max)
   if tag(c) == color_tag then
      rawset(c,1,clip_value(rawget(c,1),min,max))
      rawset(c,2,clip_value(rawget(c,2),min,max))
      rawset(c,3,clip_value(rawget(c,3),min,max))
      rawset(c,4,clip_value(rawget(c,4),min,max))
      return c
   end
end

--- Get color luminosity.
---
--- @param   c  Color.
--- @return number
---
function color_lum(c)
   if tag(c) == color_tag then
      return getraw(c,2)*0.5 + getraw(c,3)*0.3 + getraw(c,4)*0.2
   end
end

--- Get index of first hightest component.
---
--- @param   c  Color.
--- @return index
---
function color_max(c)
   if tag(c) == color_tag then
      return table_max(c)
   end
end

--- Get index of first lowest component.
---
--- @param   c  Color.
--- @return index
---
function color_min(color)
   if tag(color) == color_tag then
      return table_min(color)
   end
end

--- Scale color to have the highest component equal to 1.
---
--- @param   c  Color.
--- @return c
---
function color_maximize(c)
   local max = color_max(c)
   if max and rawget(c,max) > 0.0001 then
      local scale=rawget(c,max)
      rawset(c,1,rawget(c,1)/scale)
      rawset(c,2,rawget(c,2)/scale)
      rawset(c,3,rawget(c,3)/scale)
      rawset(c,4,rawget(c,4)/scale)
   else
      rawset(c,1,1)
      rawset(c,2,1)
      rawset(c,3,1)
      rawset(c,4,1)
   end
   return c
end

--- Transform color to string (#AARRBBGG).
---
--- @param   c  Color.
--- @return string
---
function color_tostring(c)
   if tag(c) == color_tag then
      return format("#%02X%02X%02X%02X",
		    clip_value(rawget(c,1)*255,0,255),
		    clip_value(rawget(c,2)*255,0,255),
		    clip_value(rawget(c,3)*255,0,255),
		    clip_value(rawget(c,4)*255,0,255))
   end
end

--- Transform color to string (#AARRBBGG).
---
---  This function works kike color_tostring() except it always return
---  a string. The default value is "#00000000".
---
--- @param   c  Color.
--- @return string
---
function color_tohexa(c)
   return color_tostring(c) or "#00000000"
end

--- color "concat" tag method.
--- 
--- @param   a  concat left operand
--- @param   b  concat right operand
--- @return string
---
function color_concat(a,b)
   local s1,s2
   s1 = color_tostring(a) or tostring(a)
   s2 = color_tostring(b) or tostring(b)
   return s1..s2
end


--- color "index" tag method.
---
--- @param   c  color
--- @param   i  an index (valid values are "a","r","g","b")
---
--- @return color component
---
function color_index(c,i)
   if tag(c) == color_tag then
      local idx = color_name_to_index[i]
      if idx then	return c[idx] end
   end
end

--- color "settable" tag method.
---
--- @param   c  color
--- @param   i  index [1..4] or ["a","r","g","b"]
--- @param   v  component new value
---
function color_settable(c,i,v)
   if tag(c) == color_tag then
      if type(i) == "string" then
	 i = color_name_to_index[i]
      end
      if type(i) == "number" and i>=1 and i<=4 then
	 rawset(c,i,v)
      end
   end
end

-- Overload standard tostring() function.
function tostring(c)
   return color_tostring(c) or %tostring(c)
end

--- color "add" tag method; "+" operator.
---
--- @param   a  left operand (color or number)
--- @param   b  right operand (color or number)
--- @return  color
---
--- @see table_add()
---
function color_add(a,b)
   local r=table_add(a,b)
   if type(r) == "table" then
      settag(r,color_tag)
      return r
   end
end

--- color "sub" tag method; "-" operator.
---
--- @param   a  left operand (color or number)
--- @param   b  right operand (color or number)
--- @return  color
---
--- @see table_sub()
---
function color_sub(a,b)
   local r=table_sub(a,b)
   if type(r) == "table" then
      settag(r,color_tag)
      return r
   end
end

--- color "mul" tag method; "*" operator.
---
--- @param   a  left operand (color or number)
--- @param   b  right operand (color or number)
--- @return  color
---
--- @see table_mul()
---
function color_mul(a,b)
   local r=table_mul(a,b)
   if type(r) == "table" then
      settag(r,color_tag)
      return r
   end
end

--- color "div" tag method; "/" operator.
---
--- @param   a  left operand (color or number)
--- @param   b  right operand (color or number)
--- @return  color
---
--- @see table_div()
---
function color_div(a,b)
   local r=table_div(a,b)
   if type(r) == "table" then
      settag(r,color_tag)
      return r
   end
end

--- color "minus" tag method; unary "-" operator.
---
--- @param   a  left operand (color or number)
--- @param   b  right operand (color or number)
--- @return  color
---
--- @see table_minus()
---
function color_minus(a,b)
   local r=table_minus(a,b)
   if type(r) == "table" then
      settag(r,color_tag)
      return r
   end
end

--- Absolutes color components.
---
--- @param   c  color
--- @return c with each component as the absolute value of its former value.
---
function color_abs(c)
   if tag(c) == color_tag then
      rawset(c,1,abs(rawget(c,1)))
      rawset(c,2,abs(rawget(c,2)))
      rawset(c,3,abs(rawget(c,3)))
      rawset(c,4,abs(rawget(c,4)))
   end
end

--- Get a distant color
---
--- @param   c    color
--- @param   alt  alternate color. If nil alt is the one's complement color.
---
--- @return color which should be distant from the colorc.
---
function color_distant(c, alt)
   if tag(c) ~= color_tag then return end
   local t = 0.5
   local r = color_new(1,1,1,1) - c
   local d = c ^ r
   if d < t then
      if tag(alt) ~= color_tag then alt = color_new(alt) end
      if tag(alt) ~= color_tag then alt = color_new(1,1,1,1) end
      d = d / t
      r = r * d + alt * (1-d)
   end
   return r
end

--- Descending sort R,G,B components.
--- 
--- @param   c  color
--- @return a table with the index of the componant at this raw.
---
--- e.g { 0.3, 0.1, 0.8, 0.2 } => { 3, 1, 4, 2 }
---
function color_sort(c)
   if tag(c) ~= color_tag then return end
   local order = { 2, 3, 4 }
   local i,j
   for i=1, 2, 1 do
      for j=i+1, 3, 1 do
	 if rawget(c,order[j]) > rawget(c,order[i]) then
	    order[i], order[j] = order[j], order[i]
	 end
      end
   end
   return order[1],order[2],order[3]
end


-- Set tag method for color object
--
settagmethod(color_tag, "add",		color_add)
settagmethod(color_tag, "sub",		color_sub)
settagmethod(color_tag, "mul",		color_mul)
settagmethod(color_tag, "div",		color_div)
settagmethod(color_tag, "pow",		table_sqrdist)
settagmethod(color_tag, "unm",		color_minus)
settagmethod(color_tag, "concat",	color_concat)
settagmethod(color_tag, "index",	color_index)
settagmethod(color_tag, "settable",	color_settable)

-- Load rgb color table
dolib("rgb")

return 1

--
--- @}
----
