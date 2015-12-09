#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceVisitor.hpp"
#include "Geo/GeoVector.hpp"
#include "Util/StringAPI.hxx"

#include <algorithm>

void
AirspaceSelectInfo::ResetVector()
{
  vec.SetInvalid();
}

const GeoVector &
AirspaceSelectInfo::GetVector(const GeoPoint &location,
                              const FlatProjection &projection) const
{
  if (!vec.IsValid()) {
    const auto closest_loc = airspace->ClosestPoint(location, projection);
    vec = GeoVector(location, closest_loc);
  }

  return vec;
}

bool
AirspaceFilterData::Match(const GeoPoint &location,
                          const FlatProjection &projection,
                          const AbstractAirspace &as) const
{
  if (cls != AirspaceClass::AIRSPACECLASSCOUNT && as.GetType() != cls)
    return false;

  if (name_prefix != nullptr && !as.MatchNamePrefix(name_prefix))
    return false;

  if (!direction.IsNegative()) {
    const auto closest = as.ClosestPoint(location, projection);
    const auto bearing = location.Bearing(closest);
    auto direction_error = (bearing - direction).AsDelta().AbsoluteDegrees();
    if (direction_error > fixed(18))
      return false;
  }

  if (!negative(distance)) {
    const auto closest = as.ClosestPoint(location, projection);
    const auto distance = location.Distance(closest);
    if (distance > distance)
      return false;
  }

  return true;
}

class AirspaceFilterPredicate final : public AirspacePredicate {
  const GeoPoint location;
  const FlatProjection &projection;
  const AirspaceFilterData &filter;

public:
  AirspaceFilterPredicate(const GeoPoint &_location,
                          const FlatProjection &_projection,
                          const AirspaceFilterData &_filter)
    :location(_location), projection(_projection), filter(_filter) {}

  bool operator()(const AbstractAirspace &as) const override {
    return filter.Match(location, projection, as);
  }
};

class AirspaceFilterVisitor final : public AirspaceVisitor {
public:
  AirspaceSelectInfoVector result;

  void Visit(const AbstractAirspace &as) override {
    result.emplace_back(as);
  }
};

static void
SortByDistance(AirspaceSelectInfoVector &vec, const GeoPoint &location,
               const FlatProjection &projection)
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return elem1.GetVector(location, projection).distance <
           elem2.GetVector(location, projection).distance;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

static void
SortByName(AirspaceSelectInfoVector &vec)
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return StringCollate(elem1.GetAirspace().GetName(),
                         elem2.GetAirspace().GetName()) < 0;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

AirspaceSelectInfoVector
FilterAirspaces(const Airspaces &airspaces, const GeoPoint &location,
                const AirspaceFilterData &filter)
{
  const AirspaceFilterPredicate predicate(location, airspaces.GetProjection(),
                                          filter);
  AirspaceFilterVisitor visitor;

  if (!negative(filter.distance))
    airspaces.VisitWithinRange(location, filter.distance, visitor, predicate);
  else
    for (const auto &i : airspaces)
      if (predicate(i.GetAirspace()))
        visitor.Visit(i.GetAirspace());

  if (filter.direction.IsNegative() && negative(filter.distance))
    SortByName(visitor.result);
  else
    SortByDistance(visitor.result, location, airspaces.GetProjection());

  return visitor.result;
}
