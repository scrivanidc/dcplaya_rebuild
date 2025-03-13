--- @ingroup dcplaya_lua_gui
--- @file    gui.lua
--- @author  vincent penne
--- @brief   gui lua library on top of evt system
---
--- $Id$
---
---

--- @defgroup  dcplaya_lua_gui  Graphic User Interface
--- @ingroup   dcplaya_lua
--- @brief     graphic user interface.
---
--- dcplaya graphic user interface (gui) are
--- @link dcplaya_application applications @endlink. Generally these
--- applications display something on a box. GUI must have required
--- fields, have a look to the ::gui structure definition for more information
---
---
--- @par Z space organization
---
---   Each gui application have a Z space from 0 to 200. This space is divided
---   in two parts. The first from 0 to 100 is the application Z space.
---   The second (obviously) from 100 to 200 is reserved for its children.
---   The whole stuff is managed by a system of
---   @link dcplaya_display_list sub-display-list @endlink directly. The 
---   gui_child_autoplacement() does all the tricks and must be call when
---   application focus changes. This is done by the gui_dialog_basic_handle()
---   function, so the simpliest thing to do is to call this function in
---   the gui application handler. Technically these functions creates a 
---   display-list (_dl) which application display-list (dl) becomes and
---   sublist and all suitable transforation are performed in the _dl
---   display-list so that the [0..100] coordinates in the dl display-list
---   match the application reserved Z. 
---
--- @par Event translation
--- 
--- The dialog box handle function take care of translating a few events :
---   - Button A is translated to event gui_evt_confirm
---   - Button X is translated to event gui_evt_select
---   - Button B is translated to event gui_evt_cancel
---
--- @par
---
--- These translated events are sent @b ONLY to the focused widget into the
--- dialog.
--- So if you want to react only when you are focused, check for these
--- translated events, if you want to react in any cases, then check for
--- the untranslated events. (Because untranslated events are @b STILL sent
--- to all child of the dialog with usual convention (first to focused child,
--- then to the dialog itsef, and at last to other children)
---
--- @since 04/03/2003
---
--- @see dcplaya_lua_app
--- @see dcplaya_lua_event
---
--- @author    penne vincent
--- @{
---

--- gui application.
--: struct gui : application {
--:   /** the item box { x1, y1, x2, y2 } used for focusing.
--:    *  As table are referenced, changing it will affect the focus in
--:    *  both way : display and layout.
--:    */
--:   number box[4];
--:
--:   /** Default display list used to draw inside. */
--:   display_list dl;
--:
--:   /** Z position of the item for reference to draw upon it.
--:    *  @warning  z position should be handle with care. Have a look
--:    *   to the Z space organization in the
--:    *   @link dcplaya_lua_gui gui documentation @endlink.
--:    *  @see gui_child_autoplacement()
--:    *  @see gui_dialog_basic_handle()
--:    **/
--:    number z;
--:
--:   /** Table of function(app, evt) indexed by event key.
--:    *  @see dcplaya_lua_event
--:    */
--:   event_handler event_table[event_key];
--:
--:   /** Table whose entry describe some functionalities. */
--:    boolean flags[flag_enum];
--:
--:   /** Optional @link dcplaya_lua_menu_gui menu definition @endlink opened
--:    *  by the
--:    *  @link dcplaya_lua_application_switcher application switcher @endlink
--:    */
--:    menudef mainmenu_def;
--:
--:   /** gui flags enumeration.
--:    *  @warning This documentation show tou this as "C" enumeration, in lua
--:    *  implementation is value are strings.
--:    */
--:    enum flag_enum {
--:     /** The item (usually a dialog box) eats all events except shutdown
--:      *  (can be dangerous !).
--:      */
--:      "modal", 
--:
--:     /** The item cannot be focused.
--:      *  @deprecated Seems not to be implemented.
--:      */
--:      "inactive"
--:    };
--: };

if not dolib "evt" or not dolib "box3d" or not dolib "taggedtext" then
   return
end

gui_box_bcolor    = color_new({ 0.8, 0.2, 0.3, 0.3 })
gui_box_color1    = color_new({ 0.8, 0.2, 0.3, 0.3 })
gui_box_color2    = color_new({ 0.8, 0.1, 0.15, 0.15 })
gui_button_bcolor = color_new({ 0.8, 0.5, 0.7, 0.5 })
gui_button_color1 = color_new({ 0.8, 0.5, 0.7, 0.5 })
gui_button_color2 = color_new({ 0.8, 0.2, 0.4, 0.4 })
gui_text_color    = color_new({ 0.9, 1.0, 1.0, 0.7 })
gui_text_shiftbox = color_new({ 5, 5, -5, -5 })
gui_input_color1  = color_new({ 0.8, 0.2, 0.3, 0.3 })
gui_input_color2  = color_new({ 0.8, 0.1, 0.1, 0.1 })
gui_input_cursor_color1 = color_new({ 1, 1, 0.5, 0 })
gui_input_cursor_color2 = color_new({ 1, 0.5, 1.0, 0 })
gui_focus_border_width = 2
gui_focus_border_height = 2

gui_keyup = { 
   [KBD_KEY_UP] = 1, 
   [KBD_CONT1_DPAD_UP] = 1, [KBD_CONT1_DPAD2_UP] = 1, 
   [KBD_CONT2_DPAD_UP] = 1, [KBD_CONT2_DPAD2_UP] = 1, 
   [KBD_CONT3_DPAD_UP] = 1, [KBD_CONT3_DPAD2_UP] = 1, 
   [KBD_CONT4_DPAD_UP] = 1, [KBD_CONT4_DPAD2_UP] = 1
}
gui_keydown = { 
   [KBD_KEY_DOWN] = 1, 
   [KBD_CONT1_DPAD_DOWN] = 1, [KBD_CONT1_DPAD2_DOWN] = 1, 
   [KBD_CONT2_DPAD_DOWN] = 1, [KBD_CONT2_DPAD2_DOWN] = 1, 
   [KBD_CONT3_DPAD_DOWN] = 1, [KBD_CONT3_DPAD2_DOWN] = 1, 
   [KBD_CONT4_DPAD_DOWN] = 1, [KBD_CONT4_DPAD2_DOWN] = 1
}
gui_keyleft = { 
   [KBD_KEY_LEFT] = 1, 
   [KBD_CONT1_DPAD_LEFT] = 1, [KBD_CONT1_DPAD2_LEFT] = 1, 
   [KBD_CONT2_DPAD_LEFT] = 1, [KBD_CONT2_DPAD2_LEFT] = 1, 
   [KBD_CONT3_DPAD_LEFT] = 1, [KBD_CONT3_DPAD2_LEFT] = 1, 
   [KBD_CONT4_DPAD_LEFT] = 1, [KBD_CONT4_DPAD2_LEFT] = 1
}
gui_keyright = { 
   [KBD_KEY_RIGHT] = 1, 
   [KBD_CONT1_DPAD_RIGHT] = 1, [KBD_CONT1_DPAD2_RIGHT] = 1, 
   [KBD_CONT2_DPAD_RIGHT] = 1, [KBD_CONT2_DPAD2_RIGHT] = 1, 
   [KBD_CONT3_DPAD_RIGHT] = 1, [KBD_CONT3_DPAD2_RIGHT] = 1, 
   [KBD_CONT4_DPAD_RIGHT] = 1, [KBD_CONT4_DPAD2_RIGHT] = 1
}
gui_keyconfirm = { 
   [KBD_ENTER] = 1, 
   [KBD_CONT1_A] = 1, 
   [KBD_CONT2_A] = 1, 
   [KBD_CONT3_A] = 1, 
   [KBD_CONT4_A] = 1, 
}
gui_keycancel = { 
   [KBD_ESC] = 1, 
   [KBD_CONT1_B] = 1, 
   [KBD_CONT2_B] = 1, 
   [KBD_CONT3_B] = 1, 
   [KBD_CONT4_B] = 1, 
}
gui_keymenu = {
   -- (WARNING : the name is not correct, it does not open a menu in most case)
   [KBD_KEY_PRINT] = 1, 
   [KBD_CONT1_Y] = 1, 
   [KBD_CONT2_Y] = 1, 
   [KBD_CONT3_Y] = 1, 
   [KBD_CONT4_Y] = 1, 
}
gui_keyselect = { 
   [KBD_BACKSPACE] = 1, 
   [KBD_CONT1_X] = 1, 
   [KBD_CONT2_X] = 1, 
   [KBD_CONT3_X] = 1, 
   [KBD_CONT4_X] = 1, 
}

gui_keynext = { 
   [KBD_KEY_PGDOWN] = 1, 
   [KBD_CONT1_C] = 1, 
   [KBD_CONT2_C] = 1, 
   [KBD_CONT3_C] = 1, 
   [KBD_CONT4_C] = 1, 
}
gui_keyprev = { 
   [KBD_KEY_PGUP] = 1, 
   [KBD_CONT1_D] = 1, 
   [KBD_CONT2_D] = 1, 
   [KBD_CONT3_D] = 1, 
   [KBD_CONT4_D] = 1, 
}

-- compute an automatic guess if none is given
function gui_orphanguess_z(z)
   if not z then
--      z = gui_curz
--      gui_curz = gui_curz + 100
      z = 0
   end
   return z
end

-- compute an automatic guess if none is given, using parent's z
function gui_guess_z(owner, z)
   if not z then
      -- $$$ ben : +10 ???
      z = gui_orphanguess_z(owner.z) + 10
   end
   return z
end



-- place child of a dialog box
dskt_zmin = 100
dskt_zmax = 200
-- $$$ ben : make lotsa changes here like cleaning used _dl. 
function gui_child_autoplacement(app)
--   printf("gui_child_autoplacement %q",app.name)

   local i = app.sub
   local n = 0
   while i do
      if i.dl and i.dl ~= app.dl then
	 -- Sub-application has its own a dl 
	 if not i._dl then
	    i._dl = dl_new_list(256, 1, nil, i.name.."._dl")
	    dl_sublist(i._dl, i.dl)
	 end
	 n = n + 1
      elseif i._dl then
--	 printf("sub-app %q has no more own dl !\n",i.name)
	 i._dl = nil
      end
      i = i.next
   end

   -- Application has no sub app, exit and destroy existing _dl1 and _dl2
   if n == 0 then
--       print("-> No sub , exit")
      app._dl1 = nil
      app._dl2 = nil
      if app._dl then
	 if not app.dl then
	    -- If there is no dl, _dl could be destroy safely.
	    app._dl = nil
-- 	    printf("app %q has no more dl !\n",app.name)
	 else
	    -- Else only attach the dl to the _dl.
-- 	    printf("app %q has no more child dl !\n",app.name)
	    dl_clear(app._dl)
	    dl_sublist(app._dl, app.dl)
	 end
      end
      return
   end

   -- From this point the application has at least one sub app to display
   if not app._dl1 then
--       print("-> No _dl1, create it")
      app._dl1 = dl_new_list(256, 0, 1, app.name.."._dl1")
      app._dl2 = dl_new_list(256, 0, 1, app.name.."._dl2")
   else
--       print("-> clear _dl1")
      dl_clear(app._dl1)
   end

   if not app._dl then
--       print("-> No _dl, create it")
      app._dl = dl_new_list(128, 1, 0, app.name.."._dl")
   else
      dl_clear(app._dl)
--       print("-> clear _dl")
   end
   if app.dl then
--       print("-> has dl")
      dl_sublist(app._dl, app.dl)
   end
   dl_sublist(app._dl, app._dl1)
   dl_sublist(app._dl, app._dl2)
   
   -- Attach children _dl to the current _dl and compute its transform matrix
   local zpart,z = 100/n, 200
   i = app.sub
   while i do
      if i._dl then
	 z = z - zpart
	 dl_set_trans(i._dl,
		      mat_scale(1, 1, 0.5/n)
			 * mat_trans(0, 0, z))
	 dl_sublist(app._dl1, i._dl)
-- 	 printf("matrix of %q",i.name)
-- 	 mat_dump(dl_get_trans(i._dl),1)
      end
      i = i.next
   end

   -- swaping display lists
   dl_set_active2(app._dl1, app._dl2, 1)
   app._dl1, app._dl2 = app._dl2, app._dl1
end

-- change focused item
function gui_new_focus(app, f)
   local of = app.sub
   if f and f~=of then
--      if of then
--	 evt_send(of, { key = gui_unfocus_event, app = of })
      --      end
      evt_app_remove(f)
      evt_app_insert_first(app, f)
--      evt_send(f, { key = gui_focus_event, app = f })

      return 1
   end
end


-- test focus item
function gui_is_focus(f)
   return f == f.owner.sub
end

function gui_less(b1, b2, i)
   return b1[i] < b2[i]
end
function gui_more(b1, b2, i)
   --	print (b1[i], b2[i])
   return b1[i] > b2[i]
end

function gui_boxdist(b1, b2)
   local x = b2[1] - b1[1]
   local y = b2[2] - b1[2]
   return x*x + y*y
end

-- find closest item to given box, satisfying given condition
gui_closestcoef = { 1, 1, 1, 1 }
gui_closestcoef_horizontal = { 1, 5, 1, 5 }
gui_closestcoef_vertical = { 5, 1, 5, 1 }
function gui_closest(app, box, coef, cond, condi)

   if not coef then
      coef = gui_closestcoef
   end
   box = box * coef
   local i = app.sub
   if not i then
      return
   end
   local imin = nil
   local min = 1000000000
   repeat
      local b = i.box * coef
      if not cond or cond(b, box, condi) then
	 local d = gui_boxdist(b, box)
	 if d < min then
	    imin = i
	    min = d
	 end
      end

      i = i.next
   until not i

   --	print (min, app.sub, imin)
   
   return imin
   
end

-- A minimal handle function
function gui_minimal_handle(app,evt)
   local key = evt.key
   local f = app.event_table[key]
   if key == evt_shutdown_event then
      if f then 
	 -- still call shutdown user response before standard shutdown
	 f(app, evt) 
      end
      if (app.dl) then
	 dl_set_active(app.dl)
      end
      return evt
   end
   if f then
      return f(app, evt)
   end

   -- $$$ added by ben for vmu display
   if vmu_set_text then
      if key == gui_focus_event and evt.app == app then
	 vmu_set_text("focus on:   "..app.name)
      elseif key == gui_unfocus_event and evt.app == app then
	 vmu_set_text(nil)
      end
   end

   return evt
end

-- dialog

function gui_dialog_shutdown(app)
   if app.sub then
      evt_send(app.sub, { key = gui_unfocus_event, app = app.sub }, 1)
   end
   if gui_disaparate then
      gui_disaparate(app)
   else
      dl_set_active(app.dl)
   end
   dl_set_active(app.focusup_dl)
   dl_set_active(app.focusdown_dl)
   dl_set_active(app.focusright_dl)
   dl_set_active(app.focusleft_dl)
   evt_app_remove(app)
--   cond_connect(1)
end

function gui_dialog_basic_handle(app, evt)
   local key = evt.key
   if (key == evt_app_insert_event or key == evt_app_remove_event) and evt.app.owner == app then
   --if key == evt_app_insert_event and evt.app.owner == app then
      gui_child_autoplacement(app)
      return
   end

   return evt
end

function gui_dialog_handle(app, evt)
   local key = evt.key

   local focused = app.sub

   local f = app.event_table[key]
   if key == evt_shutdown_event then
      if f then 
	 -- still call shutdown user response before standard shutdown
	 f(app, evt) 
      end
      gui_dialog_shutdown(app)
      return evt -- pass the shutdown event to next app
   end
   
   --	print("dialog key:"..key)
   if f then
      return f(app, evt)
   end

   -- $$$ added by ben for vmu display
   if vmu_set_text then
      if key == gui_focus_event and evt.app == app then
	 vmu_set_text("dialog:     " .. app.name)
      elseif key == gui_unfocus_event and evt.app == app then
	 vmu_set_text(nil)
      end
   end

   if (key == evt_app_insert_event or key == evt_app_remove_event) and evt.app.owner == app then

      -- $$$ 
--       printf("gui_dialog_handle %q : recieve %q for %q",
-- 	     app.name, (key == evt_app_insert_event and "insert") or "remove",
-- 	     evt.app.name)

      if key == evt_app_remove_event and focused and focused == evt.app then
	 focused = focused.next
      end

      if focused ~= app.focused then
	 if app.focused then
	    evt_send(app.focused, { key = gui_unfocus_event, app = app.focused }, 1)
	 end
	 if focused then
	    evt_send(focused, { key = gui_focus_event, app = focused }, 1)
	 end
	 app.focused = focused
	 app.focus_time = 0
      end

      gui_child_autoplacement(app)
      return
   end
   
   if focused then
      if key == gui_focus_event or key == gui_unfocus_event then
	 -- translate and pass down the event to the focused item
--	 print("unfocus", focused.name)
	 evt.app = focused
	 evt_send(focused, evt, 1)
	 return
      end

      if gui_keyconfirm[key] then
	 if not evt_send(focused, { key = gui_press_event }) then
	    return
	 end
      end

      if gui_keycancel[key] then
	 if not evt_send(focused, { key = gui_cancel_event }) then
	    return
	 end
      end

      if gui_keyselect[key] then
	 if not evt_send(focused, { key = gui_select_event }) then
	    return
	 end
      end


--      if gui_keymenu[key] then
--	 evt_send(focused, { key = gui_menu_event })
--	 return
--      end
      
      if gui_keyup[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_less, 4)) then
	    return
	 end
      end
      
      if gui_keydown[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_more, 2)) then
	    return
	 end
      end
      
      if gui_keyleft[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_less, 3)) then
	    return
	 end
      end
      
      if gui_keyright[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_more, 1)) then
	    return
	 end
      end
      
   end
   
   if app.flags.modal then
