/* Definitions and headers for communication with X protocol.
   Copyright (C) 1989, 1993-1994, 1998-2021 Free Software Foundation,
   Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef XTERM_H
#define XTERM_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

/* Include Xutil.h after keysym.h to work around a bug that prevents
   correct recognition of AltGr key in some X versions.  */

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <X11/Xatom.h>
#include <X11/Xresource.h>

#ifdef USE_X_TOOLKIT
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>	/* CoreP.h needs this */
#include <X11/CoreP.h>		/* foul, but we need this to use our own
				   window inside a widget instead of one
				   that Xt creates... */
#ifdef X_TOOLKIT_EDITRES
#include <X11/Xmu/Editres.h>
#endif

typedef Widget xt_or_gtk_widget;
#endif

#ifdef USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif /* USE_GTK */

#ifndef USE_GTK
#define GTK_CHECK_VERSION(i, j, k) false
#endif

#ifdef USE_GTK
/* Some definitions to reduce conditionals.  */
typedef GtkWidget *xt_or_gtk_widget;
#undef XSync
/* gdk_window_process_all_updates is deprecated in GDK 3.22.  */
#if GTK_CHECK_VERSION (3, 22, 0)
#define XSync(d, b) do { XSync ((d), (b)); } while (false)
#else
#define XSync(d, b) do { gdk_window_process_all_updates (); \
                         XSync (d, b);  } while (false)
#endif
#endif /* USE_GTK */

#ifdef USE_CAIRO
#include <cairo-xlib.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#endif

#ifdef HAVE_X_I18N
#include <X11/Xlocale.h>
#endif

#ifdef USE_XCB
#include <X11/Xlib-xcb.h>
#endif

#include "dispextern.h"
#include "termhooks.h"

INLINE_HEADER_BEGIN

/* Black and white pixel values for the screen which frame F is on.  */
#define BLACK_PIX_DEFAULT(f)					\
  BlackPixel (FRAME_X_DISPLAY (f), FRAME_X_SCREEN_NUMBER (f))
#define WHITE_PIX_DEFAULT(f)					\
  WhitePixel (FRAME_X_DISPLAY (f), FRAME_X_SCREEN_NUMBER (f))

/* The mask of events that text windows always want to receive.  This
   includes mouse movement events, since handling the mouse-font text property
   means that we must track mouse motion all the time.  */

#define STANDARD_EVENT_SET      \
  (KeyPressMask			\
   | ExposureMask		\
   | ButtonPressMask		\
   | ButtonReleaseMask		\
   | PointerMotionMask		\
   | StructureNotifyMask	\
   | FocusChangeMask		\
   | LeaveWindowMask		\
   | EnterWindowMask		\
   | VisibilityChangeMask)

#ifdef HAVE_X11R6_XIM
/* Data structure passed to xim_instantiate_callback.  */
struct xim_inst_t
{
  struct x_display_info *dpyinfo;
  char *resource_name;
};
#endif /* HAVE_X11R6_XIM */

/* Structure recording X pixmap and reference count.
   If REFCOUNT is 0 then this record is free to be reused.  */

struct x_bitmap_record
{
#ifdef USE_CAIRO
  cairo_pattern_t *stipple;
#endif
  Pixmap pixmap;
  bool have_mask;
  Pixmap mask;
  char *file;
  int refcount;
  /* Record some info about this pixmap.  */
  int height, width, depth;
};

#ifdef USE_CAIRO
struct x_gc_ext_data
{
#define MAX_CLIP_RECTS 2
  /* Number of clipping rectangles.  */
  int n_clip_rects;

  /* Clipping rectangles.  */
  XRectangle clip_rects[MAX_CLIP_RECTS];
};

extern cairo_pattern_t *x_bitmap_stipple (struct frame *, Pixmap);
#endif


struct color_name_cache_entry
{
  struct color_name_cache_entry *next;
  XColor rgb;
  char *name;
};

Status x_parse_color (struct frame *f, const char *color_name,
		      XColor *color);


/* For each X display, we have a structure that records
   information about it.  */

struct x_display_info
{
  /* Chain of all x_display_info structures.  */
  struct x_display_info *next;

  /* The generic display parameters corresponding to this X display. */
  struct terminal *terminal;

  /* This says how to access this display in Xlib.  */
  Display *display;

  /* A connection number (file descriptor) for the display.  */
  int connection;

  /* This is a cons cell of the form (NAME . FONT-LIST-CACHE).  */
  Lisp_Object name_list_element;

  /* Number of frames that are on this display.  */
  int reference_count;

  /* The Screen this connection is connected to.  */
  Screen *screen;

  /* Dots per inch of the screen.  */
  double resx, resy;

  /* The Visual being used for this display.  */
  Visual *visual;

  /* The colormap being used.  */
  Colormap cmap;

  /* Number of planes on this screen.  */
  int n_planes;

  /* Mask of things that cause the mouse to be grabbed.  */
  int grabbed;

  /* Emacs bitmap-id of the default icon bitmap for this frame.
     Or -1 if none has been allocated yet.  */
  ptrdiff_t icon_bitmap_id;

  /* The root window of this screen.  */
  Window root_window;

  /* Client leader window.  */
  Window client_leader_window;

  /* The cursor to use for vertical scroll bars.  */
  Cursor vertical_scroll_bar_cursor;

  /* The cursor to use for horizontal scroll bars.  */
  Cursor horizontal_scroll_bar_cursor;

  /* The invisible cursor used for pointer blanking.
     Unused if this display supports Xfixes extension.  */
  Cursor invisible_cursor;

  /* Function used to toggle pointer visibility on this display.  */
  void (*toggle_visible_pointer) (struct frame *, bool);

#ifdef USE_GTK
  /* The GDK cursor for scroll bars and popup menus.  */
  GdkCursor *xg_cursor;
#endif

  /* X Resource data base */
  XrmDatabase rdb;

  /* Minimum width over all characters in all fonts in font_table.  */
  int smallest_char_width;

  /* Minimum font height over all fonts in font_table.  */
  int smallest_font_height;

