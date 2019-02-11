/*
 * gui/widgets/timeline_arranger.c - The timeline containing regions
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "actions/undoable_action.h"
#include "actions/create_chords_action.h"
#include "actions/undo_manager.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineArrangerWidget,
               timeline_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
timeline_arranger_widget_set_allocation (
  TimelineArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_REGION_WIDGET (widget))
    {
      RegionWidget * rw = Z_REGION_WIDGET (widget);
      REGION_WIDGET_GET_PRIVATE (rw);
      Track * track = rw_prv->region->track;
      /*TRACK_WIDGET_GET_PRIVATE (track->widget);*/

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (track->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px (
          &rw_prv->region->start_pos,
          1);
      allocation->y = wy;
      allocation->width =
        ui_pos_to_px (
          &rw_prv->region->end_pos,
          1) - allocation->x;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget)) -
        gtk_widget_get_allocated_height (
          track_widget_get_bottom_paned (track->widget));
    }
  else if (Z_IS_AUTOMATION_POINT_WIDGET (widget))
    {
      AutomationPointWidget * ap_widget =
        Z_AUTOMATION_POINT_WIDGET (widget);
      AutomationPoint * ap = ap_widget->ap;
      /*Automatable * a = ap->at->automatable;*/

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (ap->at->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px (&ap->pos, 1) -
        AP_WIDGET_POINT_SIZE / 2;
      allocation->y =
        (wy + automation_point_get_y_in_px (ap)) -
        AP_WIDGET_POINT_SIZE / 2;
      allocation->width = AP_WIDGET_POINT_SIZE;
      allocation->height = AP_WIDGET_POINT_SIZE;
    }
  else if (Z_IS_AUTOMATION_CURVE_WIDGET (widget))
    {
      AutomationCurveWidget * acw =
        Z_AUTOMATION_CURVE_WIDGET (widget);
      AutomationCurve * ac = acw->ac;
      /*Automatable * a = ap->at->automatable;*/

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (ac->at->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);
      AutomationPoint * prev_ap =
        automation_track_get_ap_before_curve (ac->at, ac);
      AutomationPoint * next_ap =
        automation_track_get_ap_after_curve (ac->at, ac);

      allocation->x =
        ui_pos_to_px (
          &prev_ap->pos,
          1) - AC_Y_HALF_PADDING;
      int prev_y = automation_point_get_y_in_px (prev_ap);
      int next_y = automation_point_get_y_in_px (next_ap);
      allocation->y = (wy + (prev_y > next_y ? next_y : prev_y) - AC_Y_HALF_PADDING);
      allocation->width =
        (ui_pos_to_px (
          &next_ap->pos,
          1) - allocation->x) + AC_Y_HALF_PADDING;
      allocation->height = (prev_y > next_y ? prev_y - next_y : next_y - prev_y) + AC_Y_PADDING;
    }
  else if (Z_IS_CHORD_WIDGET (widget))
    {
      ChordWidget * cw = Z_CHORD_WIDGET (widget);
      Track * track = (Track *) CHORD_TRACK;

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (track->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px (
          &cw->chord->pos,
          1);
      Position tmp;
      position_set_to_pos (&tmp,
                           &cw->chord->pos);
      position_set_beat (
        &tmp,
        tmp.beats + 1);
      allocation->y = wy;
      allocation->width =
        ui_pos_to_px (
          &tmp,
          1) - allocation->x;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
    }
}

Track *
timeline_arranger_widget_get_track_at_y (double y)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      GtkAllocation allocation;
      gtk_widget_get_allocation (GTK_WIDGET (track->widget),
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (track->widget),
                0,
                0,
                &wx,
                &wy);

      if (y > -wy && y < ((-wy) + allocation.height))
        {
          return track;
        }
    }
  return NULL;
}

AutomationTrack *
timeline_arranger_widget_get_automation_track_at_y (double y)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      AutomationTracklist * automation_tracklist =
        track_get_automation_tracklist (track);
      if (!automation_tracklist)
        continue;

      for (int j = 0;
           j < automation_tracklist->num_automation_tracks;
           j++)
        {
          AutomationTrack * at = automation_tracklist->automation_tracks[j];

          /* TODO check the rest */
          if (at->widget)
            {
              GtkAllocation allocation;
              gtk_widget_get_allocation (
                GTK_WIDGET (at->widget),
                &allocation);

              gint wx, wy;
              gtk_widget_translate_coordinates(
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (at->widget),
                0,
                y,
                &wx,
                &wy);

              if (wy >= 0 && wy <= allocation.height)
                {
                  return at;
                }
            }
        }
    }

  return NULL;
}