--      print("modal !!")
      return nil
   end
   --	print("dialog unhandle:"..key)
   return evt
end

function gui_dialog_update(app, frametime)
   local focused = app.sub
   if focused and gui_is_focus(app) then
      -- handle focus cursor
      
      -- converge to focused item box
      app.focus_box = app.focus_box + 
	 20 * frametime * (focused.box - app.focus_box)
      
      -- set focus cursor position and color
      dl_set_trans(app.focusup_dl, 
		   mat_scale(app.focus_box[3] - app.focus_box[1], 
			     gui_focus_border_height, 1) *
		      mat_trans(app.focus_box[1],
				app.focus_box[2]-gui_focus_border_height, 0))
      dl_set_trans(app.focusdown_dl, 
		   mat_scale(app.focus_box[3] - app.focus_box[1], 
			     gui_focus_border_height, 1) *
		      mat_trans(app.focus_box[1], app.focus_box[4], 0))
      dl_set_trans(app.focusleft_dl, 
		   mat_scale(gui_focus_border_width, 
			     2*gui_focus_border_height	+ app.focus_box[4]
				- app.focus_box[2], 1)
		      * mat_trans(app.focus_box[1]-gui_focus_border_width,
				  app.focus_box[2]-gui_focus_border_height, 0))
      dl_set_trans(app.focusright_dl, 
		   mat_scale(gui_focus_border_width, 
			     2*gui_focus_border_height + app.focus_box[4] - app.focus_box[2], 1) *
		      mat_trans(app.focus_box[3], app.focus_box[2]-gui_focus_border_height, 0))
      
      app.focus_time = app.focus_time + frametime
      local ci = 0.5+0.5*cos(360*app.focus_time*2)
      
      local focus_dl
      for _, focus_dl in { app.focusup_dl, app.focusdown_dl, app.focusleft_dl, app.focusright_dl } do
	 dl_set_color(focus_dl, 0.15 + 0.25*ci, 1, ci, ci)
	 dl_set_active(focus_dl, 1)
      end
      
   else
      -- no focus cursor
      if app.focusup_dl then 
	 local focus_dl
	 for _, focus_dl in { app.focusup_dl, app.focusdown_dl, app.focusleft_dl, app.focusright_dl } do
	    dl_set_active(focus_dl, nil)
	 end
      end
   end
   