  /* Reusable Graphics Context for drawing a cursor in a non-default face. */
  GC scratch_cursor_gc;

  /* Information about the range of text currently shown in
     mouse-face.  */
  Mouse_HLInfo mouse_highlight;

  /* Logical identifier of this display.  */
  unsigned x_id;

  /* Default name for all frames on this display.  */
  char *x_id_name;

  /* The number of fonts opened for this display.  */
  int n_fonts;

  /* Pointer to bitmap records.  */
  struct x_bitmap_record *bitmaps;

  /* Allocated size of bitmaps field.  */
  ptrdiff_t bitmaps_size;

  /* Last used bitmap index.  */
  ptrdiff_t bitmaps_last;

  /* Which modifier keys are on which modifier bits?

     With each keystroke, X returns eight bits indicating which modifier
     keys were held down when the key was pressed.  The interpretation
     of the top five modifier bits depends on what keys are attached
     to them.  If the Meta_L and Meta_R keysyms are on mod5, then mod5
     is the meta bit.

     meta_mod_mask is a mask containing the bits used for the meta key.
     It may have more than one bit set, if more than one modifier bit
     has meta keys on it.  Basically, if EVENT is a KeyPress event,
     the meta key is pressed if (EVENT.state & meta_mod_mask) != 0.

     shift_lock_mask is LockMask if the XK_Shift_Lock keysym is on the
     lock modifier bit, or zero otherwise.  Non-alphabetic keys should
     only be affected by the lock modifier bit if XK_Shift_Lock is in
     use; XK_Caps_Lock should only affect alphabetic keys.  With this
     arrangement, the lock modifier should shift the character if
     (EVENT.state & shift_lock_mask) != 0.  */
  int meta_mod_mask, shift_lock_mask;

  /* These are like meta_mod_mask, but for different modifiers.  */
  int alt_mod_mask, super_mod_mask, hyper_mod_mask;

  /* Communication with window managers.  */
  Atom Xatom_wm_protocols;

  /* Kinds of protocol things we may receive.  */
  Atom Xatom_wm_take_focus;
  Atom Xatom_wm_save_yourself;
  Atom Xatom_wm_delete_window;

  /* Atom for indicating window state to the window manager.  */
  Atom Xatom_wm_change_state;

  /* Other WM communication */
  Atom Xatom_wm_configure_denied; /* When our config request is denied */
  Atom Xatom_wm_window_moved;     /* When the WM moves us.  */
  Atom Xatom_wm_client_leader;    /* Id of client leader window.  */

  /* EditRes protocol */
  Atom Xatom_editres;

  /* More atoms, which are selection types.  */
  Atom Xatom_CLIPBOARD, Xatom_TIMESTAMP, Xatom_TEXT, Xatom_DELETE,
  Xatom_COMPOUND_TEXT, Xatom_UTF8_STRING,
  Xatom_MULTIPLE, Xatom_INCR, Xatom_EMACS_TMP, Xatom_TARGETS, Xatom_NULL,
  Xatom_ATOM, Xatom_ATOM_PAIR, Xatom_CLIPBOARD_MANAGER;

  /* More atoms for font properties.  The last three are private
     properties, see the comments in src/fontset.h.  */
  Atom Xatom_PIXEL_SIZE, Xatom_AVERAGE_WIDTH,
  Xatom_MULE_BASELINE_OFFSET, Xatom_MULE_RELATIVE_COMPOSE,
  Xatom_MULE_DEFAULT_ASCENT;

  /* More atoms for Ghostscript support.  */
  Atom Xatom_DONE, Xatom_PAGE;

  /* Atoms used in toolkit scroll bar client messages.  */
  Atom Xatom_Scrollbar, Xatom_Horizontal_Scrollbar;

  /* Atom used in XEmbed client messages.  */
  Atom Xatom_XEMBED, Xatom_XEMBED_INFO;

  /* The frame (if any) which has the X window that has keyboard focus.
     Zero if none.  This is examined by Ffocus_frame in xfns.c.  Note
     that a mere EnterNotify event can set this; if you need to know the
     last frame specified in a FocusIn or FocusOut event, use
     x_focus_event_frame.  */
  struct frame *x_focus_frame;

  /* The last frame mentioned in a FocusIn or FocusOut event.  This is
     separate from x_focus_frame, because whether or not LeaveNotify
     events cause us to lose focus depends on whether or not we have
     received a FocusIn event for it.  */
  struct frame *x_focus_event_frame;

  /* The frame which currently has the visual highlight, and should get
     keyboard input (other sorts of input have the frame encoded in the
     event).  It points to the X focus frame's selected window's
     frame.  It differs from x_focus_frame when we're using a global
     minibuffer.  */
  struct frame *highlight_frame;

  /* The frame waiting to be auto-raised in XTread_socket.  */
  struct frame *x_pending_autoraise_frame;

  /* The frame where the mouse was last time we reported a ButtonPress event.  */
  struct frame *last_mouse_frame;

  /* The frame where the mouse was last time we reported a mouse position.  */
  struct frame *last_mouse_glyph_frame;

  /* The frame where the mouse was last time we reported a mouse motion.  */
  struct frame *last_mouse_motion_frame;

  /* The scroll bar in which the last X motion event occurred.  */
  struct scroll_bar *last_mouse_scroll_bar;

  /* Time of last user interaction as returned in X events on this display.  */
  Time last_user_time;

  /* Position where the mouse was last time we reported a motion.
     This is a position on last_mouse_motion_frame.  */
  int last_mouse_motion_x;
  int last_mouse_motion_y;

  /* Where the mouse was last time we reported a mouse position.
     This is a rectangle on last_mouse_glyph_frame.  */
  XRectangle last_mouse_glyph;

  /* Time of last mouse movement on this display.  This is a hack because
     we would really prefer that XTmouse_position would return the time
     associated with the position it returns, but there doesn't seem to be
     any way to wrest the time-stamp from the server along with the position
     query.  So, we just keep track of the time of the last movement we
     received, and return that in hopes that it's somewhat accurate.  */
  Time last_mouse_movement_time;