ChordWidget *
timeline_arranger_widget_get_hit_chord (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      CHORD_WIDGET_TYPE);
  if (widget)
    {
      return Z_CHORD_WIDGET (widget);
    }
  return NULL;
}

RegionWidget *
timeline_arranger_widget_get_hit_region (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      REGION_WIDGET_TYPE);
  if (widget)
    {
      return Z_REGION_WIDGET (widget);
    }
  return NULL;
}

AutomationPointWidget *
timeline_arranger_widget_get_hit_ap (
  TimelineArrangerWidget *  self,
  double            x,
  double            y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      AUTOMATION_POINT_WIDGET_TYPE);
  if (widget)
    {
      return Z_AUTOMATION_POINT_WIDGET (widget);
    }
  return NULL;
}

AutomationCurveWidget *
timeline_arranger_widget_get_hit_curve (
  TimelineArrangerWidget * self,
  double x,
  double y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      AUTOMATION_CURVE_WIDGET_TYPE);
  if (widget)
    {
      return Z_AUTOMATION_CURVE_WIDGET (widget);
    }
  return NULL;
}

void
timeline_arranger_widget_update_inspector (
  TimelineArrangerWidget *self)
{
  inspector_widget_show_selections (
    INSPECTOR_CHILD_REGION,
    (void **) TIMELINE_SELECTIONS->regions,
    TIMELINE_SELECTIONS->num_regions);
  inspector_widget_show_selections (
    INSPECTOR_CHILD_AP,
    (void **) TIMELINE_SELECTIONS->automation_points,
    TIMELINE_SELECTIONS->num_automation_points);
  inspector_widget_show_selections (
    INSPECTOR_CHILD_CHORD,
    (void **) TIMELINE_SELECTIONS->chords,
    TIMELINE_SELECTIONS->num_chords);
}

void
timeline_arranger_widget_select_all (
  TimelineArrangerWidget *  self,
  int                       select)
{
  TIMELINE_SELECTIONS->num_regions = 0;
  TIMELINE_SELECTIONS->num_automation_points = 0;

  /* select chords */
  ChordTrack * ct = tracklist_get_chord_track ();
  for (int i = 0; i < ct->num_chords; i++)
    {
      Chord * chord = ct->chords[i];
      if (chord->visible)
        {
          chord_widget_select (chord->widget, select);

          if (select)
            {
              /* select  */
              array_append (
                TIMELINE_SELECTIONS->chords,
                TIMELINE_SELECTIONS->num_chords,
                chord);
            }
        }
    }

  /* select everything else */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * chan = MIXER->channels[i];

      if (chan->visible)
        {
          int num_regions;
          InstrumentTrack * it;
          AudioTrack *      at;
          AutomationTracklist * automation_tracklist =
            track_get_automation_tracklist (chan->track);
          if (chan->track->type ==
                TRACK_TYPE_INSTRUMENT)
            {
              it = (InstrumentTrack *) chan->track;
              num_regions = it->num_regions;
            }
          else if (chan->track->type ==
                    TRACK_TYPE_AUDIO)
            {
              at = (AudioTrack *) chan->track;
              num_regions = at->num_regions;
            }
          for (int j = 0; j < num_regions; j++)
            {
              Region * r;

              if (chan->track->type ==
                    TRACK_TYPE_INSTRUMENT)
                r = (Region *) it->regions[j];
              else if (chan->track->type ==
                         TRACK_TYPE_AUDIO)
                r = (Region *) at->regions[j];

              region_widget_select (r->widget, select);

              if (select)
                {
                  /* select  */
                  array_append (
                    TIMELINE_SELECTIONS->regions,
                    TIMELINE_SELECTIONS->num_regions,
                    r);
                }
            }
          if (chan->track->bot_paned_visible)
            {
              for (int j = 0;
                   j < automation_tracklist->num_automation_tracks;
                   j++)
                {
                  AutomationTrack * at =
                    automation_tracklist->automation_tracks[j];
                  if (at->visible)
                    {
                      for (int k = 0; k < at->num_automation_points; k++)
                        {
                          /*AutomationPoint * ap = at->automation_points[k];*/

                          /*ap_widget_select (ap->widget, select);*/

                          /*if (select)*/
                            /*{*/
                              /*[> select  <]*/
                              /*array_append ((void **)self->tl_automation_points,*/
                                            /*&self->num_tl_automation_points,*/
                                            /*ap);*/
                            /*}*/
                        }
                    }
                }
            }
        }
    }
}

