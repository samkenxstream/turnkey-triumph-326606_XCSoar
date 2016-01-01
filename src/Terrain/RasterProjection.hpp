/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_TERRAIN_RASTER_PROJECTION_HPP
#define XCSOAR_TERRAIN_RASTER_PROJECTION_HPP

#include "Terrain/RasterLocation.hpp"
#include "Geo/GeoPoint.hpp"
#include "Compiler.h"

class GeoBounds;

/**
 * This class manages the projection from GeoPoint to RasterMap
 * coordinates.
 */
class RasterProjection {
  int left, top;
  double x_scale, y_scale;

public:
  /**
   * Initialise the projection.
   *
   * @param bounds the geographic bounds of the map file
   * @param width the sub-pixel ("fine)" width of the map file
   * @param height the sub-pixel ("fine)" height of the map file
   */
  void Set(const GeoBounds &bounds, unsigned width, unsigned height);

  gcc_pure
  Angle WidthToAngle(double pixels) const {
    return Angle::Native(pixels / x_scale);
  }

  gcc_pure
  Angle HeightToAngle(double pixels) const {
    return Angle::Native(pixels / y_scale);
  }

  gcc_pure
  int AngleToWidth(Angle angle) const {
    return (int)(angle.Native() * x_scale);
  }

  gcc_pure
  int AngleToHeight(Angle angle) const {
    return (int)(angle.Native() * y_scale);
  }

  gcc_pure SignedRasterLocation
  ProjectFine(const GeoPoint &location) const {
    const int x = AngleToWidth(location.longitude) - left;
    const int y = top - AngleToHeight(location.latitude);

    return {x, y};
  }

  gcc_pure
  GeoPoint
  UnprojectFine(SignedRasterLocation coords) const {
    const Angle x = WidthToAngle((int)coords.x + left);
    const Angle y = HeightToAngle(top - (int)coords.y);
    return GeoPoint(x, y);
  }

  gcc_pure SignedRasterLocation
  ProjectCoarse(const GeoPoint &location) const {
    return ProjectFine(location) >> 8;
  }

  gcc_pure
  GeoPoint
  UnprojectCoarse(SignedRasterLocation coords) const {
    return UnprojectFine(coords << 8);
  }

  /**
   * Determines the distance (in meters) of two raster pixels.
   *
   * @param pixels the pixel distance between two pixels
   */
  gcc_pure double
  FinePixelDistance(const GeoPoint &location, unsigned pixels) const;

  gcc_pure
  double CoarsePixelDistance(const GeoPoint &location, unsigned pixels) const {
    /* factor 256 because the caller should pass a physical pixel
       number, not interpolated */
    return FinePixelDistance(location, pixels << 8);
  }

  /**
   * Converts a distance (in meters) to a pixel distance.
   *
   * @param pixels the pixel distance between two pixels
   */
  gcc_pure unsigned
  DistancePixelsFine(double distance) const;

  gcc_pure unsigned DistancePixelsCoarse(double distance) const {
    return DistancePixelsFine(distance) >> 8;
  }
};

#endif