  /* The gray pixmap.  */
  Pixmap gray;

#ifdef HAVE_X_I18N
  /* XIM (X Input method).  */
  XIM xim;
  XIMStyles *xim_styles;
  struct xim_inst_t *xim_callback_data;
#endif

  /* A cache mapping color names to RGB values.  */
  struct color_name_cache_entry *color_names;

  /* If non-null, a cache of the colors in the color map.  Don't
     use this directly, call x_color_cells instead.  */
  XColor *color_cells;
  int ncolor_cells;

  /* Bits and shifts to use to compose pixel values on TrueColor visuals.  */
  int red_bits, blue_bits, green_bits;
  int red_offset, blue_offset, green_offset;

  /* The type of window manager we have.  If we move FRAME_OUTER_WINDOW
     to x/y 0/0, some window managers (type A) puts the window manager
     decorations outside the screen and FRAME_OUTER_WINDOW exactly at 0/0.
     Other window managers (type B) puts the window including decorations
     at 0/0, so FRAME_OUTER_WINDOW is a bit below 0/0.
     Record the type of WM in use so we can compensate for type A WMs.  */
  enum
    {
      X_WMTYPE_UNKNOWN,
      X_WMTYPE_A,
      X_WMTYPE_B
    } wm_type;


  /* Atoms that are drag and drop atoms */
  Atom *x_dnd_atoms;
  ptrdiff_t x_dnd_atoms_size;
  ptrdiff_t x_dnd_atoms_length;

  /* Extended window manager hints, Atoms supported by the window manager and
     atoms for setting the window type.  */
  Atom Xatom_net_supported, Xatom_net_supporting_wm_check;
  Atom *net_supported_atoms;
  int nr_net_supported_atoms;
  Window net_supported_window;
  Atom Xatom_net_window_type, Xatom_net_window_type_tooltip;
  Atom Xatom_net_active_window;

  /* Atoms dealing with EWMH (i.e. _NET_...) */
  Atom Xatom_net_wm_state, Xatom_net_wm_state_fullscreen,
    Xatom_net_wm_state_maximized_horz, Xatom_net_wm_state_maximized_vert,
    Xatom_net_wm_state_sticky, Xatom_net_wm_state_above, Xatom_net_wm_state_below,
    Xatom_net_wm_state_hidden, Xatom_net_wm_state_skip_taskbar,
    Xatom_net_frame_extents, Xatom_net_current_desktop, Xatom_net_workarea;

  /* XSettings atoms and windows.  */
  Atom Xatom_xsettings_sel, Xatom_xsettings_prop, Xatom_xsettings_mgr;
  Window xsettings_window;

  /* Frame name and icon name */
  Atom Xatom_net_wm_name, Xatom_net_wm_icon_name;
  /* Frame opacity */
  Atom Xatom_net_wm_window_opacity;

  /* SM */
  Atom Xatom_SM_CLIENT_ID;

#ifdef HAVE_XRANDR
  int xrandr_major_version;
  int xrandr_minor_version;
#endif

#ifdef USE_CAIRO
  XExtCodes *ext_codes;
#endif

#ifdef USE_XCB
  xcb_connection_t *xcb_connection;
#endif

#ifdef HAVE_XDBE
  bool supports_xdbe;
#endif
};

#ifdef HAVE_X_I18N
/* Whether or not to use XIM if we have it.  */
extern bool use_xim;
#endif

/* This is a chain of structures for all the X displays currently in use.  */
extern struct x_display_info *x_display_list;

extern struct x_display_info *x_display_info_for_display (Display *);
extern struct frame *x_top_window_to_frame (struct x_display_info *, int);
extern struct x_display_info *x_term_init (Lisp_Object, char *, char *);
extern bool x_display_ok (const char *);

extern void select_visual (struct x_display_info *);

extern Window tip_window;

/* Each X frame object points to its own struct x_output object
   in the output_data.x field.  The x_output structure contains
   the information that is specific to X windows.  */

struct x_output
{
#if defined (USE_X_TOOLKIT) || defined (USE_GTK)
  /* Height of menu bar widget, in pixels.  This value
     is not meaningful if the menubar is turned off.  */
  int menubar_height;
#endif

  /* Height of tool bar widget, in pixels.  top_height is used if tool bar
     at top, bottom_height if tool bar is at the bottom.
     Zero if not using an external tool bar or if tool bar is vertical.  */
  int toolbar_top_height, toolbar_bottom_height;

  /* Width of tool bar widget, in pixels.  left_width is used if tool bar
     at left, right_width if tool bar is at the right.
     Zero if not using an external tool bar or if tool bar is horizontal.  */
  int toolbar_left_width, toolbar_right_width;

  /* The tiled border used when the mouse is out of the frame.  */
  Pixmap border_tile;

  /* Here are the Graphics Contexts for the default font.  */
  GC normal_gc;				/* Normal video */
  GC reverse_gc;			/* Reverse video */
  GC cursor_gc;				/* cursor drawing */

  /* The X window used for this frame.
     May be zero while the frame object is being created
     and the X window has not yet been created.  */
  Window window_desc;

  /* The drawable to which we're rendering.  In the single-buffered
     base, the window itself.  In the double-buffered case, the
     window's back buffer.  */
  Drawable draw_desc;

  /* Flag that indicates whether we've modified the back buffer and
     need to publish our modifications to the front buffer at a
     convenient time.  */
  bool need_buffer_flip;

  /* The X window used for the bitmap icon;
     or 0 if we don't have a bitmap icon.  */
  Window icon_desc;

  /* The X window that is the parent of this X window.
     Usually this is a window that was made by the window manager,
     but it can be the root window, and it can be explicitly specified
     (see the explicit_parent field, below).  */
  Window parent_desc;

#ifdef USE_X_TOOLKIT
  /* The widget of this screen.  This is the window of a "shell" widget.  */
  Widget widget;
  /* The XmPanedWindows...  */
  Widget column_widget;
  /* The widget of the edit portion of this screen; the window in
     "window_desc" is inside of this.  */
  Widget edit_widget;