/**
 * Selects the region.
 */
void
timeline_arranger_widget_toggle_select_region (
  TimelineArrangerWidget * self,
  Region *                 region,
  int                      append)
{
  arranger_widget_toggle_select (
    Z_ARRANGER_WIDGET (self),
    REGION_WIDGET_TYPE,
    (void *) region,
    append);
}

/* FIXME should be macro? */
void
timeline_arranger_widget_toggle_select_chord (
  TimelineArrangerWidget * self,
  Chord *                  chord,
  int                      append)
{
  arranger_widget_toggle_select (
    Z_ARRANGER_WIDGET (self),
    CHORD_WIDGET_TYPE,
    (void *) chord,
    append);
}

void
timeline_arranger_widget_toggle_select_automation_point (
  TimelineArrangerWidget *  self,
  AutomationPoint * ap,
  int               append)
{
  arranger_widget_toggle_select (
    Z_ARRANGER_WIDGET (self),
    AUTOMATION_POINT_WIDGET_TYPE,
    (void *) ap,
    append);
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
timeline_arranger_widget_show_context_menu (
  TimelineArrangerWidget * self)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

void
timeline_arranger_widget_on_drag_begin_region_hit (
  TimelineArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  RegionWidget *           rw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  REGION_WIDGET_GET_PRIVATE (rw);

  /* open MIDI editor */
  if (ar_prv->n_press == 2)
    {
      Track * track = rw_prv->region->track;
      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          piano_roll_set_track (track);
        }
    }

  Region * region = rw_prv->region;
  self->start_region = rw_prv->region;

  /* update arranger action */
  if (region->type == REGION_TYPE_MIDI &&
           rw_prv->cursor_state ==
             UI_CURSOR_STATE_RESIZE_L)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_L;
  else if (region->type == REGION_TYPE_MIDI &&
           rw_prv->cursor_state ==
              UI_CURSOR_STATE_RESIZE_R)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  else
    {
      ar_prv->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      ui_set_cursor (GTK_WIDGET (rw), "grabbing");
    }

  /* select/ deselect regions */
  if (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK)
    {
      /* if ctrl pressed toggle on/off */
      timeline_arranger_widget_toggle_select_region (
        self, region, 1);
    }
  else if (!array_contains ((void **)TIMELINE_SELECTIONS->regions,
                      TIMELINE_SELECTIONS->num_regions,
                      region))
    {
      /* else if not already selected select only it */
      timeline_arranger_widget_select_all (self, 0);
      timeline_arranger_widget_toggle_select_region (self, region, 0);
    }

  /* find highest and lowest selected regions */
  TIMELINE_SELECTIONS->top_region = TIMELINE_SELECTIONS->regions[0];
  TIMELINE_SELECTIONS->bot_region = TIMELINE_SELECTIONS->regions[0];
  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * region = TIMELINE_SELECTIONS->regions[i];
      if (tracklist_get_track_pos (region->track) <
          tracklist_get_track_pos (TIMELINE_SELECTIONS->top_region->track))
        {
          TIMELINE_SELECTIONS->top_region = region;
        }
      if (tracklist_get_track_pos (region->track) >
          tracklist_get_track_pos (TIMELINE_SELECTIONS->bot_region->track))
        {
          TIMELINE_SELECTIONS->bot_region = region;
        }
    }
}

