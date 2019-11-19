/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "gui/widgets/main_window.h"
#include "audio/position.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define ARRANGER_WIDGET_TYPE ( \
  arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerWidget,
  arranger_widget,
  Z, ARRANGER_WIDGET,
  GtkDrawingArea)

#define ARRANGER_WIDGET_GET_ACTION(arr,actn) \
  (arr->action == UI_OVERLAY_ACTION_##actn)

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct _ArrangerPlayheadWidget
  ArrangerPlayheadWidget;

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct _GtkEventControllerMotion
  GtkEventControllerMotion;

typedef enum ArrangerCursor
{
  /** Invalid cursor. */
  ARRANGER_CURSOR_NONE,
  ARRANGER_CURSOR_SELECT,
  ARRANGER_CURSOR_EDIT,
  ARRANGER_CURSOR_CUT,
  ARRANGER_CURSOR_ERASER,
  ARRANGER_CURSOR_AUDITION,
  ARRANGER_CURSOR_RAMP,
  ARRANGER_CURSOR_GRAB,
  ARRANGER_CURSOR_GRABBING,
  ARRANGER_CURSOR_RESIZING_L,
  ARRANGER_CURSOR_RESIZING_L_LOOP,
  ARRANGER_CURSOR_RESIZING_R,
  ARRANGER_CURSOR_RESIZING_R_LOOP,
  ARRANGER_CURSOR_RESIZING_UP,
  ARRANGER_CURSOR_GRABBING_COPY,
  ARRANGER_CURSOR_GRABBING_LINK,
  ARRANGER_CURSOR_RANGE,
} ArrangerCursor;

/**
 * Type of arranger.
 */
typedef enum ArrangerWidgetType
{
  ARRANGER_WIDGET_TYPE_TIMELINE,
  ARRANGER_WIDGET_TYPE_MIDI,
  ARRANGER_WIDGET_TYPE_MIDI_MODIFIER,
  ARRANGER_WIDGET_TYPE_AUDIO,
  ARRANGER_WIDGET_TYPE_CHORD,
  ARRANGER_WIDGET_TYPE_AUTOMATION,
} ArrangerWidgetType;

/**
 * The arranger widget is a canvas that draws all
 * the arranger objects it contains.
 */
typedef struct _ArrangerWidget
{
  GtkDrawingArea       parent_instance;

  /** Type of arranger this is. */
  ArrangerWidgetType   type;

  GtkGestureDrag *     drag;
  GtkGestureMultiPress * multipress;
  GtkGestureMultiPress * right_mouse_mp;
  GtkEventControllerMotion * motion_controller;

  /** Used when dragging. */
  double               last_offset_x;
  double               last_offset_y;

  UiOverlayAction      action;

  /** X-axis coordinate at start of drag. */
  double               start_x;

  /** Y-axis coordinate at start of drag. */
  double               start_y;

  /** X-axis coordinate at the start of the drag,
   * in pixels. */
  double               start_pos_px;

  /** Whether an object exists, so we can use the
   * earliest_obj_start_pos. */
  int                  earliest_obj_exists;

  /** Start Position of the earliest object
   * at the start of the drag. */
  Position             earliest_obj_start_pos;

  /**
   * The object that was clicked in this drag
   * cycle, if any.
   *
   * This is the ArrangerObject that was clicked,
   * even though there could be more selected.
   */
  ArrangerObject *       start_object;

  /** Whether the start object was selected before
   * drag_begin. */
  int                    start_object_was_selected;

  /** A clone of the ArrangerSelections on drag
   * begin. */
  ArrangerSelections *  sel_at_start;

  /** Start Position of the earliest object
   * currently. */
  //Position             earliest_obj_pos;

  /** The absolute (not snapped) Position at the
   * start of a drag, translated from start_x. */
  Position             start_pos;

  /** The absolute (not snapped) current diff in
   * ticks from the curr_pos to the start_pos. */
  long                 curr_ticks_diff_from_start;

  /** The adjusted diff in ticks to use for moving
   * objects starting from their cached start
   * positions. */
  long                 adj_ticks_diff;

  /** The absolute (not snapped) Position as of the
   * current action. */
  Position             curr_pos;

  Position             end_pos; ///< for moving regions
  gboolean             key_is_pressed;

  /** Current hovering positions. */
  double               hover_x;
  double               hover_y;

  /** Number of clicks in current action. */
  int                  n_press;

  /** Associated SnapGrid. */
  SnapGrid *           snap_grid;

  /** Whether shift button is held down. */
  int                  shift_held;

  /** Whether Ctrl button is held down. */
  int                  ctrl_held;

  /** Whether Alt is currently held down. */
  int                  alt_held;

  gint64               last_frame_time;

  /* ----- TIMELINE ------ */

  /** The number of visible tracks moved during a
   * moving operation between tracks up to the last
   * cycle. */
  int                  visible_track_diff;

  /** The number of lanes moved during a
   * moving operation between lanes, up to the last
   * cycle. */
  int                  lane_diff;

  int                  last_timeline_obj_bars;

  /** Whether this TimelineArrangerWidget is for
   * the PinnedTracklist or not. */
  int                  is_pinned;

  /**
   * 1 if resizing range.
   */
  int                  resizing_range;

  /**
   * 1 if this is the first call to resize the range,
   * so range1 can be set.
   */
  int                  resizing_range_start;

  /** Cache for chord object height, used during
   * child size allocation. */
  int                  chord_obj_height;

  /* ----- END TIMELINE ----- */

  /* ------ MIDI (PIANO ROLL) ---- */

  /** The note currently hovering over */
  int                      hovered_note;

  /* ------ END MIDI (PIANO ROLL) ---- */

  /* ------ MIDI MODIFIER ---- */

  /** 1-127. */
  int            start_vel_val;

  /**
   * Maximum Velocity diff applied in this action.
   *
   * Used in drag_end to create an UndableAction.
   * This can have any value, even greater than 127
   * and it will be clamped when applying it to
   * a Velocity.
   */
  int            vel_diff;

  /* ------ END MIDI MODIFIER ---- */

  /* ------- CHORD ------- */

  /** Index of the chord being hovered on. */
  int             hovered_chord_index;

  /* ------- END CHORD ------- */

  /** Set to 1 to redraw. */
  int               redraw;

  cairo_t *         cached_cr;

  cairo_surface_t * cached_surface;

  /** Rectangle in the last call. */
  GdkRectangle      last_rect;

} ArrangerWidget;

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
#define ARRANGER_W_DECLARE_GET_CURSOR(cc,sc) \
  ArrangerCursor \
  sc##_arranger_widget_get_cursor ( \
    cc##ArrangerWidget * self, \
    UiOverlayAction action, \
    Tool            tool)