  Widget menubar_widget;
#endif

#ifdef USE_GTK
  /* The widget of this screen.  This is the window of a top widget.  */
  GtkWidget *widget;
  /* The widget of the edit portion of this screen; the window in
     "window_desc" is inside of this.  */
  GtkWidget *edit_widget;
  /* The widget used for laying out widgets vertically.  */
  GtkWidget *vbox_widget;
  /* The widget used for laying out widgets horizontally.  */
  GtkWidget *hbox_widget;
  /* The menubar in this frame.  */
  GtkWidget *menubar_widget;
  /* The tool bar in this frame  */
  GtkWidget *toolbar_widget;
  /* True if tool bar is packed into the hbox widget (i.e. vertical).  */
  bool_bf toolbar_in_hbox : 1;
  bool_bf toolbar_is_packed : 1;

  /* The last size hints set.  */
  GdkGeometry size_hints;
  long hint_flags;

  GtkTooltip *ttip_widget;
  GtkWidget *ttip_lbl;
  GtkWindow *ttip_window;
#endif /* USE_GTK */

  /* If >=0, a bitmap index.  The indicated bitmap is used for the
     icon. */
  ptrdiff_t icon_bitmap;

  /* Default ASCII font of this frame.  */
  struct font *font;

  /* The baseline offset of the default ASCII font.  */
  int baseline_offset;

  /* If a fontset is specified for this frame instead of font, this
     value contains an ID of the fontset, else -1.  */
  int fontset;

  unsigned long cursor_pixel;
  unsigned long border_pixel;
  unsigned long mouse_pixel;
  unsigned long cursor_foreground_pixel;

  /* Foreground color for scroll bars.  A value of -1 means use the
     default (black for non-toolkit scroll bars).  */
  unsigned long scroll_bar_foreground_pixel;

  /* Background color for scroll bars.  A value of -1 means use the
     default (background color of the frame for non-toolkit scroll
     bars).  */
  unsigned long scroll_bar_background_pixel;

#if defined (USE_LUCID) && defined (USE_TOOLKIT_SCROLL_BARS)
  /* Top and bottom shadow colors for 3D Lucid scrollbars.
     -1 means let the scroll compute them itself.  */
  unsigned long scroll_bar_top_shadow_pixel;
  unsigned long scroll_bar_bottom_shadow_pixel;
#endif

  /* Descriptor for the cursor in use for this window.  */
  Cursor text_cursor;
  Cursor nontext_cursor;
  Cursor modeline_cursor;
  Cursor hand_cursor;
  Cursor hourglass_cursor;
  Cursor horizontal_drag_cursor;
  Cursor vertical_drag_cursor;
  Cursor current_cursor;
  Cursor left_edge_cursor;
  Cursor top_left_corner_cursor;
  Cursor top_edge_cursor;
  Cursor top_right_corner_cursor;
  Cursor right_edge_cursor;
  Cursor bottom_right_corner_cursor;
  Cursor bottom_edge_cursor;
  Cursor bottom_left_corner_cursor;

  /* Window whose cursor is hourglass_cursor.  This window is temporarily
     mapped to display an hourglass cursor.  */
  Window hourglass_window;

  /* These are the current window manager hints.  It seems that
     XSetWMHints, when presented with an unset bit in the `flags'
     member of the hints structure, does not leave the corresponding
     attribute unchanged; rather, it resets that attribute to its
     default value.  For example, unless you set the `icon_pixmap'
     field and the `IconPixmapHint' bit, XSetWMHints will forget what
     your icon pixmap was.  This is rather troublesome, since some of
     the members (for example, `input' and `icon_pixmap') want to stay
     the same throughout the execution of Emacs.  So, we keep this
     structure around, just leaving values in it and adding new bits
     to the mask as we go.  */
  XWMHints wm_hints;

  /* This is the Emacs structure for the X display this frame is on.  */
  struct x_display_info *display_info;

  /* This is a button event that wants to activate the menubar.
     We save it here until the command loop gets to think about it.  */
  XEvent *saved_menu_event;

  /* This is the widget id used for this frame's menubar in lwlib.  */
#ifdef USE_X_TOOLKIT
  int id;
#endif

  /* True means hourglass cursor is currently displayed.  */
  bool_bf hourglass_p : 1;

  /* True means our parent is another application's window
     and was explicitly specified.  */
  bool_bf explicit_parent : 1;

  /* True means tried already to make this frame visible.  */
  bool_bf asked_for_visible : 1;

  /* True if this frame was ever previously visible.  */
  bool_bf has_been_visible : 1;

  /* Xt waits for a ConfigureNotify event from the window manager in
     EmacsFrameSetCharSize when the shell widget is resized.  For some
     window managers like fvwm2 2.2.5 and KDE 2.1 this event doesn't
     arrive for an unknown reason and Emacs hangs in Xt.  If this is
     false, tell Xt not to wait.  */
  bool_bf wait_for_wm : 1;

#ifdef HAVE_X_I18N
  /* Input context (currently, this means Compose key handler setup).  */
  XIC xic;
  XIMStyle xic_style;
  XFontSet xic_xfs;
#endif

  /* Relief GCs, colors etc.  */
  struct relief
  {
    GC gc;
    unsigned long pixel;
  }
  black_relief, white_relief;

  /* The background for which the above relief GCs were set up.
     They are changed only when a different background is involved.  */
  unsigned long relief_background;

  /* Keep track of focus.  May be EXPLICIT if we received a FocusIn for this
     frame, or IMPLICIT if we received an EnterNotify.
     FocusOut and LeaveNotify clears EXPLICIT/IMPLICIT. */
  int focus_state;

  /* The offset we need to add to compensate for type A WMs.  */
  int move_offset_top;
  int move_offset_left;

/* Extreme 'short' and 'long' values suitable for libX11.  */
#define X_SHRT_MAX 0x7fff
#define X_SHRT_MIN (-1 - X_SHRT_MAX)
#define X_LONG_MAX 0x7fffffff
#define X_LONG_MIN (-1 - X_LONG_MAX)
#define X_ULONG_MAX 0xffffffffUL

#ifdef USE_CAIRO
  /* Cairo drawing context.  */
  cairo_t *cr_context;
  /* Width and height reported by the last ConfigureNotify event.
     They are used when creating the cairo surface next time.  */
  int cr_surface_desired_width, cr_surface_desired_height;
#endif
};