void
timeline_arranger_widget_on_drag_begin_chord_hit (
  TimelineArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  ChordWidget *            cw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* if double click */
  if (ar_prv->n_press == 2)
    {
    }

  Chord * chord = cw->chord;
  self->start_chord = chord;

  /* update arranger action */
  ar_prv->action = UI_OVERLAY_ACTION_STARTING_MOVING;
  ui_set_cursor (GTK_WIDGET (cw), "grabbing");

  /* select/ deselect chords */
  if (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK)
    {
      /* if ctrl pressed toggle on/off */
      timeline_arranger_widget_toggle_select_chord (
        self, chord, 1);
    }
  else if (!array_contains (
             (void **)TIMELINE_SELECTIONS->chords,
             TIMELINE_SELECTIONS->num_chords,
             chord))
    {
      /* else if not already selected select only it */
      timeline_arranger_widget_select_all (
        self, 0);
      timeline_arranger_widget_toggle_select_chord (
        self, chord, 0);
    }
}

void
timeline_arranger_widget_on_drag_begin_ap_hit (
  TimelineArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  AutomationPointWidget *  ap_widget)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  AutomationPoint * ap = ap_widget->ap;
  ar_prv->start_pos_px = start_x;
  self->start_ap = ap;
  if (!array_contains ((void **) TIMELINE_SELECTIONS->automation_points,
                       TIMELINE_SELECTIONS->num_automation_points,
                       (void *) ap))
    {
      TIMELINE_SELECTIONS->automation_points[0] = ap;
      TIMELINE_SELECTIONS->num_automation_points = 1;
    }
  switch (ap_widget->state)
    {
    case APW_STATE_NONE:
      g_warning ("hitting AP but AP hover state is none, should be fixed");
      break;
    case APW_STATE_HOVER:
    case APW_STATE_SELECTED:
      ar_prv->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      ui_set_cursor (GTK_WIDGET (ap_widget), "grabbing");
      break;
    }

  /* update selection */
  if (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK)
    {
      timeline_arranger_widget_toggle_select_automation_point (
                                                                    self, ap, 1);
    }
  else
    {
      timeline_arranger_widget_select_all (self, 0);
      timeline_arranger_widget_toggle_select_automation_point (self, ap, 0);
    }
}

void
timeline_arranger_widget_find_start_poses (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * r = TIMELINE_SELECTIONS->regions[i];
      if (position_compare (&r->start_pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->start_pos);
        }

      /* set start poses for regions */
      position_set_to_pos (&self->region_start_poses[i],
                           &r->start_pos);
    }
  for (int i = 0; i < TIMELINE_SELECTIONS->num_chords; i++)
    {
      Chord * r = TIMELINE_SELECTIONS->chords[i];
      if (position_compare (&r->pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->pos);
        }

      /* set start poses for chords */
      position_set_to_pos (&self->chord_start_poses[i],
                           &r->pos);
    }
  for (int i = 0; i < TIMELINE_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap = TIMELINE_SELECTIONS->automation_points[i];
      if (position_compare (&ap->pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &ap->pos);
        }

      /* set start poses for APs */
      position_set_to_pos (&self->ap_poses[i],
                           &ap->pos);
    }
}

void
timeline_arranger_widget_create_ap (
  TimelineArrangerWidget * self,
  AutomationTrack *        at,
  Track *                  track,
  Position *               pos,
  double                   start_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
    position_snap (NULL,
                   pos,
                   track,
                   NULL,
                   ar_prv->snap_grid);

  /* if the automatable is float in this automation track */
  if (automatable_is_float (at->automatable))
    {
      /* add automation point to automation track */
      float value =
        automation_track_widget_get_fvalue_at_y (
          at->widget,
          start_y);

      AutomationPoint * ap =
        automation_point_new_float (
          at,
          value,
          pos);
      automation_track_add_automation_point (
        at,
        ap,
        GENERATE_CURVE_POINTS);
      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                               GTK_WIDGET (ap->widget));
      gtk_widget_show (GTK_WIDGET (ap->widget));
      TIMELINE_SELECTIONS->automation_points[0] = ap;
      TIMELINE_SELECTIONS->num_automation_points = 1;
    }
}