/**
 * Called when moving selected items in
 * drag update for moving up/down (changing Track
 * of a Region, changing MidiNote pitch, etc.).
 */
#define ARRANGER_W_DECLARE_MOVE_ITEMS_Y(cc,sc) \
  void \
  sc##_arranger_widget_move_items_y ( \
    cc##ArrangerWidget *self, \
    double              offset_y)

/**
 * To be called once at init time to set up the
 * arranger.
 */
#define ARRANGER_W_DECLARE_SETUP(cc,sc) \
  void \
  sc##_arranger_widget_setup ( \
    cc##ArrangerWidget * self)

/**
 * Called by an arranger widget during drag_update
 * to find and select (or delete) child objects
 * enclosed in the selection area.
 *
 * @param delete If this is a select-delete
 *   operation
 */
#define ARRANGER_W_DECLARE_SELECT(cc,sc) \
  void \
  sc##_arranger_widget_select ( \
    cc##ArrangerWidget * self, \
    const double             offset_x, \
    const double             offset_y, \
    const int                delete)

/**
 * Called on drag end to set default cursors back,
 * clear any start* variables, call actions, etc.
 */
#define ARRANGER_W_DECLARE_ON_DRAG_END(cc,sc) \
  void \
  sc##_arranger_widget_on_drag_end ( \
    cc##ArrangerWidget * self)

/**
 * Sets width to ruler width and height to the
 * corresponding height, if any (eg Tracklist
 * height for TimelineArrangerWidget).
 */
#define ARRANGER_W_DECLARE_SET_SIZE(cc,sc) \
  void \
  sc##_arranger_widget_set_size ( \
    cc##ArrangerWidget * self)

/**
 * Declares all functions that an arranger must
 * have.
 */
#define ARRANGER_W_DECLARE_FUNCS( \
  cc,sc) \
  ARRANGER_W_DECLARE_SET_ALLOCATION (cc, sc); \
  ARRANGER_W_DECLARE_GET_CURSOR (cc, sc); \
  ARRANGER_W_DECLARE_MOVE_ITEMS_Y (cc, sc); \
  ARRANGER_W_DECLARE_REFRESH_CHILDREN (cc, sc); \
  ARRANGER_W_DECLARE_SETUP (cc, sc); \
  ARRANGER_W_DECLARE_SELECT (cc, sc); \
  ARRANGER_W_DECLARE_ON_DRAG_END (cc, sc); \
  ARRANGER_W_DECLARE_SET_SIZE (cc, sc)

/**
 * Creates a timeline widget using the given
 * timeline data.
 */
void
arranger_widget_setup (
  ArrangerWidget *   self,
  ArrangerWidgetType type,
  SnapGrid *         snap_grid);

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor);

int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding);

void
arranger_widget_refresh_cursor (
  ArrangerWidget * self);

/**
 * Used for pushing transients to the bottom on
 * the z axis.
 */
void
arranger_widget_add_overlay_child (
  ArrangerWidget * self,
  ArrangerObject * obj);

/**
 * Sets transient object and actual object
 * visibility for every ArrangerObject in the
 * ArrangerWidget based on the current action.
 */
//void
//arranger_widget_update_visibility (
  //ArrangerWidget * self);

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self);

/**
 * Get all objects currently present in the arranger.
 *
 * @param objs Array to fill in.
 * @param size Array size to fill in.
 */
void
arranger_widget_get_all_objects (
  ArrangerWidget *  self,
  ArrangerObject ** objs,
  int *             size);

/**
 * Wrapper for ui_px_to_pos depending on the arranger
 * type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  int              has_padding);

/**
 * Refreshes all arranger backgrounds.
 */
void
arranger_widget_refresh_all_backgrounds (void);

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param rect The rectangle to search in.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  ArrangerObject **  array,
  int *              array_size);

/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  const double       x,
  const double       y);

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select);

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
int
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self);

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
ArrangerSelections *
arranger_widget_get_selections (
  ArrangerWidget * self);

/**
 * Selects the object, optionally appending it to
 * the selected items or making it the only
 * selected item.
 */
void
arranger_widget_select (
  ArrangerWidget * self,
  GType            type,
  void *           child,
  int              select,
  int              append);

/**
 * Readd children.
 */
int
arranger_widget_refresh (
  ArrangerWidget * self);

#endif