enum
{
  /* Values for focus_state, used as bit mask.
     EXPLICIT means we received a FocusIn for the frame and know it has
     the focus.  IMPLICIT means we received an EnterNotify and the frame
     may have the focus if no window manager is running.
     FocusOut and LeaveNotify clears EXPLICIT/IMPLICIT. */
  FOCUS_NONE     = 0,
  FOCUS_IMPLICIT = 1,
  FOCUS_EXPLICIT = 2
};


/* Return the X output data for frame F.  */
#define FRAME_X_OUTPUT(f) ((f)->output_data.x)
#define FRAME_OUTPUT_DATA(f) FRAME_X_OUTPUT (f)

/* Return the X window used for displaying data in frame F.  */
#define FRAME_X_WINDOW(f) ((f)->output_data.x->window_desc)
#define FRAME_NATIVE_WINDOW(f) FRAME_X_WINDOW (f)

/* Return the drawable used for rendering to frame F.  */
#define FRAME_X_RAW_DRAWABLE(f) ((f)->output_data.x->draw_desc)

extern void x_mark_frame_dirty (struct frame *f);

/* Return the drawable used for rendering to frame F and mark the
   frame as needing a buffer flip later.  There's no easy way to run
   code after any drawing command, but we can run code whenever
   someone asks for the handle necessary to draw.  */
#define FRAME_X_DRAWABLE(f)                             \
  (x_mark_frame_dirty((f)), FRAME_X_RAW_DRAWABLE ((f)))

#define FRAME_X_DOUBLE_BUFFERED_P(f)            \
  (FRAME_X_WINDOW (f) != FRAME_X_RAW_DRAWABLE (f))

/* Return the need-buffer-flip flag for frame F.  */
#define FRAME_X_NEED_BUFFER_FLIP(f) ((f)->output_data.x->need_buffer_flip)

/* Return the outermost X window associated with the frame F.  */
#ifdef USE_X_TOOLKIT
#define FRAME_OUTER_WINDOW(f) ((f)->output_data.x->widget ?             \
                               XtWindow ((f)->output_data.x->widget) :  \
                               FRAME_X_WINDOW (f))
#else
#ifdef USE_GTK

#ifdef HAVE_GTK3
#define DEFAULT_GDK_DISPLAY() \
  gdk_x11_display_get_xdisplay (gdk_display_get_default ())
#else
#undef GDK_WINDOW_XID
#define GDK_WINDOW_XID(w) GDK_WINDOW_XWINDOW (w)
#define DEFAULT_GDK_DISPLAY() GDK_DISPLAY ()
#define gtk_widget_get_preferred_size(a, ign, b) \
  gtk_widget_size_request (a, b)
#endif

#define GTK_WIDGET_TO_X_WIN(w) \
  ((w) && gtk_widget_get_window (w) \
   ? GDK_WINDOW_XID (gtk_widget_get_window (w)) : 0)

#define FRAME_GTK_OUTER_WIDGET(f) ((f)->output_data.x->widget)
#define FRAME_GTK_WIDGET(f) ((f)->output_data.x->edit_widget)
#define FRAME_OUTER_WINDOW(f)                                   \
       (FRAME_GTK_OUTER_WIDGET (f) ?                            \
        GTK_WIDGET_TO_X_WIN (FRAME_GTK_OUTER_WIDGET (f)) :      \
         FRAME_X_WINDOW (f))

#else /* !USE_GTK */
#define FRAME_OUTER_WINDOW(f) (FRAME_X_WINDOW (f))
#endif /* !USE_GTK */
#endif

#if defined (USE_X_TOOLKIT) || defined (USE_GTK)
#define FRAME_MENUBAR_HEIGHT(f) ((f)->output_data.x->menubar_height)
#else
#define FRAME_MENUBAR_HEIGHT(f) ((void) f, 0)
#endif /* USE_X_TOOLKIT || USE_GTK */

#define FRAME_FONT(f) ((f)->output_data.x->font)
#define FRAME_FONTSET(f) ((f)->output_data.x->fontset)
#define FRAME_TOOLBAR_TOP_HEIGHT(f) ((f)->output_data.x->toolbar_top_height)
#define FRAME_TOOLBAR_BOTTOM_HEIGHT(f) \
  ((f)->output_data.x->toolbar_bottom_height)
#define FRAME_TOOLBAR_HEIGHT(f) \
  (FRAME_TOOLBAR_TOP_HEIGHT (f) + FRAME_TOOLBAR_BOTTOM_HEIGHT (f))
#define FRAME_TOOLBAR_LEFT_WIDTH(f) ((f)->output_data.x->toolbar_left_width)
#define FRAME_TOOLBAR_RIGHT_WIDTH(f) ((f)->output_data.x->toolbar_right_width)
#define FRAME_TOOLBAR_WIDTH(f) \
  (FRAME_TOOLBAR_LEFT_WIDTH (f) + FRAME_TOOLBAR_RIGHT_WIDTH (f))
#define FRAME_BASELINE_OFFSET(f) ((f)->output_data.x->baseline_offset)

/* This gives the x_display_info structure for the display F is on.  */
#define FRAME_DISPLAY_INFO(f) ((f)->output_data.x->display_info)

/* This is the `Display *' which frame F is on.  */
#define FRAME_X_DISPLAY(f) (FRAME_DISPLAY_INFO (f)->display)

/* This is the `Screen *' which frame F is on.  */
#define FRAME_X_SCREEN(f) (FRAME_DISPLAY_INFO (f)->screen)

/* This is the screen index number of screen which frame F is on.  */
#define FRAME_X_SCREEN_NUMBER(f) XScreenNumberOfScreen (FRAME_X_SCREEN (f))

/* This is the Visual which frame F is on.  */
#define FRAME_X_VISUAL(f) FRAME_DISPLAY_INFO (f)->visual