void
timeline_arranger_widget_create_region (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos)
{
  if (track->type == TRACK_TYPE_AUDIO)
    return;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
    {
      position_snap (NULL,
                     pos,
                     track,
                     NULL,
                     ar_prv->snap_grid);
    }
  Region * region;
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      region = (Region *) midi_region_new (track,
                               pos,
                               pos);
    }
  position_set_min_size (&region->start_pos,
                         &region->end_pos,
                         ar_prv->snap_grid);
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      instrument_track_add_region ((InstrumentTrack *)track,
                        (MidiRegion *) region);
    }
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (region->widget));
  gtk_widget_show (GTK_WIDGET (region->widget));
  ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  TIMELINE_SELECTIONS->regions[0] = region;
  TIMELINE_SELECTIONS->num_regions = 1;
}

void
timeline_arranger_widget_create_chord (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
    position_snap (NULL,
                   pos,
                   track,
                   NULL,
                   ar_prv->snap_grid);
 Chord * chord = chord_new (NOTE_A,
                            1,
                            NOTE_A,
                            CHORD_TYPE_MIN,
                            0);
 position_set_to_pos (&chord->pos,
                      pos);
 Chord * chords[1] = { chord };
 UndoableAction * action =
   create_chords_action_new (chords, 1);
 undo_manager_perform (UNDO_MANAGER,
                       action);
  ar_prv->action = UI_OVERLAY_ACTION_NONE;
  TIMELINE_SELECTIONS->chords[0] = chord;
  TIMELINE_SELECTIONS->num_chords = 1;
}

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (
  TimelineArrangerWidget * self,
  double                   y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  Track * track =
    timeline_arranger_widget_get_track_at_y (y);

  if (track)
    {
      /* determine selection type based on click
       * position */
      GtkAllocation allocation;
      gtk_widget_get_allocation (
        GTK_WIDGET (track->widget),
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (self),
        GTK_WIDGET (track->widget),
        0,
        y,
        &wx,
        &wy);

      /* if bot half, select range */
      if (wy >= allocation.height / 2 &&
          wy <= allocation.height)
        {
          self->selection_type =
            TA_SELECTION_TYPE_RANGE;
        }
      else /* if top half, select objects */
        {
          self->selection_type =
            TA_SELECTION_TYPE_OBJECTS;

          /* deselect all */
          arranger_widget_select_all (self, 0);
        }
    }
  else
    {
      /* TODO something similar as above based on
       * visible space */
      self->selection_type =
        TA_SELECTION_TYPE_OBJECTS;

      /* deselect all */
      arranger_widget_select_all (self, 0);
    }
}

/**
 * First determines the selection type (objects/
 * range), then either finds and selects items or
 * selects a range.
 */
void
timeline_arranger_widget_select (
  TimelineArrangerWidget * self,
  double                   offset_x,
  double                   offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (self->selection_type ==
      TA_SELECTION_TYPE_OBJECTS)
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);

      /* find enclosed regions */
      GtkWidget *    region_widgets[800];
      int            num_region_widgets = 0;
      arranger_widget_get_hit_widgets_in_range (
        Z_ARRANGER_WIDGET (self),
        REGION_WIDGET_TYPE,
        ar_prv->start_x,
        ar_prv->start_y,
        offset_x,
        offset_y,
        region_widgets,
        &num_region_widgets);


      /* select the enclosed regions */
      for (int i = 0; i < num_region_widgets; i++)
        {
          RegionWidget * rw =
            Z_REGION_WIDGET (region_widgets[i]);
          REGION_WIDGET_GET_PRIVATE (rw);
          Region * region = rw_prv->region;
          timeline_arranger_widget_toggle_select_region (
            self,
            region,
            1);
        }

      /* find enclosed chords */
      GtkWidget *    chord_widgets[800];
      int            num_chord_widgets = 0;
      arranger_widget_get_hit_widgets_in_range (
        Z_ARRANGER_WIDGET (self),
        CHORD_WIDGET_TYPE,
        ar_prv->start_x,
        ar_prv->start_y,
        offset_x,
        offset_y,
        chord_widgets,
        &num_chord_widgets);


      /* select the enclosed chords */
      for (int i = 0; i < num_chord_widgets; i++)
        {
          ChordWidget * rw =
            Z_CHORD_WIDGET (chord_widgets[i]);
          Chord * chord = rw->chord;
          timeline_arranger_widget_toggle_select_chord (
            self,
            chord,
            1);
        }

      /* find enclosed automation_points */
      GtkWidget *    ap_widgets[800];
      int            num_ap_widgets = 0;
      arranger_widget_get_hit_widgets_in_range (
        Z_ARRANGER_WIDGET (self),
        AUTOMATION_POINT_WIDGET_TYPE,
        ar_prv->start_x,
        ar_prv->start_y,
        offset_x,
        offset_y,
        ap_widgets,
        &num_ap_widgets);

      /* select the enclosed automation_points */
      for (int i = 0; i < num_ap_widgets; i++)
        {
          AutomationPointWidget * ap_widget =
            Z_AUTOMATION_POINT_WIDGET (ap_widgets[i]);
          AutomationPoint * ap = ap_widget->ap;
          timeline_arranger_widget_toggle_select_automation_point (self,
                                                          ap,
                                                          1);
        }
    } /* end if selecting objects */
  /* if selecting range */
  else if (self->selection_type ==
           TA_SELECTION_TYPE_RANGE)
    {
      /* set range */
      PROJECT->has_range = 1;
      ui_px_to_pos (
        ar_prv->start_x,
        &PROJECT->range_1,
        1);
      ui_px_to_pos (
        ar_prv->start_x + offset_x,
        &PROJECT->range_2,
        1);
    }
}

