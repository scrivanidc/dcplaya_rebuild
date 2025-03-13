/**
 * @ingroup  dcplaya_exe_plugin_devel
 * @file     display.c
 * @author   vincent penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "dcplaya/config.h"
#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "obj3d.h"
#include "draw/gc.h"
#include "draw_object.h"

#include "display_driver.h"
#include "display_matrix.h"

#include "sysdebug.h"

/* display_matrix.c */
DL_FUNCTION_DECLARE(init_matrix_type);
DL_FUNCTION_DECLARE(shutdown_matrix_type);
DL_FUNCTION_DECLARE(set_trans);
DL_FUNCTION_DECLARE(get_trans);
DL_FUNCTION_DECLARE(mat_new);
DL_FUNCTION_DECLARE(mat_mult);
DL_FUNCTION_DECLARE(mat_mult_self);
DL_FUNCTION_DECLARE(mat_rotx);
DL_FUNCTION_DECLARE(mat_roty);
DL_FUNCTION_DECLARE(mat_rotz);
DL_FUNCTION_DECLARE(mat_scale);
DL_FUNCTION_DECLARE(mat_trans);
DL_FUNCTION_DECLARE(mat_li);
DL_FUNCTION_DECLARE(mat_co);
DL_FUNCTION_DECLARE(mat_el);
DL_FUNCTION_DECLARE(mat_dim);
DL_FUNCTION_DECLARE(mat_stat);
DL_FUNCTION_DECLARE(mat_dump);

/* display_text.c */
DL_FUNCTION_DECLARE(draw_text);
DL_FUNCTION_DECLARE(draw_scroll_text);
DL_FUNCTION_DECLARE(measure_text);
DL_FUNCTION_DECLARE(text_prop);

/* display_box.c */
DL_FUNCTION_DECLARE(draw_box1);
DL_FUNCTION_DECLARE(draw_box2);
DL_FUNCTION_DECLARE(draw_box4);

/* display_clipping.c */
DL_FUNCTION_DECLARE(set_clipping);
DL_FUNCTION_DECLARE(get_clipping);

/* display_triangle.c */
DL_FUNCTION_DECLARE(draw_triangle);

/* display_strip.c */
DL_FUNCTION_DECLARE(draw_strip);

/* display_texture.c */
DL_FUNCTION_DECLARE(tex_new);
DL_FUNCTION_DECLARE(tex_destroy);
DL_FUNCTION_DECLARE(tex_get);
DL_FUNCTION_DECLARE(tex_exist);
DL_FUNCTION_DECLARE(tex_info);

/* display_commands.c */
DL_FUNCTION_DECLARE(nop);
DL_FUNCTION_DECLARE(sublist);
DL_FUNCTION_DECLARE(gc_push);
DL_FUNCTION_DECLARE(gc_pop);

/* display_color.c */
DL_FUNCTION_DECLARE(draw_colors);

/* display list LUA interface */
int dl_list_tag;

DL_FUNCTION_DECLARE(dl_gc);

static void lua_init_dl_type(lua_State * L)
{
  lua_getglobal(L,"dl_tag");
  dl_list_tag = lua_tonumber(L,1);
  if (! dl_list_tag) {
    dl_list_tag = lua_newtag(L);
    lua_pushnumber(L, dl_list_tag);
    lua_setglobal(L,"dl_tag");
  }
  lua_pushcfunction(L, lua_dl_gc);
  lua_settagmethod(L, dl_list_tag, "gc");

  /* Initialize an empty list of display list */
  /*   lua_dostring(L,  */
  /* 	       "\n dl_lists = { }" */
  /* 	       "\n init_display_driver = nil" */
  /* 	       ); */
}

static void lua_shutdown_dl_type(lua_State * L)
{
  /* Destroy all allocated display lists */
  /*   lua_dostring(L,  */
  /* 	       "\n if type(dl_lists)==[[table]] then " */
  /* 	       "\n   while next(dl_lists) do" */
  /* 	       "\n     dl_destroy_list(next(dl_lists))" */
  /* 	       "\n   end" */
  /* 	       "\n end" */
  /* 	       "\n dl_lists = { }" */
  /* 	       "\n init_display_driver = nil" */
  /* 	       ); */
  /*   dl_list_tag = -1; */
}