/* This is the Colormap which frame F uses.  */
#define FRAME_X_COLORMAP(f) FRAME_DISPLAY_INFO (f)->cmap

#define FRAME_XIC(f) ((f)->output_data.x->xic)
#define FRAME_X_XIM(f) (FRAME_DISPLAY_INFO (f)->xim)
#define FRAME_X_XIM_STYLES(f) (FRAME_DISPLAY_INFO (f)->xim_styles)
#define FRAME_XIC_STYLE(f) ((f)->output_data.x->xic_style)
#define FRAME_XIC_FONTSET(f) ((f)->output_data.x->xic_xfs)

/* X-specific scroll bar stuff.  */

/* We represent scroll bars as lisp vectors.  This allows us to place
   references to them in windows without worrying about whether we'll
   end up with windows referring to dead scroll bars; the garbage
   collector will free it when its time comes.

   We use struct scroll_bar as a template for accessing fields of the
   vector.  */

struct scroll_bar
{
  /* These fields are shared by all vectors.  */
  union vectorlike_header header;

  /* The window we're a scroll bar for.  */
  Lisp_Object window;

  /* The next and previous in the chain of scroll bars in this frame.  */
  Lisp_Object next, prev;

  /* Fields after 'prev' are not traced by the GC.  */

  /* The X window representing this scroll bar.  */
  Window x_window;

  /* The position and size of the scroll bar in pixels, relative to the
     frame.  */
  int top, left, width, height;

  /* The starting and ending positions of the handle, relative to the
     handle area (i.e. zero is the top position, not
     SCROLL_BAR_TOP_BORDER).  If they're equal, that means the handle
     hasn't been drawn yet.

     These are not actually the locations where the beginning and end
     are drawn; in order to keep handles from becoming invisible when
     editing large files, we establish a minimum height by always
     drawing handle bottoms VERTICAL_SCROLL_BAR_MIN_HANDLE pixels below
     where they would be normally; the bottom and top are in a
     different coordinate system.  */
  int start, end;

  /* If the scroll bar handle is currently being dragged by the user,
     this is the number of pixels from the top of the handle to the
     place where the user grabbed it.  If the handle isn't currently
     being dragged, this is -1.  */
  int dragging;

#if defined (USE_TOOLKIT_SCROLL_BARS) && defined (USE_LUCID)
  /* Last scroll bar part seen in xaw_jump_callback and xaw_scroll_callback.  */
  enum scroll_bar_part last_seen_part;
#endif

#if defined (USE_TOOLKIT_SCROLL_BARS) && !defined (USE_GTK)
  /* Last value of whole for horizontal scrollbars.  */
  int whole;
#endif

  /* True if the scroll bar is horizontal.  */
  bool horizontal;
} GCALIGNED_STRUCT;

/* Turning a lisp vector value into a pointer to a struct scroll_bar.  */
#define XSCROLL_BAR(vec) ((struct scroll_bar *) XVECTOR (vec))

#ifdef USE_X_TOOLKIT

/* Extract the X widget of the scroll bar from a struct scroll_bar.
   XtWindowToWidget should be fast enough since Xt uses a hash table
   to map windows to widgets.  */

#define SCROLL_BAR_X_WIDGET(dpy, ptr) \
  XtWindowToWidget (dpy, ptr->x_window)

/* Store a widget id in a struct scroll_bar.  */

#define SET_SCROLL_BAR_X_WIDGET(ptr, w)		\
  do {						\
    Window window = XtWindow (w);		\
    ptr->x_window = window;			\
  } while (false)

#endif /* USE_X_TOOLKIT */

/* Return the inside width of a vertical scroll bar, given the outside
   width.  */
#define VERTICAL_SCROLL_BAR_INSIDE_WIDTH(f, width) \
  ((width) \
   - VERTICAL_SCROLL_BAR_LEFT_BORDER \
   - VERTICAL_SCROLL_BAR_RIGHT_BORDER)

/* Return the length of the rectangle within which the top of the
   handle must stay.  This isn't equivalent to the inside height,
   because the scroll bar handle has a minimum height.

   This is the real range of motion for the scroll bar, so when we're
   scaling buffer positions to scroll bar positions, we use this, not
   VERTICAL_SCROLL_BAR_INSIDE_HEIGHT.  */
#define VERTICAL_SCROLL_BAR_TOP_RANGE(f, height) \
  (VERTICAL_SCROLL_BAR_INSIDE_HEIGHT (f, height) - VERTICAL_SCROLL_BAR_MIN_HANDLE)

/* Return the inside height of vertical scroll bar, given the outside
   height.  See VERTICAL_SCROLL_BAR_TOP_RANGE too.  */
#define VERTICAL_SCROLL_BAR_INSIDE_HEIGHT(f, height) \
  ((height) - VERTICAL_SCROLL_BAR_TOP_BORDER - VERTICAL_SCROLL_BAR_BOTTOM_BORDER)

/* Return the inside height of a horizontal scroll bar, given the outside
   height.  */
#define HORIZONTAL_SCROLL_BAR_INSIDE_HEIGHT(f, height)	\
  ((height) \
   - HORIZONTAL_SCROLL_BAR_TOP_BORDER \
   - HORIZONTAL_SCROLL_BAR_BOTTOM_BORDER)

/* Return the length of the rectangle within which the left part of the
   handle must stay.  This isn't equivalent to the inside width, because
   the scroll bar handle has a minimum width.

   This is the real range of motion for the scroll bar, so when we're
   scaling buffer positions to scroll bar positions, we use this, not
   HORIZONTAL_SCROLL_BAR_INSIDE_WIDTH.  */
#define HORIZONTAL_SCROLL_BAR_LEFT_RANGE(f, width) \
  (HORIZONTAL_SCROLL_BAR_INSIDE_WIDTH (f, width) - HORIZONTAL_SCROLL_BAR_MIN_HANDLE)

/* Return the inside width of horizontal scroll bar, given the outside
   width.  See HORIZONTAL_SCROLL_BAR_LEFT_RANGE too.  */