end

function gui_dialog_box_draw(dl, box, z, bcolor, color1, color2, transparent)
   
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]
      local b3d

      b3d= box3d(box, 4, nil, t, l, b, r)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))

      b3d= box3d(box + { 4, 4, -4, -4 }, 2, nil, b, r, t, l)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))

      if not transparent then
	 dl_draw_box(dl, box + {6,6,-6,-6}, z, color1, color2)
      end
   else
      dl_draw_box(dl, box, z, gui_box_color1, gui_box_color2)
   end
end

function gui_button_box_draw(dl, box, z, bcolor, color1, color2)
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]

      --local b3d = box3d(box, 2, color1, t, l, b, r)
      local b3d = box3d(box, 2, nil, t, l, b, r)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))

      dl_draw_box(dl, box+{2,2,-2,-2}, z, color1, color2)
   else
      dl_draw_box(dl, box, z, gui_button_color1, gui_button_color2)
   end
end

function gui_input_box_draw(dl, box, z, bcolor, color, transparent)
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]
      local b3d = box3d(box, 2, not transparent and color, b, r, t, l)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))
   else
      dl_draw_box(app.dl, app.box, z, bcolor, color)
   end
end


function gui_new_dialog(owner, box, z, dlsize, text, mode, name, flags)
   local dial

   --  $$$ ben : default owner is desktop
   if not owner then owner = evt_desktop_app end
   if not owner then print("gui_new_dialog : no desktop") return nil end
  
   z = gui_guess_z(owner, z)