void
timeline_arranger_widget_snap_regions_l (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * region = TIMELINE_SELECTIONS->regions[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
        position_snap (NULL,
                       pos,
                       region->track,
                       NULL,
                       ar_prv->snap_grid);
      region_set_start_pos (region,
                            pos,
                            0);
    }
}

void
timeline_arranger_widget_snap_regions_r (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * region = TIMELINE_SELECTIONS->regions[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
        position_snap (NULL,
                       pos,
                       region->track,
                       NULL,
                       ar_prv->snap_grid);
      if (position_compare (pos, &region->start_pos) > 0)
        {
          region_set_end_pos (region,
                              pos);
        }
    }
}

void
timeline_arranger_widget_move_items_x (
  TimelineArrangerWidget * self,
  long                     ticks_diff)
{
  /* update region positions */
  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * r = TIMELINE_SELECTIONS->regions[i];
      Position * prev_start_pos = &self->region_start_poses[i];
      long length_ticks = position_to_ticks (&r->end_pos) -
        position_to_ticks (&r->start_pos);
      Position tmp;
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff + length_ticks);
      region_set_end_pos (r, &tmp);
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff);
      region_set_start_pos (r, &tmp, 1);
    }

  /* update chord positions */
  for (int i = 0; i < TIMELINE_SELECTIONS->num_chords; i++)
    {
      Chord * r = TIMELINE_SELECTIONS->chords[i];
      Position * prev_start_pos =
        &self->chord_start_poses[i];
      Position tmp;
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff);
      chord_set_pos (r, &tmp);
    }

  /* update ap positions */
  for (int i = 0; i < TIMELINE_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap = TIMELINE_SELECTIONS->automation_points[i];

      /* get prev and next value APs */
      AutomationPoint * prev_ap = automation_track_get_prev_ap (ap->at,
                                                                ap);
      AutomationPoint * next_ap = automation_track_get_next_ap (ap->at,
                                                                ap);
      /* get adjusted pos for this automation point */
      Position ap_pos;
      Position * prev_pos = &self->ap_poses[i];
      position_set_to_pos (&ap_pos,
                           prev_pos);
      position_add_ticks (&ap_pos, ticks_diff);

      Position mid_pos;
      AutomationCurve * ac;

      /* update midway points */
      if (prev_ap && position_compare (&ap_pos, &prev_ap->pos) >= 0)
        {
          /* set prev curve point to new midway pos */
          position_get_midway_pos (&prev_ap->pos,
                                   &ap_pos,
                                   &mid_pos);
          ac = automation_track_get_next_curve_ac (ap->at,
                                                   prev_ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap */
          if (!next_ap)
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      if (next_ap && position_compare (&ap_pos, &next_ap->pos) <= 0)
        {
          /* set next curve point to new midway pos */
          position_get_midway_pos (&ap_pos,
                                   &next_ap->pos,
                                   &mid_pos);
          ac = automation_track_get_next_curve_ac (ap->at,
                                                   ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap - if no prev ap exists or if the position
           * is also after the prev ap */
          if ((prev_ap &&
                position_compare (&ap_pos, &prev_ap->pos) >= 0) ||
              (!prev_ap))
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      else if (!prev_ap && !next_ap)
        {
          /* set pos for ap */
          position_set_to_pos (&ap->pos, &ap_pos);
        }
    }
}

void
timeline_arranger_widget_setup (
  TimelineArrangerWidget * self)
{
  // set the size
  int ww, hh;
  TracklistWidget * tracklist = MW_TRACKLIST;
  gtk_widget_get_size_request (
    GTK_WIDGET (tracklist),
    &ww,
    &hh);
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    hh);
}

void
timeline_arranger_widget_move_items_y (
  TimelineArrangerWidget * self,
  double                   offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (self->start_region)
    {
      /* check if should be moved to new track */
      Track * track = timeline_arranger_widget_get_track_at_y (ar_prv->start_y + offset_y);
      Track * old_track = self->start_region->track;
      if (track)
        {
          Track * pt =
            tracklist_get_prev_visible_track (
              old_track);
          Track * nt =
            tracklist_get_next_visible_track (
              old_track);
          Track * tt =
            tracklist_get_first_visible_track ();
          Track * bt =
            tracklist_get_last_visible_track ();
          if (self->start_region->track != track)
            {
              /* if new track is lower and bot region is not at the lowest track */
              if (track == nt &&
                  TIMELINE_SELECTIONS->bot_region->track != bt)
                {
                  /* shift all selected regions to their next track */
                  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
                    {
                      Region * region = TIMELINE_SELECTIONS->regions[i];
                      nt =
                        tracklist_get_next_visible_track (
                          region->track);
                      old_track = region->track;
                      if (old_track->type == nt->type)
                        {
                          if (nt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              instrument_track_remove_region ((InstrumentTrack *) old_track, (MidiRegion *) region);
                              instrument_track_add_region ((InstrumentTrack *) nt, (MidiRegion *) region);
                            }
                          else if (nt->type ==
                                     TRACK_TYPE_AUDIO)
                            {
                              /* TODO */

                            }
                        }
                    }
                }
              else if (track == pt &&
                       TIMELINE_SELECTIONS->top_region->track != tt)
                {
                  /*g_message ("track %s top region track %s tt %s",*/
                             /*track->channel->name,*/
                             /*self->top_region->track->channel->name,*/
                             /*tt->channel->name);*/
                  /* shift all selected regions to their prev track */
                  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
                    {
                      Region * region = TIMELINE_SELECTIONS->regions[i];
                      pt =
                        tracklist_get_prev_visible_track (
                          region->track);
                      old_track = region->track;
                      if (old_track->type == pt->type)
                        {
                          if (pt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              instrument_track_remove_region ((InstrumentTrack *) old_track, (MidiRegion *) region);
                              instrument_track_add_region ((InstrumentTrack *) pt, (MidiRegion *) region);
                            }
                          else if (pt->type ==
                                     TRACK_TYPE_AUDIO)
                            {
                              /* TODO */

                            }
                        }
                    }
                }
            }
        }
    }
  else if (self->start_ap)
    {
      for (int i = 0; i < TIMELINE_SELECTIONS->num_automation_points; i++)
        {
          AutomationPoint * ap = TIMELINE_SELECTIONS->automation_points[i];

          /* get adjusted y for this ap */
          /*Position region_pos;*/
          /*position_set_to_pos (&region_pos,*/
                               /*&pos);*/
          /*int diff = position_to_frames (&region->start_pos) -*/
            /*position_to_frames (&self->tl_start_region->start_pos);*/
          /*position_add_frames (&region_pos, diff);*/
          int this_y =
            automation_track_widget_get_y (ap->at->widget,
                                           ap->widget);
          int start_ap_y =
            automation_track_widget_get_y (self->start_ap->at->widget,
                                           self->start_ap->widget);
          int diff = this_y - start_ap_y;

          float fval =
            automation_track_widget_get_fvalue_at_y (ap->at->widget,
                                                     ar_prv->start_y + offset_y + diff);
          automation_point_update_fvalue (ap, fval);
        }
    }
}

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
void
timeline_arranger_widget_on_drag_end (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0; i < TIMELINE_SELECTIONS->num_regions; i++)
    {
      Region * region = TIMELINE_SELECTIONS->regions[i];
      ui_set_cursor (GTK_WIDGET (region->widget), "default");
    }
  for (int i = 0; i < TIMELINE_SELECTIONS->num_chords; i++)
    {
      Chord * chord = TIMELINE_SELECTIONS->chords[i];
      ui_set_cursor (GTK_WIDGET (chord->widget), "default");
    }

  /* if didn't click on something */
  if (ar_prv->action != UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      self->start_region = NULL;
      self->start_ap = NULL;
    }
}

static void
add_children_from_chord_track (
  TimelineArrangerWidget * self,
  ChordTrack *             ct)
{
  for (int i = 0; i < ct->num_chords; i++)
    {
      Chord * chord = ct->chords[i];
      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                               GTK_WIDGET (chord->widget));
    }
}