#define HORIZONTAL_SCROLL_BAR_INSIDE_WIDTH(f, width) \
  ((width) - HORIZONTAL_SCROLL_BAR_LEFT_BORDER - HORIZONTAL_SCROLL_BAR_LEFT_BORDER)


/* Border widths for scroll bars.

   Scroll bar windows don't have any X borders; their border width is
   set to zero, and we redraw borders ourselves.  This makes the code
   a bit cleaner, since we don't have to convert between outside width
   (used when relating to the rest of the screen) and inside width
   (used when sizing and drawing the scroll bar window itself).

   The handle moves up and down/back and forth in a rectangle inset
   from the edges of the scroll bar.  These are widths by which we
   inset the handle boundaries from the scroll bar edges.  */
#define VERTICAL_SCROLL_BAR_LEFT_BORDER (2)
#define VERTICAL_SCROLL_BAR_RIGHT_BORDER (2)
#define VERTICAL_SCROLL_BAR_TOP_BORDER (2)
#define VERTICAL_SCROLL_BAR_BOTTOM_BORDER (2)

#define HORIZONTAL_SCROLL_BAR_LEFT_BORDER (2)
#define HORIZONTAL_SCROLL_BAR_RIGHT_BORDER (2)
#define HORIZONTAL_SCROLL_BAR_TOP_BORDER (2)
#define HORIZONTAL_SCROLL_BAR_BOTTOM_BORDER (2)

/* Minimum lengths for scroll bar handles, in pixels.  */
#define VERTICAL_SCROLL_BAR_MIN_HANDLE (5)
#define HORIZONTAL_SCROLL_BAR_MIN_HANDLE (5)

/* If a struct input_event has a kind which is SELECTION_REQUEST_EVENT
   or SELECTION_CLEAR_EVENT, then its contents are really described
   by this structure.  */

/* For an event of kind SELECTION_REQUEST_EVENT,
   this structure really describes the contents.  */

struct selection_input_event
{
  ENUM_BF (event_kind) kind : EVENT_KIND_WIDTH;
  struct x_display_info *dpyinfo;
  /* We spell it with an "o" here because X does.  */
  Window requestor;
  Atom selection, target, property;
  Time time;
};

/* Unlike macros below, this can't be used as an lvalue.  */
INLINE Display *
SELECTION_EVENT_DISPLAY (struct selection_input_event *ev)
{
  return ev->dpyinfo->display;
}
#define SELECTION_EVENT_DPYINFO(eventp) \
  ((eventp)->dpyinfo)
/* We spell it with an "o" here because X does.  */
#define SELECTION_EVENT_REQUESTOR(eventp)	\
  ((eventp)->requestor)
#define SELECTION_EVENT_SELECTION(eventp)	\
  ((eventp)->selection)
#define SELECTION_EVENT_TARGET(eventp)	\
  ((eventp)->target)
#define SELECTION_EVENT_PROPERTY(eventp)	\
  ((eventp)->property)
#define SELECTION_EVENT_TIME(eventp)	\
  ((eventp)->time)

/* From xfns.c.  */

extern void x_free_gcs (struct frame *);
extern void x_relative_mouse_position (struct frame *, int *, int *);
extern void x_real_pos_and_offsets (struct frame *f,
                                    int *left_offset_x,
                                    int *right_offset_x,
                                    int *top_offset_y,
                                    int *bottom_offset_y,
                                    int *x_pixels_diff,
                                    int *y_pixels_diff,
                                    int *xptr,
                                    int *yptr,
                                    int *outer_border);
extern void x_default_font_parameter (struct frame* f, Lisp_Object parms);

/* From xrdb.c.  */

XrmDatabase x_load_resources (Display *, const char *, const char *,
			      const char *);
extern const char *x_get_string_resource (void *, const char *, const char *);

/* Defined in xterm.c */

typedef void (*x_special_error_handler)(Display *, XErrorEvent *, char *,
					void *);

extern bool x_text_icon (struct frame *, const char *);
extern void x_catch_errors (Display *);
extern void x_catch_errors_with_handler (Display *, x_special_error_handler,
					 void *);
extern void x_check_errors (Display *, const char *)
  ATTRIBUTE_FORMAT_PRINTF (2, 0);
extern bool x_had_errors_p (Display *);
extern void x_uncatch_errors (void);
extern void x_uncatch_errors_after_check (void);
extern void x_clear_errors (Display *);
extern void x_set_window_size (struct frame *f, bool, int, int);
extern void x_make_frame_visible (struct frame *f);
extern void x_make_frame_invisible (struct frame *f);
extern void x_iconify_frame (struct frame *f);
extern void x_free_frame_resources (struct frame *);
extern void x_wm_set_size_hint (struct frame *, long, bool);

extern void x_delete_terminal (struct terminal *terminal);
extern unsigned long x_copy_color (struct frame *, unsigned long);
#ifdef USE_X_TOOLKIT
extern XtAppContext Xt_app_con;
extern void x_activate_timeout_atimer (void);
#endif
#ifdef USE_LUCID
extern bool x_alloc_lighter_color_for_widget (Widget, Display *, Colormap,
					      unsigned long *,
					      double, int);
#endif
extern bool x_alloc_nearest_color (struct frame *, Colormap, XColor *);
extern void x_query_colors (struct frame *f, XColor *, int);
extern void x_clear_area (struct frame *f, int, int, int, int);
#if !defined USE_X_TOOLKIT && !defined USE_GTK
extern void x_mouse_leave (struct x_display_info *);
#endif

#if defined USE_X_TOOLKIT || defined USE_MOTIF
extern int x_dispatch_event (XEvent *, Display *);
#endif
extern int x_x_to_emacs_modifiers (struct x_display_info *, int);
extern int x_emacs_to_x_modifiers (struct x_display_info *, intmax_t);
#ifdef USE_CAIRO
extern void x_cr_destroy_frame_context (struct frame *);
extern void x_cr_update_surface_desired_size (struct frame *, int, int);
extern cairo_t *x_begin_cr_clip (struct frame *, GC);
extern void x_end_cr_clip (struct frame *);
extern void x_set_cr_source_with_gc_foreground (struct frame *, GC);
extern void x_set_cr_source_with_gc_background (struct frame *, GC);
extern void x_cr_draw_frame (cairo_t *, struct frame *);
extern Lisp_Object x_cr_export_frames (Lisp_Object, cairo_surface_type_t);
#endif

