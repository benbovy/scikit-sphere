#ifndef SPHERELY_GEOGRAPHY_H_
#define SPHERELY_GEOGRAPHY_H_

#include <memory>

#include "s2/s2lax_polyline_shape.h"
#include "s2/s2loop.h"
#include "s2/s2point.h"
#include "s2/s2polyline.h"
#include "s2geography.h"
#include "s2geography_addons.hpp"

namespace s2geog = s2geography;

namespace spherely {

using S2GeographyPtr = std::unique_ptr<s2geog::Geography>;
using S2GeographyIndexPtr = std::unique_ptr<s2geog::ShapeIndexGeography>;

/*
** The registered Geography types
*/
enum class GeographyType : std::int8_t {
    None = -1,
    Point,
    LineString,
    LinearRing,
    Polygon,
    MultiPoint,
    MultiLineString,
    GeographyCollection
};

/*
** Thin wrapper around s2geography::Geography.
**
** This wrapper implements the following specific features (that might
** eventually move into s2geography::Geography?):
**
** - Implement move semantics only.
** - Add a virtual ``clone()`` method for explicit copies (similarly to s2geometry).
** - Add a virtual ``geog_type()`` method for getting the geography type.
** - Encapsulate a lazy ``s2geography::ShapeIndexGeography`` accessible via ``geog_index()``.
**
*/
class Geography {
public:
    using S2GeographyType = s2geog::Geography;

    Geography(const Geography&) = delete;
    Geography(Geography&& geog) : m_s2geog_ptr(std::move(geog.m_s2geog_ptr)) {}
    Geography(S2GeographyPtr&& s2geog_ptr) : m_s2geog_ptr(std::move(s2geog_ptr)) {}

    virtual ~Geography() {}

    Geography& operator=(const Geography&) = delete;
    Geography& operator=(Geography&& other) {
        m_s2geog_ptr = std::move(other.m_s2geog_ptr);
        return *this;
    }

    inline virtual GeographyType geog_type() const {
        return GeographyType::None;
    }

    inline const s2geog::Geography& geog() const {
        return *m_s2geog_ptr;
    }

    inline const s2geog::ShapeIndexGeography& geog_index() {
        if (!m_s2geog_index_ptr) {
            m_s2geog_index_ptr = std::make_unique<s2geog::ShapeIndexGeography>(geog());
        }

        return *m_s2geog_index_ptr;
    }
    void reset_index() {
        m_s2geog_index_ptr.reset();
    }
    bool has_index() {
        return m_s2geog_index_ptr != nullptr;
    }

    int dimension() const {
        return m_s2geog_ptr->dimension();
    }
    int num_shapes() const {
        return m_s2geog_ptr->num_shapes();
    }

private:
    S2GeographyPtr m_s2geog_ptr;
    S2GeographyIndexPtr m_s2geog_index_ptr;
};

class Point : public Geography {
public:
    using S2GeographyType = s2geog::PointGeography;

    Point(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::Point;
    }

    inline const S2Point& s2point() const {
        const auto& points = static_cast<const s2geog::PointGeography&>(geog()).Points();
        // TODO: does not work for empty point geography
        return points[0];
    }
};

class MultiPoint : public Geography {
public:
    using S2GeographyType = s2geog::PointGeography;

    MultiPoint(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::MultiPoint;
    }

    inline const std::vector<S2Point>& s2points() const {
        return static_cast<const s2geog::PointGeography&>(geog()).Points();
    }
};

class LineString : public Geography {
public:
    using S2GeographyType = s2geog::PolylineGeography;

    LineString(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::LineString;
    }

    inline const S2Polyline& s2polyline() const {
        const auto& polylines = static_cast<const S2GeographyType&>(geog()).Polylines();
        // TODO: does not work for empty point geography
        return *polylines[0];
    }
};

class MultiLineString : public Geography {
public:
    using S2GeographyType = s2geog::PolylineGeography;

    MultiLineString(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::MultiLineString;
    }

    inline const std::vector<std::unique_ptr<S2Polyline>>& s2polylines() const {
        return static_cast<const S2GeographyType&>(geog()).Polylines();
    }
};

class LinearRing : public Geography {
public:
    using S2GeographyType = s2geog::ClosedPolylineGeography;

    LinearRing(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::LinearRing;
    }

    inline const S2LaxClosedPolylineShape& s2polyline() const {
        return static_cast<const S2GeographyType&>(geog()).Polyline();
    }

    LinearRing* clone() const {
        const auto& s2geog = static_cast<const S2GeographyType&>(geog());
        return new LinearRing(std::unique_ptr<S2GeographyType>(s2geog.clone()));
    }
};

class Polygon : public Geography {
public:
    using S2GeographyType = s2geog::PolygonGeography;

    Polygon(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::Polygon;
    }

    inline const S2Polygon& polygon() const {
        return *static_cast<const S2GeographyType&>(geog()).Polygon();
    }
};

class GeographyCollection : public Geography {
public:
    using S2GeographyType = s2geog::GeographyCollection;

    GeographyCollection(S2GeographyPtr&& geog_ptr) : Geography(std::move(geog_ptr)) {};

    inline GeographyType geog_type() const override {
        return GeographyType::GeographyCollection;
    }

    const std::vector<std::unique_ptr<s2geog::Geography>>& features() const {
        return static_cast<const S2GeographyType&>(geog()).Features();
    }
};

/*
** Helper to create Geography object wrappers.
**
** @tparam T1 The S2Geography wrapper type
** @tparam T2 This library wrapper type.
** @tparam S The S2Geometry type
*/
template <class T1, class T2, class S>
std::unique_ptr<T2> make_geography(S&& s2_obj) {
    S2GeographyPtr s2geog_ptr = std::make_unique<T1>(std::forward<S>(s2_obj));
    return std::make_unique<T2>(std::move(s2geog_ptr));
}

// Helpers for explicit copy of s2geography objects.
std::unique_ptr<s2geog::Geography> clone_s2geography(const s2geog::Geography& geog);

}  // namespace spherely

#endif  // SPHERELY_GEOGRAPHY_H_