--   print(z)

   if not dlsize then
      dlsize = 10*1024
   end
   name = name or "gui_dialog"
   dial = { 

      name = name,
      version = "0.9",
      
      handle = gui_dialog_handle,
      update = gui_dialog_update,
      
      dl = dl_new_list(dlsize, 1, nil, nil, name .. ".dl"),
      box = box,
      z = z,
      
      focusup_dl = dl_new_list(256, 0, 0, name .. ".focusUP.dl"),
      focusdown_dl = dl_new_list(256, 0, 0, name .. ".focusDW.dl"),
      focusleft_dl = dl_new_list(256, 0, 0, name .. ".focusLT.dl"),
      focusright_dl = dl_new_list(256, 0, 0, name .. ".focusRT.dl"),
      focus_box = box,
      focus_time = 0,  -- blinking time

      event_table = { },
      flags  = flags or {}
   }

   dl_set_active(dial.dl, nil) -- hide it while we draw

   for _, dl in { dial.focusup_dl, dial.focusdown_dl, dial.focusleft_dl, dial.focusright_dl } do
      dl_sublist(dial.dl, dl)
   end
   
   -- draw surrounding box
   gui_dialog_box_draw(dial.dl, box, z, gui_box_bcolor, gui_box_color1, gui_box_color2, dial.flags.transparent)
   
   -- draw the focus cursor
   local focus_dl
   for _, focus_dl in 
      { dial.focusup_dl, dial.focusdown_dl, 
      dial.focusleft_dl, dial.focusright_dl } do
      dl_draw_box(focus_dl, { 0, 0,  1, 1 }, 
		  z+99, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })
   end
   
   if text then
      mode = mode or { }
      mode.x = mode.x or "left"
      mode.y = mode.y or "upout"
      mode.font_h = mode.font_h or 14
      gui_label(dial, text, mode)
   end
   
   evt_app_insert_first(owner, dial)


   if gui_apparate then
      gui_apparate(dial)
   else
      dl_set_active(dial.dl, 1)
   end

   
   -- disconnect joypad for main app
