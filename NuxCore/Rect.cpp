/*
 * Copyright 2010 Inalogic® Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as
 * published by the  Free Software Foundation; either version 2.1 or 3.0
 * of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */


#include "NuxCore.h"
#include "Point.h"
#include "Size.h"
#include "Rect.h"

namespace nux
{
  Rect::Rect()
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;

  }

  Rect::Rect (int x_, int y_, int width_, int height_)
  {
    x = x_;
    y = y_;
    width = Max<int> (0, int (width_) );
    height = Max<int> (0, int (height_) );
  }

  Rect::~Rect()
  {

  }

  Rect::Rect (const Rect &r)
  {
    x = r.x;
    y = r.y;
    width = r.width;
    height = r.height;
  }

  Rect &Rect::operator = (const Rect &r)
  {
    if (&r == this)
      return *this;

    x = r.x;
    y = r.y;
    width = r.width;
    height = r.height;
    return *this;
  }

  bool Rect::operator == (const Rect &r) const
  {
    if ( (x == r.x) && (y == r.y) && (width == r.width) && (height == r.height) )
    {
      return true;
    }

    return false;
  }

  bool Rect::operator != (const Rect &r) const
  {
    if ( (x == r.x) && (y == r.y) && (width == r.width) && (height == r.height) )
    {
      return false;
    }

    return true;
  }

  bool Rect::IsNull() const
  {
    return ((width == 0) || (height == 0));
  }

  void Rect::Set (int px, int py, int w, int h)
  {
    x = px;
    y = py;
    width = w;
    height = h;
  }

  void Rect::SetPosition (int px, int py)
  {
    x = px;
    y = py;
  }

  void Rect::SetSize (int w, int h)
  {
    width = w;
    height = h;
  }

  bool Rect::IsInside (const Point &p) const
  {
    return ( (x <= p.x) && (x + width > p.x) &&
             (y <= p.y) && (y + height > p.y) );
  }

  bool Rect::IsPointInside (int x_, int y_) const
  {
    return ( (x <= x_) && (x + width > x_) &&
             (y <= y_) && (y + height > y_) );
  }

  Rect Rect::Intersect (const Rect &r) const
  {
    // Get the corner points.

    bool intersect = ! ((r.x > x + width) ||
                      (r.x + r.width < x) ||
                      (r.y > y + height) ||
                      (r.y + r.height < y));

    if (intersect)
    {
      const Point &ul1 = Point (x, y);
      const Point &ul2 = Point (r.x, r.y);
      int xx = Max<int> (ul1.x, ul2.x);
      int yy = Max<int> (ul1.y, ul2.y);
      int ww = Min<int> (ul1.x + width,  ul2.x + r.width) - xx;
      int hh = Min<int> (ul1.y + height, ul2.y + r.height) - yy;

      return Rect (xx, yy, ww, hh);
    }
    else
    {
      // No intersection
      return Rect ();
    }
  }

  // expand the width by factor_x and the height by factor_y
  void Rect::Expand (int dx, int dy)
  {
    if (!IsNull() )
    {
      x -= dx;
      y -= dy;
      width  += 2 * dx;
      height += 2 * dy;
    }
  }

  // expand the width by factor_x and the height by factor_y
  Rect Rect::GetExpand (int dx, int dy) const
  {
    Rect r = Rect (x - dx, y - dy, width + 2 * dx, height + 2 * dy);

    if (r.IsNull() )
    {
      return Rect (0, 0, 0, 0);
    }

    return r;
  }


  Rect operator+(Rect const& lhs, Rect const& rhs)
  {
    return Rect(lhs.x + rhs.x,
                lhs.y + rhs.y,
                lhs.width + rhs.width,
                lhs.height + rhs.height);
  }

  Rect operator-(Rect const& lhs, Rect const& rhs)
  {
    return Rect(lhs.x - rhs.x,
                lhs.y - rhs.y,
                lhs.width - rhs.width,
                lhs.height - rhs.height);
  }

  Rect operator*(Rect const& lhs, float scalar)
  {
    return Rect(lhs.x * scalar,
                lhs.y * scalar,
                lhs.width * scalar,
                lhs.height * scalar);
  }

}

