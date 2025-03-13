--- @ingroup dcplaya_lua_playlist
--- @file    playlist.lua
--- @author  benjamin gerard
--- @date    2002/12/12
--- @brief   loading and saving playlist file.
---
playlist_loaded = nil

--- @defgroup dcplaya_lua_playlist Playlist
--- @ingroup  dcplaya_lua_basics
--- @brief    loading and saving playlist file.
---
---  @par Supported format
---
---  Playlist formats supported for loading:
---    - @b m3u (with extended info)
---    - @b pls
---
---  Playlist formats supported for saving:
---    - @b m3u (without extended info)
---
---  @par Adding new playlist format:
---
---    The playlist module allows to add new playlist supported format :
---      - write a loader function which get a file parameter and return
---        a playlist entry (playlist_entry). This function will be call
---        by the generic playlist loader function playlist_load_generic()
---        until it returns nil.
---      - Add a new filetype for this playlist format with the function
---        filetype_add().
---      - Reference the new loader with the playlist_add_loader() function.
---
--- @verbatim
--- function mypl_generic_loader(file)
---    local name=read(file)
---    return name and ( file=name }
--- end
---
--- filetype_add("playlist", "mypl", ".mypl.\0.mpl\0")
--- playlist_add_loader("mypl", mypl_generic_loader)
--- @endverbatim
---        
---
---  @par Limitations
---
---    Playlist module could only save m3u playlist, since there is no
---    generic save function nor saver table.
---    It should be very easy to add it but it is not very useful !
---
---
--- @author  benjamin gerard
---
--- @{

--- Save a playlist in basic m3u format.
---
---  @param  fname  Playlist filename.
---  @param  dir    Table of entry { [file,] name [,path] }.
---
---  @return error-code
---  @retval nil on error
---  @retval 1 on success
--
function playlist_save(fname, dir)
   if type(fname) ~= "string"  or type(dir) ~= "table" then return end

   unlink(fname)
   local file = openfile(fname,"w")
   if not file then return end
   
   print("Saving playlist ["..fname.."]")
   write(file,"# dcplaya - playlist\n")

   local i,v
   for i,v in dir do
      if type(v) == "table" then
	 local name = v.file or v.name
	 if type(name) == "string" then
	    if type(v.path) == "string" then
	       name = v.path .. "/" .. name
	    end
	    name = canonical_path(name)
	    write(file, name, "\n")
	    print(" ["..name.."]")
	 end
      end
   end
   closefile(file)
   return 1
end

--- Make a playlist entry.
--- @internal
---
---  @param  path   Default path (should be the playlist file path)
---  @param  entry  Entry table as return by the read_entry function.
---
---  @return playlist-entry table
---  @retval nil om error.
--
function playlist_make_entry(path, entry)
   if not entry then return end

   local file = entry.file or entry.name
   if type(file) ~= "string" then return end

   file = gsub(file, "\\","/") -- Remove DOS backslash
   local char13 = strchar(13)
   file, char13 = gsub(file, char13, "") -- Remove DOS fucking 'CR'

   if strfind(file,"^%a:.*") then
      if strfind(file,"^%a:/.*") then
	 file = "/cd/"..strsub(file,4) -- assume DOS absolute path are /cd
      else
	 file = strsub(file,3)
      end
   end
   local epath, eleaf = get_path_and_leaf(file,path)
   if not eleaf then return end

   -- VP put back the trailing / if there was one
--   if strsub(file, -1) == "/" then
--      eleaf = eleaf.."/"
--   end
   print(epath, eleaf)

   return {
      file = eleaf,
      name = entry.name or eleaf,
      path = epath,
      size = entry.size or 0,
      time = entry.time
   }
end

--- Load playlist generic function.
--- @internal
---
---  @param  path        Playlist file path.
---  @param  file        Playlist file handle open in read mode.
---  @param  read_entry  Function that read an entry in playlist file.
---
---  @return entries table
---  @retval nil on error.
---
function playlist_load_generic(path, file, read_entry)
   local dir = { n = 0 }

   local entry = read_entry(file)
   while entry do
      entry = playlist_make_entry(path, entry)
--      print("playlist_load_generic:"..getn(dir))

--      dump(entry,path)

      if entry then
--	 print("entry:"..entry.name)
	 --		 dump(entry)
	 tinsert(dir, entry)
      end
--      print("playlist_load_generic readentry IN")
      entry = read_entry(file)
--      print("playlist_load_generic readentry OUT : "..tostring(entry))
   end
--   print("playlist_load_generic out :"..getn(dir))

   return dir
end

--- Load a mu3 playlist entry.
--- @internal
---
---  @param  file  Playlist file handle open in read mode
---
---  @return playlist entry.
---  @retval nil on error or end of file.
---
function playlist_load_m3u_entry(file)
   local start,stop,time,name
   local cnt=0

   local line = read(file)
   while line do
      if strlen(line) > 0 then
	 if strsub(line,1,1) == "#" then
	    start,stop,time,name = strfind(line,"^#EXTINF:([0-9]*),(.*)")
	    print(start, stop, time, name)
	 else
	    if not name then
	       start = strfind(line, "<endpath>")
	       if start then
		  name = strsub(line, 1, start-1)
	       end
	    end
	    return { name = name, file = line, time = tonumber(time) }
	 end
      end
      line = read(file)
      cnt = cnt+1
      if cnt > 1024 then
	 print("playlist_load_m3u_entry : too many lines !!")
	 return
      end
   end
end

--- Load a pls playlist entry.
--- @internal
---
---  @param  file  Playlist file handle open in read mode
---
---  @return playlist entry.
---  @retval nil on error or end of file.
---
function playlist_load_pls_entry(file)
   local start,stop,label,id,value
   local filepos, entry

   local line = read(file)
   while line do
      --	  print("pls line ["..line.."]")
      if strlen(line) > 0 then
	 start,stop,label,id,value = strfind(line,"^(%a*)(%d*)=(.*)")
	 if start then
	    print("label:"..label.." id:"..id.." value:"..value)

	    if not entry or not entry.id then
	       entry = { id=id }
	    elseif id and entry.id ~= id then
	       if filepos and filepos >= 0 then
		  print("rewind to ", filepos)
		  seek(file, "set", filepos)
	       end
	       return entry
	    end
	    label = strupper(label)
	    if label == "FILE" then
	       entry.file = value
	    elseif label == "TITLE" then
	       entry.name = value
	    elseif label == "LENGTH" then
	       entry.time = tonumber(value)
	    end
	 end
      end
      filepos = seek(file)
      line = read(file)
   end
end

--- Load a playlist.
---
---  @param  fname  Playlist filename.
---
---  @return error-code
---  @retval nil on error
---  @retval 1 on success
--
function playlist_load(fname)
   if type(fname) ~= "string" then return end

   local type, major,minor = filetype(fname, 0)
   if not major or major ~= "playlist" or not minor then
      print("playlist_load : not a playlist ["..fname.."]")
      return
   end

   local file = openfile(fname,"r")
   if not file then
      print("playlist_load : open error ["..fname.."]")
      return
   end
   local path,leaf = get_path_and_leaf(fname)
   local dir
   local loader = playlist_loaders and playlist_loaders[minor]

   if loader then
      print("playlist path:"..tostring(path))
      dir = playlist_load_generic(path, file, loader)
   end

   closefile(file)

   if dir then
      print("playlist loading success : "..dir.n.." entries")
   end

   return dir
end

--- Reference a new playlist loader.
---
---   The playlist_add_loader() functions add an entry to the playlist_loaders
---   table indexed by minor with the loader function. playlist_loaders will
---   be create if it does not exist.
--- 
---  @param  minor   Minor type name for this playlist type.
---  @param  loader  Playlist load entry function.
---
---  @return error-code
---  @retval nil, on error.
---  @retval 1, on success.
-- 
function playlist_add_loader(minor, loader)
   if type(playlist_loaders) ~= "table" then
      playlist_loaders = {}
   end

   if type(minor) ~= "string" or type(loader) ~= "function" then
      print("playlist_add_loader : failed")
      return
   end
   playlist_loaders[minor] = loader
   return 1;
end

--- Referenced playlist loader table.
---
---   The playlist_loader table references all playlist type that
---   could be load. The table is indexed by minor filetype and contains
---   the loader function for that type.
---
---   The loader function get a file as parameter and returns the next
---   entry read in file.
---
--: function playlist_loaders[minor];

--
--- @}
----

if type(filetype_add) == "function" then
   filetype_add("playlist")
   filetype_add("playlist", nil, ".m3u\0")
   filetype_add("playlist", nil, ".pls\0")
end

playlist_add_loader("m3u", playlist_load_m3u_entry)
playlist_add_loader("pls", playlist_load_pls_entry)

playlist_loaded = 1
return playlist_loaded