--   cond_connect(nil)
   
   return dial
end



-- button
function gui_button_shutdown(app)
end

function gui_button_handle(app, evt)
   local key = evt.key

   local f = app.event_table[key]
   if key == evt_shutdown_event then
      if f then 
	 -- still call shutdown user response before standard shutdown
	 f(app, evt) 
      end
      gui_button_shutdown(app)
      return evt -- pass the shutdown event to next app
   end

   if f then
      return f(app, evt)
   end

   -- $$$ added by ben for vmu display
   if vmu_set_text then
      if key == gui_focus_event and evt.app == app then
	 vmu_set_text("button:     " .. app.name)
      elseif key == gui_unfocus_event and evt.app == app then
	 vmu_set_text(nil)
      end
   end

   return evt
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_button(owner, box, text, mode, z, name)
   local app

   z = gui_guess_z(owner, z) + 10
   app = { 
      name = name or "gui_button",
      version = "0.9",

      handle = gui_button_handle,
      update = gui_button_update,

      dl = owner.dl,
      box = box,
      z = z,

      event_table = { },
      flags = { }
   }

   --dl_draw_box(app.dl, app.box, z, gui_button_color1, gui_button_color2)
   gui_button_box_draw(app.dl, app.box, z,
		       gui_button_bcolor, gui_button_color1, gui_button_color2)

   if text then
      gui_label(app, text, mode)
   end

   evt_app_insert_last(owner, app)

   return app