static void
add_children_from_instrument_track (
  TimelineArrangerWidget * self,
  InstrumentTrack *        it)
{
  for (int i = 0; i < it->num_regions; i++)
    {
      MidiRegion * mr = it->regions[i];
      Region * r = (Region *) mr;
      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                               GTK_WIDGET (r->widget));
    }
  ChannelTrack * ct = (ChannelTrack *) it;
  AutomationTracklist * atl = &ct->automation_tracklist;
  for (int i = 0; i < atl->num_automation_tracks; i++)
    {
      AutomationTrack * at = atl->automation_tracks[i];
      if (at->visible)
        {
          for (int j = 0; j < at->num_automation_points; j++)
            {
              AutomationPoint * ap =
                at->automation_points[j];
              if (ap->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (self),
                    GTK_WIDGET (ap->widget));
                }
            }
          for (int j = 0; j < at->num_automation_curves; j++)
            {
              AutomationCurve * ac =
                at->automation_curves[j];
              if (ac->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (self),
                    GTK_WIDGET (ac->widget));
                }
            }
        }
    }
}

static void
add_children_from_audio_track (
  TimelineArrangerWidget * self,
  AudioTrack *             audio_track)
{
  g_message ("adding children");
  for (int i = 0; i < audio_track->num_regions; i++)
    {
      AudioRegion * mr = audio_track->regions[i];
      g_message ("adding region");
      Region * r = (Region *) mr;
      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                               GTK_WIDGET (r->widget));
    }
  ChannelTrack * ct = (ChannelTrack *) audio_track;
  AutomationTracklist * atl = &ct->automation_tracklist;
  for (int i = 0; i < atl->num_automation_tracks; i++)
    {
      AutomationTrack * at = atl->automation_tracks[i];
      if (at->visible)
        {
          for (int j = 0; j < at->num_automation_points; j++)
            {
              AutomationPoint * ap =
                at->automation_points[j];
              if (ap->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (self),
                    GTK_WIDGET (ap->widget));
                }
            }
          for (int j = 0; j < at->num_automation_curves; j++)
            {
              AutomationCurve * ac =
                at->automation_curves[j];
              if (ac->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (self),
                    GTK_WIDGET (ac->widget));
                }
            }
        }
    }
}

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg && playhead */
  GList *children, *iter;

  children =
    gtk_container_get_children (GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          g_object_ref (widget);
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->visible)
        {
          switch (track->type)
            {
            case TRACK_TYPE_CHORD:
              add_children_from_chord_track (
                self,
                (ChordTrack *) track);
              break;
            case TRACK_TYPE_INSTRUMENT:
              add_children_from_instrument_track (
                self,
                (InstrumentTrack *) track);
              break;
            case TRACK_TYPE_MASTER:
              break;
            case TRACK_TYPE_AUDIO:
              add_children_from_audio_track (
                self,
                (AudioTrack *) track);
              break;
            case TRACK_TYPE_BUS:
              break;
            }
        }
    }
}

/**
 * Scroll to the given position.
 */
void
timeline_arranger_widget_scroll_to (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  /* TODO */

}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  g_message ("timeline focused");
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

static void
timeline_arranger_widget_class_init (TimelineArrangerWidgetClass * klass)
{
}

static void
timeline_arranger_widget_init (TimelineArrangerWidget *self )
{
  g_signal_connect (self,
                    "grab-focus",
                    G_CALLBACK (on_focus),
                    self);
}
