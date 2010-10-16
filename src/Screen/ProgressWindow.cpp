/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/ProgressWindow.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Version.hpp"
#include "resource.h"

#include <algorithm>

using std::min;

ProgressWindow::ProgressWindow(ContainerWindow &parent)
  :background_color(Color::WHITE),
   background_brush(background_color),
   position(0)
{
  RECT rc = parent.get_client_rect();
  set(parent, rc.left, rc.top, rc.right, rc.bottom);

  unsigned width = rc.right - rc.left, height = rc.bottom - rc.top;

  // Load logo
  bitmap_logo.load(IDB_SWIFT2);
  // Load progress bar background
  bitmap_progress_border.load(IDB_PROGRESSBORDER);

  // Adjust the title to larger screens
  bitmap_title.load(width > 272 && height > 272 ? IDB_TITLE_HD : IDB_TITLE);

  // Determine text height
  VirtualCanvas canvas(1, 1);
  text_height = canvas.text_height(_T("W"));

  // Make progress bar height proportional to window height
  progress_border_height = height / 10;
  unsigned progress_size = progress_border_height - (height/20);
  unsigned progress_horizontal_border = (progress_border_height - progress_size)/2;

  // Initialize version text field
  TextWindowStyle version_style;
  version_style.left();
  version.set(*this, XCSoar_ProductToken,
              0, 0, width - progress_size * 2, text_height, version_style);

  // Initialize message text field
  TextWindowStyle message_style;
  message_style.center();
  message.set(*this, NULL, 0,
              height - progress_border_height - text_height - (height/48),
              width, text_height, message_style);

  // Initialize progress bar
  ProgressBarStyle pb_style;
  progress_bar.set(*this, progress_horizontal_border,
                   height - progress_border_height + (progress_size/2),
                   width - progress_horizontal_border*2,
                   progress_size, pb_style);

  version.install_wndproc(); // needed for on_color()
  message.install_wndproc(); // needed for on_color()

  // Set progress bar step size and range
  set_range(0, 1000);
  set_step(50);

  // Show dialog
  bring_to_top();
  update();
}

void
ProgressWindow::set_message(const TCHAR *text)
{
  assert_none_locked();
  assert_thread();

  message.set_text(text);
}

void
ProgressWindow::set_range(unsigned min_value, unsigned max_value)
{
  progress_bar.set_range(min_value, max_value);
}

void
ProgressWindow::set_step(unsigned size)
{
  progress_bar.set_step(size);
}

void
ProgressWindow::set_pos(unsigned value)
{
  assert_none_locked();
  assert_thread();

  if (value == position)
    return;

  position = value;
  progress_bar.set_position(value);
}

void
ProgressWindow::step()
{
  progress_bar.step();
}

bool
ProgressWindow::on_erase(Canvas &canvas)
{
  canvas.clear(background_brush);
  return true;
}

void
ProgressWindow::on_paint(Canvas &canvas)
{
  BitmapCanvas bitmap_canvas(canvas, bitmap_logo);

  int window_width = canvas.get_width();
  int window_height = canvas.get_height();
  int logo_width = bitmap_canvas.get_width();
  int logo_height = bitmap_canvas.get_height();

  bitmap_canvas.select(bitmap_title);

  int title_width = bitmap_canvas.get_width();
  int title_height = bitmap_canvas.get_height();

  int logox,logoy,titlex,titley;

  bool hidetitle = false;

  if (window_width > window_height){ //Landscape
    logox = (window_width - 
              (logo_width + title_height + title_width))/2;
    logoy = (window_height - logo_height)/2-text_height;
    titlex = logox + logo_width + title_height;
    titley = (window_height - title_height)/2 - text_height;
  }
  else{ 
    if (window_width < window_height){ //Portrait
      logox = (window_width - logo_width)/2;
      logoy = (window_height - 
                (logo_height + title_height + title_height))/2
              - text_height;
      titlex = (window_width - title_width)/2;
      titley = logoy + logo_height + title_height;
    }
    else{ //Square screen
      logox = (window_width - logo_width)/2;
      logoy = (window_height - logo_height)/2 - text_height;
      hidetitle = true;      
    }
  }

  if (!hidetitle){
    //Draw 'XCSoar N.N' title
    canvas.copy(titlex, titley,
                title_width, title_height,
                bitmap_canvas, 0, 0);
  }
  bitmap_canvas.select(bitmap_logo);

  //Draw XCSoar swift logo
  canvas.copy(logox, logoy,
              logo_width, logo_height,
              bitmap_canvas, 0, 0);

  bitmap_canvas.select(bitmap_progress_border);

  int progress_bitmap_width = bitmap_canvas.get_width();
  int progress_bitmap_height = bitmap_canvas.get_height();

  //Stretch progress bar border
  canvas.stretch(0,
                 (window_height - progress_border_height),
                 window_width, progress_border_height,
                 bitmap_canvas, 0, 0,
                 progress_bitmap_width, progress_bitmap_height);
}

Brush *
ProgressWindow::on_color(Window &window, Canvas &canvas)
{
  canvas.set_text_color(Color::BLACK);
  canvas.set_background_color(background_color);
  return &background_brush;
}