end



--- Input GUI shutdown.
--
function gui_input_shutdown(app)
   if app then
      app.input_dl = nil
   end
end

gui_input_edline_set = {
   [KBD_KEY_HOME] = 1,
   [KBD_KEY_END] = 1,
   [KBD_KEY_LEFT] = 1,
   [KBD_KEY_RIGHT] = 1,
   [KBD_KEY_DEL] = 1,
   [KBD_BACKSPACE] = 1,
}

--- Input GUI event handler.
--
function gui_input_handle(app, evt)
   local key = evt.key

   if key == evt_shutdown_event then
      gui_input_shutdown(app)
      return evt -- pass the shutdown event to next app
   end

   local f = app.event_table[key]
   if f then
      return f(app, evt)
   end
   
   if not app.prev then
      if ((key >= 32 and key < 128) or gui_input_edline_set[key]) then
	 app.input,app.input_col = zed_edline(app.input,app.input_col,key)
	 gui_input_display_text(app)
	 return nil
      elseif gui_keyconfirm[key] then
	 evt_send(app.owner, { key=gui_input_confirm_event, input=app })
	 return nil
      elseif gui_keycancel[key] then
	 gui_input_set(app,strsub(app.input,1,app.input_col-1))
	 return nil
      end
   end

   if key == gui_focus_event and evt.app == app then
      gui_input_display_text(app) -- $$$ display vmu
      if ke_set_active then
	 ke_set_active(1)
	 return nil
      end
   end
   if key == gui_unfocus_event and evt.app == app then
      if ke_set_active then
	 ke_vmu_prefix = nil
	 ke_vmu_suffix = nil
	 ke_set_active(nil)
	 return nil
      elseif vmu_set_text then
	 vmu_set_text(nil)
      end
   end
   
   return evt
end

function gui_input_display_text(app)
   if not app.input then
      return
   end
   local w, h = dl_measure_text(app.input_dl, app.input)
   local box=app.input_box
   local x = box[1]
   local y = (box[2] + box[4] - h) / 2
   local z = app.z + 1
   local maxw,maxh = box[3]-box[1], box[4]-box[2]
   local maxlines

   -- Compute substrings (before and after cursor point)
   local prefix = strsub(app.input, 1, app.input_col-1)
   local suffix = strsub(app.input, app.input_col)
   -- Get dimension of before cursor strings
   w, h = dl_measure_text(app.input_dl, prefix)

   -- ensure to see the enought to get the cursor inside
   maxw = 0.75 * maxw
   if w > maxw then
      x = x - w + maxw
   end

   dl_clear(app.input_dl)
