/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#if 0

/**
 * \file
 *
 * Widget for ChordObject.
 */

#ifndef __GUI_WIDGETS_CHORD_OBJECT_H__
#define __GUI_WIDGETS_CHORD_OBJECT_H__

#include "gui/widgets/arranger_object.h"

#include <gtk/gtk.h>

#define CHORD_OBJECT_WIDGET_TYPE \
  (chord_object_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordObjectWidget,
  chord_object_widget,
  Z, CHORD_OBJECT_WIDGET,
  ArrangerObjectWidget);

typedef struct ChordObjectObject ChordObjectObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_OBJECT_WIDGET_TRIANGLE_W 10

/**
 * Widget for chords inside the ChordObjectTrack.
 */
typedef struct _ChordObjectWidget
{
  ArrangerObjectWidget parent_instance;
  ChordObject *    chord_object;

  /** For double click. */
  GtkGestureMultiPress * mp;
} ChordObjectWidget;

/**
 * Creates a chord widget.
 */
ChordObjectWidget *
chord_object_widget_new (
  ChordObject * chord);

/**
 * @}
 */

#endif
#endif