static int lua_new_list(lua_State * L)
{
  dl_list_t * l;
  int heapsize = lua_tonumber(L, 1);
  int active   = lua_tonumber(L, 2);
  int sub      = lua_tonumber(L, 3);
  const char * name = lua_tostring(L, 4);

  /*   if (dl_list_tag < 0) { */
  /*     /\* try to initialize it *\/ */
  /*     lua_dostring(L, DRIVER_NAME"_driver_init()"); */

  /*     if (dl_list_tag < 0) { */
  /*       printf("display driver not initialized !"); */
  /*       return 0; */
  /*     } */
  /*   } */
  ///$$$
  //printf("Creating new %s-list %d %d\n", sub?"sub":"main",heapsize, active);
  lua_settop(L, 0);
  l = dl_create(heapsize, active, sub, name);
  if (l) {
    driver_reference(&display_driver);

    /*     /\* insert the new display list into list of all display lists */
    /* 	   (used in shutdown) *\/ */
    /*     lua_getglobal(L, "dl_lists"); */
    /*     lua_pushusertag(L, l, dl_list_tag); */
    /*     lua_pushnumber(L, 1); */
    /*     lua_settable(L, 1); */

    /* return the display list to the happy user */
    lua_settop(L, 0);
    lua_pushusertagsz(L, l, dl_list_tag, sizeof(*l));
    return 1;
  }

  return 0;
}


DL_FUNCTION_START(dl_gc)
{
  if (!dl_dereference(dl)) {
    driver_dereference(&display_driver);
  }
  return 0;
}
DL_FUNCTION_END()

     DL_FUNCTION_START(destroy_list)
{
  printf("destroying list [%p,%s] : Obsolete !! \n", dl, dl->name);

  /*   lua_settop(L, 0); */

  /*   /\* remove the display list from list of all display lists (used in shutdown) *\/ */
  /*   lua_getglobal(L, "dl_lists"); */
  /*   lua_pushusertag(L, dl, dl_list_tag); */
  /*   lua_pushnil(L); */
  /*   lua_settable(L, 1); */

  /*   dl_destroy(dl); */
  return 0;
}
DL_FUNCTION_END()


DL_FUNCTION_START(heap_size)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl->heap_size);
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(heap_used)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl->heap_pos);
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(info)
{
  lua_settop(L, 0);
  {
    const char * name = dl->name;
    const int flags = *((int*)&dl->flags);
    int refcount = dl->refcount;
    int n_commands = dl->n_commands;
    int heap_size = dl->heap_size;
    int heap_pos = dl-> heap_pos;

    lua_newtable(L);

    lua_pushstring(L,"name");
    lua_pushstring(L,name);
    lua_settable(L,1);

    lua_pushstring(L,"flags");
    lua_pushnumber(L,flags);
    lua_settable(L,1);

    lua_pushstring(L,"refcount");
    lua_pushnumber(L,refcount);
    lua_settable(L,1);

    lua_pushstring(L,"n");
    lua_pushnumber(L,n_commands);
    lua_settable(L,1);

    lua_pushstring(L,"size");
    lua_pushnumber(L,heap_size);
    lua_settable(L,1);

    lua_pushstring(L,"used");
    lua_pushnumber(L,heap_pos);
    lua_settable(L,1);
  }

  return 1;
}
DL_FUNCTION_END()


DL_FUNCTION_START(get_active)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl_get_active(dl));
  return 1;
}
DL_FUNCTION_END()