-- $$$ ben : clipping fucks the measure text up !
   dl_set_clipping(app.input_dl,box[1],box[2],box[3],box[4])
   dl_draw_text(app.input_dl, x, y, z, gui_text_color, app.input)

   if not app.owner or gui_is_focus(app) then
      if ke_set_active then
	 ke_vmu_prefix = prefix
	 ke_vmu_suffix = suffix
      elseif vmu_set_text then
	 vmu_set_text("input :     " .. strsub(prefix,-24))
      end
   end

   dl_draw_box(app.input_dl, x+w+1, y, x+w+3, y+h, z,
	       gui_input_cursor_color1, gui_input_cursor_color2)
   --	print (x+w, y, x+w+2, y+h)
end

-- set the input text
function gui_input_set(app, string, col)
   if not string then
      string = ""
   end
   if not col then
      col = strlen(string)+1
   end
   app.input = string
   app.input_col = col
   gui_input_display_text(app)
end

-- insert text
function gui_input_insert(app, string, col)
   if not string or string == "" then return end
   if col then app.input_col = col end
   local i,len
   len = strlen(string)
   for i=1, len, 1 do
      app.input, app.input_col = zed_edline(app.input, app.input_col,
					    strbyte(string,i))
   end
   gui_input_display_text(app)
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_input(owner, box, text, mode, string, z)
   local app

   z = gui_guess_z(owner, z)
   
   app = { 
      
      name = "gui_input",
      version = "0.9",
      
      handle = gui_input_handle,
      update = gui_input_update,
      
      dl = owner.dl,
      box = box,
      input_box = box + { 2, 2, -2, -2 },
      z = z,
      
      event_table = { },
      flags = { },
      
      input_dl = dl_new_list(1024, 1, 0 , "input["..owner.name.."]")
      
   }

   dl_sublist(app.dl, app.input_dl)
   
   gui_input_box_draw(app.dl, app.box, z, gui_input_color1, gui_input_color2,
		   app.flags.transparent)
   --dl_draw_box(app.dl, app.box, z, gui_input_color1, gui_input_color2)
   
   
   if text then
      mode = mode or { }
      mode.x = mode.x or "left"
      mode.y = mode.y or "upout"
      gui_label(app, text, mode)
   end
   
   evt_app_insert_last(owner, app)

   -- $$$ ben : move this after insert becoz I need to know if focusing
   -- for vmu display
   gui_input_set(app, string)
   
   -- if we are the focused widget, then show the keyboard
   if app.sub == app and ke_set_active then
      ke_set_active(1)
   end
   
   return app
end

function gui_text_set(app, text)
   if not app then return end
   dl_clear(app.dl)
   dl_draw_box(app.dl, app.box, z, {0.1, 1, 1, 1} , {0.15, 1, 1, 1})
   if text and strlen(text) > 0 then
      gui_label(app, text, app.mode)
   end
   dl_set_active(app.dl,1)
end

function gui_destroy_text(app)
   if app then
      dl_set_active(app.dl)
   end
end

function gui_new_text(owner, box, text, mode, z)
   local app

   z = gui_guess_z(owner, z)

   app = { 
      name = "gui_text",
      version = "1.0",
      handle = gui_minimal_handle,
      dl = dl_new_list(1024,nil,nil,nil,"text["..owner.name.."]"),
      box = box,
      z = z,
      event_table = { },
      flags = { inactive = 1 },
      mode = mode
   }
   gui_text_set(app, text)
   evt_app_insert_last(owner, app)
   return app
end

-- Create a dialog child
function gui_new_children(owner, name, handle, box, mode, z)
   local app
   
   z = gui_guess_z(owner, z)
   
   local oname = owner.name or "???"
   name = name or "gui_child"
   
   if not handle then
      handle = gui_minimal_handle
   end
   
   app = { 
      name = name,
      handle = handle,
      dl = dl_new_list(1024, nil,nil,nil, name.."["..oname.."]"),
      box = box,
      z = z,
      event_table = { },
      flags = {},
      mode = mode
   }
   evt_app_insert_last(owner, app)
   return app
end

-- display justified text into given box
function gui_justify(dl, box, z, color, text, mode)

   if tag(text) == tt_tag then
      mode = text
   else
      if not mode then
	 mode = { }
      end
      mode.x = mode.x or "center"
      mode.y = mode.y or "center"
      mode.box = box
      mode.z = z
      mode.color = color
      mode = tt_build(text, mode)
   end

   tt_draw(mode)

   dl_sublist(dl, mode.dl)

   return mode.total_w, mode.total_h

end



