/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef diDrawingTypes_h
#define diDrawingTypes_h

#include <string>

/// edit event types
enum editType{
  edit_value,               // set change_value
  edit_move,                // set move
  edit_gradient,            // set change_gradient
  edit_line,                // set line
  edit_line_smooth,         // set line with smoothing
  edit_line_limited,        // set limited line
  edit_line_limited_smooth, // set limited line with smoothing
  edit_smooth,              // set smoooth (inside influence)
  edit_replace_undef,       // set replace undefined values (inside influence)
  edit_pos,                 // position (in x,y)
  edit_undo,                // undo last change
  edit_redo,                // redo last undo
  edit_size,                // change size of influence (pos. in x,y)
  edit_inspection,          // some kind of inspection (pos. in x,y)
  edit_circle,              // use influence circle
  edit_square,              // use influence square
  edit_ellipse1,            // use influence ellipse(centre) if possible
  edit_ellipse2,            // use ellipse(focus)  if possible
  edit_ecellipse,           // use ellipse eccentricity (value in x)
  edit_exline_on,           // enable  drawing of extra lines during line editing
  edit_exline_off,          // disable drawing of extra lines during line editing
  edit_class_line,          // set class line
  edit_class_copy,          // set class copy
  edit_class_value,         // set a new class value (value in x)
  edit_set_undef,           // set values to undefined
  edit_lock_value,          // lock a value during edit_class_xxx, value in x
  edit_open_value,          // open a value during edit_class_xxx, value in x
  edit_show_numbers_on,     // for "number" editing
  edit_show_numbers_off,
  edit_noop
};

/// what kind of EditEvent
enum eventOrder {
  start_event,   // start of a new move/value action
  normal_event,  // event in ongoing action
  stop_event     // last event for this action
};

/// data for a complete edit event
struct EditEvent {
  editType type;    ///< type of event
  eventOrder order; ///< start, stop or normal event
  float x;          ///< mouse x position in map coordinates
  float y;          ///< mouse y position in map coordinates
};

const int numObjectTypes= 5;

const std::string ObjectTypeNames[numObjectTypes]=
{"edittool","front","symbol","area","anno"};

#endif // diDrawingTypes_h
