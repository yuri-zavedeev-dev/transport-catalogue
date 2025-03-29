#pragma once
#include <string>

namespace geo {
    static const int EARTH_RADIUS = 6371000;

    struct Coordinates {
        double lat; // latitude
        double lng; // longitude

        bool operator==(const Coordinates& other) const;
        bool operator!=(const Coordinates& other) const;
    };

    double ComputeDistance(Coordinates from, Coordinates to);

    
    struct CoordinatesHasher {
    auto operator() (geo::Coordinates coords) const -> size_t {
        return std::hash<double>{}(coords.lat) * 47
            + std::hash<double>{}(coords.lat) * 47 * 47;
    }
    };
}  // namespace geo