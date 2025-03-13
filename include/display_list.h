/**
 * @ingroup   dcplaya_display_list
 * @file      display_list.h
 * @author    vincent penne
 * @author    benjamin gerard
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id$
 */

#ifndef _DISPLAY_LIST_H_
#define _DISPLAY_LIST_H_

/** @defgroup  dcplaya_display_list  Display list system.
 *  @ingroup   dcplaya_devel
 *  @brief     display list system
 *
 *  A display list is a list that contains commands to be executed every frame.
 *  Commands in a list are execurted sequentially in the order of insertion.
 *  Internally each display list contains a heap which may change when the list
 *  enlarges that why display list commands MUST NOT use pointers to something
 *  inside the command data (in the heap). Display lists are thread safe.
 *
 *  The display list execution occurs in two passes :
 *  -# Opaque rendering pass.
 *  -# Transparent rendering pass.
 *
 *  There is two types of display list :
 *  -# Main display list (main-list).
 *  -# Sub display list (sub-list).
 *
 *  The display list system will only executes main-lists. As the order
 *  display list execution order is indetermined, the graphic context should
 *  be set at the beginning of each main-list. Anyway it is resetted to
 *  default values before each main-list execution.
 *
 *  Sub-lists could be executed via the sub-list command from either a
 *  main-list or another sub-list.
 *
 * @author    vincent penne
 * @author    benjamin gerard
 *
 * @{
 */

#include <arch/spinlock.h>
#include <sys/queue.h>
#include "matrix.h"

#include "draw/clipping.h"
#include "draw/color.h"

/** Display list color type.
 */
typedef draw_color_t dl_color_t;

/** Display list clipping box type.
 */
typedef draw_clipbox_t dl_clipbox_t;

struct dl_command;

/** Constant for errorneous dl_comid_t. */
#define DL_COMID_ERROR   -1

/** Display list command error codes enumeration.
 */
typedef enum  {
  DL_COMMAND_OK    = 0, /**< Command success.                                */
  DL_COMMAND_BREAK = 1, /**< Command asks to break display list execution.   */
  DL_COMMAND_SKIP  = 2, /**< Command asks to skip next display list command. */
  DL_COMMAND_ERROR = -1 /**< Command failed.                                 */
} dl_code_e;

/** Display list render context.
 *
 *  A display list render context contains information needed by display
 *  list commands render functions. It consists on current color and current
 *  transformation matrix.
 */
typedef struct
{
  matrix_t   trans; /**< Transformation. */
  dl_color_t color; /**< Color.          */
} dl_context_t;

/** Display list run context.
 *
 *  A display list run context contains information needed by the sub-list
 *  command. It contains information on the way that colors and transformation
 *  must be inherited from calling display-list and which graphic properties
 *  must be restored at the end of the sub-list execution.
 */
typedef struct {
  int color_inherit:2; /**< Color heritage mode.          */
  int trans_inherit:2; /**< Transformation heritage mode. */
  int gc_flags:24;     /**< Set of GC_RESTORE flags.      */
} dl_runcontext_t;
 
/** Function type to render in opaque list or transparent list ;
 *  its argument is the pointer on the command that contained it.
 */
typedef dl_code_e (*dl_command_func_t)(void *, dl_context_t *);

/** Display list command identifier.
 */
typedef int dl_comid_t;

/** Display list command header structure.
 */
typedef struct {
  dl_comid_t next_id;                   /**< Next command in list.           */
  /** Display list command flags. */
  union {
    /** Fake struct to hold bit field. */
    struct {                            
      unsigned int inactive : 1;        /**< Command is inactive.            */
    };
    int all;                            /**< Access all flags.               */
  } flags;
	
  dl_command_func_t render_opaque;      /**< Opaque rendering function.      */
  dl_command_func_t render_transparent; /**< Transparent rendering function. */
} dl_command_t;

/** @name Display list flags.
 *  @{
 */
#define DL_INHERIT_PARENT 0 /**< effective = parent       */
#define DL_INHERIT_LOCAL  1 /**< effective = local        */
#define DL_INHERIT_ADD    2 /**< effective = parent+local */
#define DL_INHERIT_MUL    3 /**< effective = parent*local */
/**@}*/

/** Display list structure.
 */
typedef struct dl_list {
  LIST_ENTRY(dl_list) g_list;  /**< linked list entry                   */

  const char * name;           /**< Display list name (DEBUG mode only) */

  /** Display list flags. */
  struct {
	unsigned int active:1; /**< Is active.                          */
	unsigned int type:2;   /**< type of list [MAIN,SUB,DEAD]        */
  } flags;

  volatile int refcount;       /**< Reference counter.                  */

  matrix_t   trans;            /**< Current transform matrix.           */
  dl_color_t color;            /**< Current color.                      */

  int n_commands;              /**< Number of commands in command list. */
  dl_comid_t first_comid;      /**< First command.                      */
  dl_comid_t last_comid;       /**< Last command.                       */

  char * heap;                 /**< Command heap.                       */
  int    heap_size;            /**< Size of the heap.                   */
  int    heap_pos;             /**< Position in heap.                   */

} dl_list_t;

/** Heap transfert struct. */
typedef struct {
  dl_list_t *dl;    /**< Display list of this block. */
  dl_comid_t start; /**< Starting offset in heap.    */
  int end;          /**< Ending offset in heap.      */
  int cur;          /**< Current copy position.      */
} dl_heap_transfert_t;

typedef LIST_HEAD(dl_lists, dl_list) dl_lists_t;

/** @name Display list system initialization.
 */

/** Initialize the display list system. */
int dl_init(void);