--- ask a question (given as a tagged text) and propose given answers
--- (array of tagged text), with given optional box width (default to 300) and
--- dialog box label
function gui_ask(question, answers, width, label)

   if type(answers) == "string" then
      answers = { answers }
   elseif type(answers) ~= "table" then
      print("[gui_ask] : bad answer type.")
      return
   end

   width = width or 300

   local text = '<dialog guiref="dialog" x="center" name="gui_ask" icon="daemon"'

   if label then
      text = text..' label="'..label..'"'
   end
   if width then
      text = text..format(' hint_w="%d"', width)
   end
   text = text..'>'

   text = text..'<vspace h="16">'

   text = text..(question or "")

   text = text..'<vspace h="16">'

   local i
   if type(answers) == "table" then
      for i=1, getn(answers), 1 do
	 text = text..'<hspace w="16">'
	 text = text.. format('<button guiref="%d" name="%s">%s</button>',
			      i, tostring(answers[i]), tostring(answers[i]))
      end
   end
   text = text..'<hspace w="16">'

   text = text..'<vspace h="8">'

   text = text..'</dialog>'

--   print(text)
   local tt = tt_build(text, {
			  x = "center",
			  y = "center",
			  box = { 0, 20, 640, 460 },
		       }
		    )
   
   tt_draw(tt)

   tt.guis.dialog.event_table[evt_shutdown_event] =
      function(app) app.answer = 1 end

   for i=1, getn(answers), 1 do
      tt.guis.dialog.guis[format("%d", i)].event_table[gui_press_event] =
	 function(app, evt)
	    evt_shutdown_app(app.owner)
	    app.owner.answer = %i
	    return evt
	 end
   end

   while not tt.guis.dialog.answer do
      evt_peek()
   end

   return tt.guis.dialog.answer
   
end


--- Ask a question with two possible answers (default is yes/no) with value 1 and 2
function gui_yesno(question, width, label, yes, no)
   yes = yes or "Yes"
   no = no or "No"
   return gui_ask(question, { yes..'<img name="apply" src="stock_button_apply.tga" scale="1.5">', no..'<img name="cancel" src="stock_button_cancel.tga" scale="1.5">' }, width, label)
end


-- add a label to a gui item
function gui_label(app, text, mode)
   --	gui_justify(app.dl, app.box + gui_text_shiftbox, app.z+1, gui_text_color, text, mode)
   return gui_justify(app.dl, app.box, app.z+10, gui_text_color, text, mode)
   -- TODO use an optional mode.boxcolor and render a box around text if it is set ...
end


--- scrolling message in a box
function gui_scrolltext(dl, msg, txtcolor, bkgcolor, z)
   if type(msg) ~= "string" then return end
   if type(dl_new_list) ~= "function" then
      return
   end
   if tag(dl) ~= dl_tag then
      dl = dl_new_list(strlen(msg) + 512)
   end
   dl_clear(dl)
   if tag(dl) ~= dl_tag then
      return
   end
   txtcolor = txtcolor or gui_text_color
   bkgcolor = bkgcolor or {0.8,0.2,0.2,0.4}
   z = z or 400

   local size = 16

   dl_text_prop(dl,0,size)
   local w,h = dl_measure_text(dl, msg, 1 ,size)
   w = min(w * 1.5, 512)

   local x,y = (640-w) * 0.5, (480-h) * 0.5

   local c = bkgcolor
   gui_dialog_box_draw(dl, {x-8, y-8,x+w+8, y+h+3+10}, z-10,
		       gui_box_bcolor, gui_box_color1, gui_box_color2)
--   dl_draw_box1(dl, x, y,x+w, y+h+3, z-10,
--		c[1],c[2],c[3],c[4])

   dl_set_clipping(dl, x, y-3, x+w, y+h+3)

   c = txtcolor
   dl_draw_scroll_text(dl, x, y, z,
		       c[1],c[2],c[3],c[4], msg,
		       w, 1.5, 1)
   dl_set_active(dl,1)
   return dl
end


--
--- Shutdown the GUI.
--
function gui_shutdown()
   gui_curz = 1000
end

--
--- Initialize the GUI.
--
function gui_init()
   gui_shutdown()
   gui_curz = 1000
   gui_press_event		= evt_new_code()
   gui_cancel_event		= evt_new_code()
   gui_select_event		= evt_new_code()
   gui_menu_event		= evt_new_code()
   gui_focus_event		= evt_new_code()
   gui_unfocus_event	        = evt_new_code()
   gui_input_confirm_event	= evt_new_code()
   gui_item_confirm_event	= evt_new_code()
   gui_item_cancel_event	= evt_new_code()
   gui_item_change_event	= evt_new_code()
   gui_color_change_event	= evt_new_code() -- arg:color
end

gui_init()

--
--- @}
---

return 1