INLINE int
x_display_pixel_height (struct x_display_info *dpyinfo)
{
  return HeightOfScreen (dpyinfo->screen);
}

INLINE int
x_display_pixel_width (struct x_display_info *dpyinfo)
{
  return WidthOfScreen (dpyinfo->screen);
}

INLINE void
x_display_set_last_user_time (struct x_display_info *dpyinfo, Time t)
{
#ifdef ENABLE_CHECKING
  eassert (t <= X_ULONG_MAX);
#endif
  dpyinfo->last_user_time = t;
}

INLINE unsigned long
x_make_truecolor_pixel (struct x_display_info *dpyinfo, int r, int g, int b)
{
  unsigned long pr, pg, pb;

  /* Scale down RGB values to the visual's bits per RGB, and shift
     them to the right position in the pixel color.  Note that the
     original RGB values are 16-bit values, as usual in X.  */
  pr = (r >> (16 - dpyinfo->red_bits))   << dpyinfo->red_offset;
  pg = (g >> (16 - dpyinfo->green_bits)) << dpyinfo->green_offset;
  pb = (b >> (16 - dpyinfo->blue_bits))  << dpyinfo->blue_offset;

  /* Assemble the pixel color.  */
  return pr | pg | pb;
}

/* If display has an immutable color map, freeing colors is not
   necessary and some servers don't allow it, so we won't do it.  That
   also allows us to make other optimizations relating to server-side
   reference counts.  */
INLINE bool
x_mutable_colormap (Visual *visual)
{
  int class = visual->class;
  return (class != StaticColor && class != StaticGray && class != TrueColor);
}

extern void x_set_sticky (struct frame *, Lisp_Object, Lisp_Object);
extern void x_set_skip_taskbar (struct frame *, Lisp_Object, Lisp_Object);
extern void x_set_z_group (struct frame *, Lisp_Object, Lisp_Object);
extern bool x_wm_supports (struct frame *, Atom);
extern void x_wait_for_event (struct frame *, int);
extern void x_clear_under_internal_border (struct frame *f);

extern void tear_down_x_back_buffer (struct frame *f);
extern void initial_set_up_x_back_buffer (struct frame *f);

/* Defined in xfns.c.  */
extern void x_real_positions (struct frame *, int *, int *);
extern void x_change_tab_bar_height (struct frame *, int);
extern void x_change_tool_bar_height (struct frame *, int);
extern void x_implicitly_set_name (struct frame *, Lisp_Object, Lisp_Object);
extern void x_set_scroll_bar_default_width (struct frame *);
extern void x_set_scroll_bar_default_height (struct frame *);

/* Defined in xselect.c.  */

extern void x_handle_property_notify (const XPropertyEvent *);
extern void x_handle_selection_notify (const XSelectionEvent *);
extern void x_handle_selection_event (struct selection_input_event *);
extern void x_clear_frame_selections (struct frame *);

extern void x_send_client_event (Lisp_Object display,
                                 Lisp_Object dest,
                                 Lisp_Object from,
                                 Atom message_type,
                                 Lisp_Object format,
                                 Lisp_Object values);

extern bool x_handle_dnd_message (struct frame *,
				  const XClientMessageEvent *,
				  struct x_display_info *,
				  struct input_event *);
extern int x_check_property_data (Lisp_Object);
extern void x_fill_property_data (Display *,
                                  Lisp_Object,
                                  void *,
				  int,
                                  int);
extern Lisp_Object x_property_data_to_lisp (struct frame *,
                                            const unsigned char *,
                                            Atom,
                                            int,
                                            unsigned long);
extern void x_clipboard_manager_save_frame (Lisp_Object);
extern void x_clipboard_manager_save_all (void);

#ifdef USE_GTK
extern bool xg_set_icon (struct frame *, Lisp_Object);
extern bool xg_set_icon_from_xpm_data (struct frame *, const char **);
#endif /* USE_GTK */

extern void xic_free_xfontset (struct frame *);
extern void create_frame_xic (struct frame *);
extern void destroy_frame_xic (struct frame *);
extern void xic_set_preeditarea (struct window *, int, int);
extern void xic_set_statusarea (struct frame *);
extern void xic_set_xfontset (struct frame *, const char *);
extern bool x_defined_color (struct frame *, const char *, Emacs_Color *,
                             bool, bool);
#ifdef HAVE_X_I18N
extern void free_frame_xic (struct frame *);
# if defined HAVE_X_WINDOWS && defined USE_X_TOOLKIT
extern char *xic_create_fontsetname (const char *, bool);
# endif
#endif

/* Defined in xfaces.c */

#ifdef USE_X_TOOLKIT
extern void x_free_dpy_colors (Display *, Screen *, Colormap,
                               unsigned long *, int);
#endif /* USE_X_TOOLKIT */

/* Defined in xmenu.c */

#if defined USE_X_TOOLKIT || defined USE_GTK
extern Lisp_Object xw_popup_dialog (struct frame *, Lisp_Object, Lisp_Object);
#endif

#if defined USE_GTK || defined USE_MOTIF
extern void x_menu_set_in_use (bool);
#endif
extern void x_menu_wait_for_event (void *data);
extern void initialize_frame_menubar (struct frame *);

/* Defined in xsmfns.c */
#ifdef HAVE_X_SM
extern void x_session_initialize (struct x_display_info *dpyinfo);
extern bool x_session_have_connection (void);
extern void x_session_close (void);
#endif


/* Is the frame embedded into another application? */

#define FRAME_X_EMBEDDED_P(f) (FRAME_X_OUTPUT(f)->explicit_parent != 0)

#define STORE_NATIVE_RECT(nr,rx,ry,rwidth,rheight)	\
  ((nr).x = (rx),					\
   (nr).y = (ry),					\
   (nr).width = (rwidth),				\
   (nr).height = (rheight))

INLINE_HEADER_END

#endif /* XTERM_H */