DL_FUNCTION_START(set_active)
{
  int active = lua_tonumber(L, 2);
  lua_settop(L, 0);
  lua_pushnumber(L, dl_set_active(dl, active));
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(set_active2)
{
  int active = lua_tonumber(L, 3);
  dl_list_t * dl2;

  if (lua_tag(L, 2) != dl_list_tag) {
    printf("dl_set_active2 : 2nd parameter is not a list\n");
    return 0;
  }
  dl2 = lua_touserdata(L, 2);
  lua_settop(L, 0);
  lua_pushnumber(L, dl_set_active2(dl, dl2, active));
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(clear)
{
  dl_clear(dl);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(set_color)
{
  int i;
  float col[4];
  const float * dcol = (const float *) dl_get_color(dl);

  for (i=0; i<4; ++i) {
    col[i] = (lua_type(L,i+2) == LUA_TNUMBER)
      ? lua_tonumber(L, i+2)
      : dcol[i];
  }
  dl_set_color(dl,(const dl_color_t *) col);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_color)
{
  dl_color_t * col;
  col = dl_get_color(dl);
  lua_settop(L, 0);
  lua_pushnumber(L, col->a);
  lua_pushnumber(L, col->r);
  lua_pushnumber(L, col->g);
  lua_pushnumber(L, col->b);
  return 4;
}
DL_FUNCTION_END()


static int display_init(any_driver_t *d)
{
  return display_matrix_init();
}

static int display_shutdown(any_driver_t * d)
{
  SDDEBUG("[%s] : refcount = %d\n", __FUNCTION__, d->count);
  return display_matrix_shutdown();
}

static driver_option_t * display_options(any_driver_t * d, int idx,
					 driver_option_t * o)
{
  return o;
}


static int init;

static int lua_init(lua_State * L)
{
  if (init) {
    printf("display_driver_init called more than once !\n");
    return 0;
  }
  init = 1;
  printf("display_driver_init called\n");
  lua_init_matrix_type(L);
  lua_init_dl_type(L);
  return 0;
}

static int lua_shutdown(lua_State * L)
{
  if (!init) {
    printf("display_driver_shutdown called more than once !\n");
    return 0;
  }
  init = 0;
  printf("display_driver_shutdown called\n");

  lua_shutdown_dl_type(L);
  lua_shutdown_matrix_type(L);

  return 0;
}

static luashell_command_description_t display_commands[] = {
  /* internal init commands */
  {
    /* long name, short name, topic */
    DRIVER_NAME"_driver_init",0,"display-list",
    /* usage */
    DRIVER_NAME"_driver_init() : "
    "INTERNAL ; initialize lua side display driver.",
    /* function */
    SHELL_COMMAND_C, lua_init
  },
  {
    /* long name, short name, topic */
    DRIVER_NAME"_driver_shutdown",0,0,
    /* usage */
    DRIVER_NAME"_driver_shutdown() : "
    "INTERNAL ; shutdown lua side display driver.",
    /* function */
    SHELL_COMMAND_C, lua_shutdown
  },
  /* General commands */
  {
    /* long name, short name, topic */
    "dl_new_list", "dl_new",0,
    /* usage */
    "dl_new_list([heapsize, [active, [sub-list] ] ]) : "
    "Create a new display list, return handle on display list.",
    /* function */
    SHELL_COMMAND_C, lua_new_list
  },
  {
    /* long name, short name, topic */
    "dl_destroy_list", "dl_destroy",0,
    /* usage */
    "dl_destroy_list(list) : "
    "Destroy the given list. ** OBSOLETE ! DO NOT USE **.",
    /* function */
    SHELL_COMMAND_C, lua_destroy_list
  },
  {
    /* long name, short name, topic */
    "dl_heap_size",0,0,
    /* usage */
    "dl_heap_size(list) : "
    "Get heap size in bytes.",
    /* function */
    SHELL_COMMAND_C, lua_heap_size
  },
  {
    /* long name, short name, topic */
    "dl_heap_used",0,0,
    /* usage */
    "dl_heap_used(list) : "
    "Get number of byte used in the heap.",
    /* function */
    SHELL_COMMAND_C, lua_heap_used
  },
  {
    /* long name, short name, topic */
    "dl_info",0,0,
    /* usage */
    "dl_info(list) : "
    "Get display info table.",
    /* function */
    SHELL_COMMAND_C, lua_info
  },
  /* Properties commands */
  {
    /* long name, short name, topic */
    "dl_set_active",0,0,
    /* usage */
    "dl_set_active(list,state) : "
    "Set active state and return old state.",
    /* function */
    SHELL_COMMAND_C, lua_set_active
  },
  {
    /* long name, short name, topic */
    "dl_set_active2",0,0,
    /* usage */
    "dl_set_active(list1,list2,state) : "
    "Set active state of 2 lists. Bit:0/1 respectively state of list1/list2.\n"
    "Return old states.",
    /* function */
    SHELL_COMMAND_C, lua_set_active2
  },
  {
    /* long name, short name, topic */
    "dl_get_active",0,0,
    /* usage */
    "dl_get_active(list) : get active state",
    /* function */
    SHELL_COMMAND_C, lua_get_active
  },
  {
    /* long name, short name, topic */
    "dl_set_color",0,0,
    /* usage */
    "dl_set_color(list, color) : set the global color",
    /* function */
    SHELL_COMMAND_C, lua_set_color
  },
  {
    /* long name, short name, topic */
    "dl_get_color",0,0,
    /* usage */
    "dl_get_color(list) : get the color",
    /* function */
    SHELL_COMMAND_C, lua_get_color
  },
  {
    /* long name, short name, topic */
    "dl_set_trans",0,0,
    /* usage */
    "dl_set_trans(list, matrix) : set the transformation",
    /* function */
    SHELL_COMMAND_C, lua_set_trans
  },
  {
    /* long name, short name, topic */
    "dl_get_trans",0,0,
    /* usage */
    "dl_get_trans(list) : get the transformation, return a matrix",
    /* function */
    SHELL_COMMAND_C, lua_get_trans
  },
  {
    /* long name, short name, topic */
    "dl_clear",0,0,
    /* usage */
    "dl_clear(list) : get clear",
    /* function */
    SHELL_COMMAND_C, lua_clear
  },
  /* color interface */
  {
    /* long name, short name, topic */
    "dl_draw_colors",0,0,
    /* usage */
    "DEPRECATED !!!\n"
    "dl_draw_colors(list, a, r, g, b [,a, r, g, b ...]) : "
    "Set up to 4 draw colors.",
    /* function */
    SHELL_COMMAND_C, lua_draw_colors
  },
  /* box interface */
  {
    /* long name, short name, topic */
    "dl_draw_box1",0,0,
    /* usage */
    "dl_draw_box(list, x1, y1, x2, y2, z, a, r, g, b) : "
    "Draw a flat box",
    /* function */
    SHELL_COMMAND_C, lua_draw_box1
  },
  {
    /* long name, short name, topic */
    "dl_draw_box",0,0,
    /* usage */
    "dl_draw_box(list, x1, y1, x2, y2, z,"
    " a1, r1, g1, b1, a2, r2, g2, b2, type) : draw gradiant box\n"
    " - type is gradiant orientation (0:diagonal, 1:horizontal 2:vertical",
    /* function */
    SHELL_COMMAND_C, lua_draw_box2
  },
  {
    /* long name, short name, topic */
    "dl_draw_box4",0,0,
    /* usage */
    "dl_draw_box(list, x1, y1, x2, y2, z, "
    "a1, r1, g1, b1, "
    "a2, r2, g2, b2, "
    "a3, r3, g3, b3, "
    "a4, r4, g4, b4) : draw a colored box",
    /* function */
    SHELL_COMMAND_C, lua_draw_box4
  },
  /* text interface */
  {
    /* long name, short name, topic */
    "dl_draw_text",0,0,
    /* usage */
    "dl_draw_text(list, x, y, z, a, r, g, b, string) : draw text",
    /* function */
    SHELL_COMMAND_C, lua_draw_text
  },
  {
    /* long name, short name, topic */
    "dl_draw_scroll_text",0,0,
    /* usage */
    "dl_draw_scroll_text(list, x, y, z, a, r, g, b, string,"
    " window, speed, pingpong) : draw scroll text",
    /* function */
    SHELL_COMMAND_C, lua_draw_scroll_text
  },
  {
    /* long name, short name, topic */
    "dl_measure_text",0,0,
    /* usage */
    "dl_measure_text(list | nil, string [,fontid [,size [,aspect ] ] ] ) :"
    " measure text. Default properties 0, 16, 1.",
    /* function */
    SHELL_COMMAND_C, lua_measure_text
  },
  {
    /* long name, short name, topic */
    "dl_text_prop",0,0,
    /* usage */
    "dl_text_prop(list [,fontid [,size [,aspect [,filter ] ] ] ]) : "
    "set text properties (nil keeps old value).",
    /* function */
    SHELL_COMMAND_C, lua_text_prop
  },
  /* triangle interface */
  {
    /* long name, short name, topic */
    "dl_draw_triangle",0,0,
    /* usage */
    "dl_draw_triangle(list, matrix | (vector[,vector,vector]) "
    "[, [ texture-id, ] [ opacity-mode ] ] ) : draw a triangle",
    /* function */
    SHELL_COMMAND_C, lua_draw_triangle
  },
  /* strip interface */
  {
    /* long name, short name, topic */
    "dl_draw_strip",0,0,
    /* usage */
    "dl_draw_strip(list, matrix | (vector,n) "
    "[, [ texture-id, ] [ opacity-mode ] ] ) : draw a strip",
    /* function */
    SHELL_COMMAND_C, lua_draw_strip
  },
  /* built-in command interface */
  {
    "dl_nop",0,0,                /* long name, short name, topic */
    /* usage */
    "dl_nop() : No operation.",
    SHELL_COMMAND_C, lua_nop    /* function */
  },
  {
    /* long name, short name, topic */
    "dl_sublist",0,0,
    /* usage */
    "dl_sublist(list, sublist [, color-op [, trans-op [, restore] ] ]) "
    " : draw a sub-list.\n"
    " <color-op> and <trans-op> are strings that respectively set the color"
    " and transformation heritage.\n"
    " default := MODULATE\n"
    " values := { PARENT, LOCAL, ADD, MODULATE }\n"
    "\n"
    " <restore> is a string containing characters that define which graphic"
    " context properties will be restored at the end of the sub-list"
    " execution. The string is parsed from left to right.\n"
    " Control characters may be uppercase to activate and lowercase"
    " to desactivate a graphic property restore stat.\n"
    " default := A (All)\n"
    " General-Properties\n"
    " - <A/a> : set/clear all properties.\n"
    " - <~>   : invert all\n"
    " Text-Properties\n"
    " - <T/t> : activate/desactivate all text properties\n"
    " - <S/s> : activate/desactivate text size\n"
    " - <F/s> : activate/desactivate text font face\n"
    " - <K/k> : activate/desactivate text color\n"
    " Color-Properties\n"
    " - <C/c> : activate/desactivate color table properties\n"
    " - <0/1/2/3> : toggle color #\n"
    " Clipping-Property\n"
    " -<B/b>  : activate/desactivate clipping box restoration.\n",
    /* function */
    SHELL_COMMAND_C, lua_sublist
  },
  {
    /* long name, short name, topic */
    "dl_gc_push",0,0,
    /* usage */
    "dl_gc_push([flags]) : Push current GC. Do not forget to call dl_gc_pop()"
    " later in your display list.\n"
    "See dl_sublist for flags parameter.",
    /* function */
    SHELL_COMMAND_C, lua_gc_push
  },
  {
    /* long name, short name, topic */
    "dl_gc_pop",0,0,
    /* usage */
    "dl_gc_pop([flags]) : Pop current GC. Call it after dl_gc_push()"
    " in your display list.\n"
    "See dl_sublist for flags parameter.",
    /* function */
    SHELL_COMMAND_C, lua_gc_pop
  },
  /* clipping interface */
  {
    /* long name, short name, topic */
    "dl_set_clipping",0,0,
    /* usage */
    "dl_set_clipping(list, x1, y1, x2, y2) : set clipping box",
    /* function */
    SHELL_COMMAND_C, lua_set_clipping
  },
  {
    /* long name, short name, topic */
    "dl_get_clipping",0,0,
    /* usage */
    "dl_get_clipping(list) : get clipping box {x1,y1,x2,y2}",
    /* function */
    SHELL_COMMAND_C, lua_get_clipping
  },
  /* texture interface */
  {
    /* long name, short name, topic */
    "tex_new",0,"texture",
    /* usage */
    "tex_new(filename) or tex_new(name,width,heigth,a,r,g,b) :\n"
    "Create a new texure. Returns texture identifier",
    /* function */
    SHELL_COMMAND_C, lua_tex_new
  },
  {
    /* long name, short name, topic */
    "tex_destroy",0,0,
    /* usage */
    "tex_destroy(texture-name|texture-id [,force] ) : "
    "Destroy a texture.",
    /* function */
    SHELL_COMMAND_C, lua_tex_destroy
  },
  {
    /* long name, short name, topic */
    "tex_get",0,0,
    /* usage */
    "tex_get(texture-name|texture-id) : "
    "Get texture identifier.",
    /* function */
    SHELL_COMMAND_C, lua_tex_get
  },
  {
    /* long name, short name, topic */
    "tex_exist",0,0,
    /* usage */
    "tex_exist(texture-name|texture-id) : "
    "Get texture identifier if it exist. This function works like "
    "text_get() excepted that it does not print error message.",
    /* function */
    SHELL_COMMAND_C, lua_tex_exist
  },
  {
    /* long name, short name, topic */
    "tex_info",0,0,
    /* usage */
    "tex_info(texture-name|texture-id) : "
    "Get texture info structure.",
    /* function */
    SHELL_COMMAND_C, lua_tex_info
  },
  /* matrix interface */
  {
    "mat_new",0,"matrix",                /* long name, short name, topic */
    "mat_new( [ mat | lines, [ columns ] ] ) : make a new matrix. "
    "Default lines and columns is 4. "
    "If mat is given the result is a copy of mat. "
    "If matrix dimension is 4x4 the result is an identity matrix "
    "else the result matrix is zeroed.",
    SHELL_COMMAND_C, lua_mat_new         /* function */
  },
  {
    "mat_trans",0,0,                      /* long name, short name, topic */
    "mat_trans(x, y, z) : make a translation matrix",
    SHELL_COMMAND_C, lua_mat_trans       /* function */
  },
  {
    "mat_rotx",0,0,                       /* long name, short name, topic */
    "mat_rotx(angle) : make rotation matrix around X axis",
    SHELL_COMMAND_C, lua_mat_rotx        /* function */
  },
  {
    "mat_roty",0,0,                       /* long name, short name, topic */
    "mat_roty(angle) : make rotation matrix around Y axis",
    SHELL_COMMAND_C, lua_mat_roty        /* function */
  },
  {
    "mat_rotz",0,0,                       /* long name, short name, topic */
    "mat_rotz(angle) : make rotation matrix around Z axis",
    SHELL_COMMAND_C, lua_mat_rotz        /* function */
  },
  {
    "mat_scale",0,0,                      /* long name, short name, topic */
    "mat_scale(x, y, z) : make scaling matrix with given coeficient",
    SHELL_COMMAND_C, lua_mat_scale       /* function */
  },
  {
    "mat_mult",0,0,                       /* long name, short name, topic */
    "mat_mult(mat1, mat2) : make matrix by apply mat1xmat2\n"
    "Where mat2 is 4x4 matrix.",
    SHELL_COMMAND_C, lua_mat_mult        /* function */
  },
  {
    "mat_mult_self",0,0,                  /* long name, short name, topic */
    "mat_mult([mat0,] mat1, mat2) : returns mat0=mat1xmat2 or mat1=mat1xmat2\n"
    "Where mat2 is 4x4 matrix.",
    SHELL_COMMAND_C, lua_mat_mult_self   /* function */
  },
  {
    "mat_li",0,0,                         /* long name, short name, topic */
    "mat_li(mat, l) : get matrix line",
    SHELL_COMMAND_C, lua_mat_li          /* function */
  },
  {
    "mat_co",0,0,                         /* long name, short name, topic */
    "mat_co(mat, c) : get matrix column",
    SHELL_COMMAND_C, lua_mat_co          /* function */
  },
  {
    "mat_el",0,0,                         /* long name, short name, topic */
    "mat_el(mat, l, c) : get matrix element",
    SHELL_COMMAND_C, lua_mat_el          /* function */
  },
  {
    /* long name, short name, topic */
    "mat_dim",0,0,
    /* usage */
    "mat_dim(mat) : get matrix dimension (line,columns)",
    /* function */
    SHELL_COMMAND_C, lua_mat_dim
  },
  {
    /* long name, short name, topic */
    "mat_stat",0,0,
    /* usage */
    "mat_stat([level]) : Display matrices debug info.",
    /* function */
    SHELL_COMMAND_C, lua_mat_stat
  },
  {
    /* long name, short name, topic */
    "mat_dump",0,0,
    /* usage */
    "mat_dump(mat,[level]) : Display matrix mat debug info.",
    /* function */
    SHELL_COMMAND_C, lua_mat_dump
  },
  {0},                                   /* end of the command list */
};

any_driver_t display_driver =
  {
    0,                     /**< Next driver                     */
    EXE_DRIVER,            /**< Driver type                     */
    0x0100,                /**< Driver version                  */
    DRIVER_NAME,           /**< Driver name                     */
    "Vincent Penne, "      /**< Driver authors                  */
    "Benjamin Gerard",
    "Graphical LUA "
    "extension provides "
    "functions for matrix"
    " and display list "
    " support."         ,  /**< Description                     */
    0,                     /**< DLL handler                     */
    display_init,          /**< Driver init                     */
    display_shutdown,      /**< Driver shutdown                 */
    display_options,       /**< Driver options                  */
    display_commands,      /**< Lua shell commands              */
  };
EXPORT_DRIVER(display_driver)