/** Shutdown the display list system. */
int dl_shutdown(void);

/**@}*/


/** @name Display list management.
 *  @{
 */

/** Create a new display list.
  *
  * @param  heapsize  Size of the command heap (0 to get a default size of 1Kb)
  * @param  active    Initial active state
  * @param  sub       0:main-list, <>0:sub-list
  * @param  name      name of display list (debug purpose !)
  *
  * @return pointer to the created display list.
  * @retval 0 error.
  */
dl_list_t * dl_create(int heapsize, int active, int sub, const char *name);

/** Destroy the given display list (Does not ckeck the reference count.
 *  @see dl_dereference().
 */
void dl_destroy(dl_list_t * dl);

/** Reference a display list.
 *  @see dl_dereference().
 */
void dl_reference(dl_list_t * dl);

/** Dereference a display list and destroy it if there is no more reference
 *  on it.
 *  @return Number of remaining reference.
 *  @see dl_reference().
 *  @see dl_destroy_list().
 */
int dl_dereference(dl_list_t * dl);

/**@}*/


/** @name Display list control.
 *  @{
 */

/** Query active state of given display list.
 *
 *  @param  dl  Display list
 *
 *  @return   Display list active state.
 *  @retval 0 Display list is inactive.
 *  @retval 1 Display list is active.
 */
int dl_get_active(dl_list_t * dl);

/** Change active state of a display list.
 *
 *  @param  dl       Display list
 *  @param  active   new active state
 *
 *  @return   Display list preivious active state.
 *  @retval 0 Display list was inactive.
 *  @retval 1 Display list was active.
 */
int dl_set_active(dl_list_t * dl, int active);

/** Change active state of two display list.
 *
 *   The dl_set_active2() is an atomic function for changing the active state
 *   of two diaply list. It is very useful to perform a double-buffering
 *   emulation.
 *
 *  @param  l1      First display list.
 *  @param  l2      Second display list.
 *  @param  active  Bit 0 is first list new active state.
 *                  Bit 1 is second list new active state.
 *  @return   Display lists preivious active state.
 *            Bit 0 is first list previous active state.
 *            Bit 1 is second list previous active state.
 */
int dl_set_active2(dl_list_t * l1, dl_list_t * l2, int active);

/**@}*/


/** @name Display list command functions.
 *
 *  Set of function of creating and removing display list commands.
 *
 *  How to add a command to a display list :
 *  - Allocate requested number of byte for the command in the display list
 *    heap with the dl_alloc() function. After this function the display list
 *    will be locked.
 *  - Fill command data in the returned memory area.
 *  - Insert the command to the display list with the dl_insert()
 *    function. This will unlock the display list.
 *
 *  @{
 */

/** Allocate requested number of byte in the display list heap and keep
 *  display list locked until dl_insert() function call.
 *
 *  @param dl    Display list
 *  @param size  Requested number of by to allocate.
 *
 *  @return  Pointer into the display list memory heap allocated buffer.
 *  @retval  0  Failure.
 *
 *  @warning The display list is locked by the thread when the function returns
 *           successfully. You @b MUST to call dl_insert() function to unlock
 *           it.
 */
void * dl_alloc(dl_list_t * dl, size_t size);

/** Insert the given command into the display list.
 *
 *  @param dl        Display list locked by dl_alloc() function.
 *  @param pcom      Display list command allocated by dl_alloc() function.
 *  @param o_render  Opaque render callback.
 *  @param t_render  Transparent render callback.
 *
 *  @return  Command identifier
 *  @retval  DL_COMMID_ERROR failure
 */
dl_comid_t dl_insert(dl_list_t * dl, void * pcom,
		     dl_command_func_t o_render, dl_command_func_t t_render);

/** Clear the display list (i.e. reset the command list)
 *  @param dl        Display list locked by dl_alloc() function.
 */
void dl_clear(dl_list_t * dl);

/** Insert an NO-OPERATION command to display list.
 *
 *  @param   dl  Display list
 *
 *  @return  Command identifier
 *  @retval  DL_COMMID_ERROR failure
 */
dl_comid_t dl_nop_command(dl_list_t * dl);

/** Insert an SUB-LIST command to display list.
 *
 *  @param   dl        Display list
 *  @param   sublist   Sub display list to run (must be a sublist). 
 *  @param   rc        run context
 *
 *  @return  Command identifier
 *  @retval  DL_COMMID_ERROR failure
 *
 *  @see dl_create()
 */
dl_comid_t dl_sublist_command(dl_list_t * dl, dl_list_t * sublist,
			      const dl_runcontext_t * rc);

/**@}*/


/** @name Display list rendering functions.
 *
 *  These functions must be call by the rendering thread to perform display
 *  list rendering process in a given mode.
 *
 *  @{
 */
 
/** Render all active display list in opaque mode. */
void dl_render_opaque(void);

/** Render all active display list in transparent mode. */
void dl_render_transparent(void);

/**@}*/


/** @name Display list context access.
 *  @{
 */

/** Set display list global transformation.
 *  @param   dl        Display list
 *  @param   mat       New matrix
 */
void dl_set_trans(dl_list_t * dl, const matrix_t mat);

/** Get display list global transformation.
 *  @param   dl        Display list
 */
float * dl_get_trans(dl_list_t * dl);

/** Set display list global color.
 *  @param   dl        Display list
 *  @param   col       New color
 */
void dl_set_color(dl_list_t * dl, const dl_color_t * col);

/** Get display list global color.
 *  @param   dl        Display list
 */
dl_color_t * dl_get_color(dl_list_t * dl);

/**@}*/

/**@}*/

#endif /* #ifdef _DISPLAY_LIST_H_ */

