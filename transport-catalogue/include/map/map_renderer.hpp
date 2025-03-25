#pragma once
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <optional>
#include <set>
#include <string>
#include <sstream>
#include <tuple>
#include <variant>
#include <vector>
#include <unordered_set>

#include "domain.hpp"
#include "geo.hpp"
#include "svg.hpp"

namespace svg_renderer
{
    inline const double EPSILON = 1e-6;
    inline bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    class SphereProjector {
    public:
        SphereProjector() = default;

        // "points_begin" and "points_end" defines the bounds of geo::Coordinates elements interval
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding)
            : padding_(padding) //
        {
            // Nothing to compute, if sphere points not given
            if (points_begin == points_end) {
                return;
            }

            // Find points with min and max longitude
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Find points with min and max latitude
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Find scale coefficient for X axis
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Find scale coefficient for Y axis
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Scale coefficients is not zero,
                // choose minimal of them
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                // Scale coefficient for width axis is not zero, use it
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                // Scale coefficient for height axis is not zero, use it
                zoom_coeff_ = *height_zoom;
            }
        }

        // Projects latitude and longitude to SVG-image coordinates
        svg::Point operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    using Color = std::variant<std::string //text
        , std::tuple<int //rgb
            , int
            , int
        >
        , std::tuple<int //rgba
            , int
            , int
            , double
        >
    >;

    using ColorPalette = std::vector<Color>;

    struct RenderSettings
    {
        double width;
        double height;

        double padding;

        double line_width;
        double stop_radius;

        int bus_label_font_size;
        svg::Point bus_label_offset;

        int stop_label_font_size;
        svg::Point stop_label_offset;

        Color underlayer_color;
        double underlayer_width;

        ColorPalette palette;
    };

    struct StopData {
        std::string_view name;
        geo::Coordinates location;
    };

    struct StopDataCmp {
        bool operator()(const StopData& lhs, const StopData& rhs) const {
            return std::lexicographical_compare(lhs.name.begin(), lhs.name.end()
                                                , rhs.name.begin(), rhs.name.end()
            );
        }
    };

    struct RouteData {
        std::string name;
        std::vector<StopData> stops;
        bool is_round_trip;
    };

    struct RouteDataCmp {
        bool operator()(const RouteData& lhs, const RouteData& rhs) const {
            return std::lexicographical_compare(lhs.name.begin(), lhs.name.end()
                , rhs.name.begin(), rhs.name.end()
            );
        }
    };

    class Renderer {
    public:
        Renderer() = default;

        void Render(std::ostream& output_stream = std::cout);

        void SetSettings(RenderSettings&& settings);
        void AddRoute(RouteData&& route_data);

    private:
        void RenderRoutes();

        void DrawRoutePolylines();

        void DrawRouteTextWithBackground();

        void DrawStopsPoints();

        void DrawStopTextWithBackground();

        std::string ConvertColorToString(const Color& color) const;

        const Color& GetNextColor();

        std::unordered_set<geo::Coordinates, geo::CoordinatesHasher> unique_coordinates_;
        std::set<StopData, StopDataCmp> stops_data_;
        std::set<RouteData, RouteDataCmp> routes_data_;
        svg::Document document_;
        RenderSettings settings_;
        SphereProjector projector_;
        ColorPalette::iterator color_it_;
    };
}//svg_renderer