#include "map_renderer.hpp"

namespace svg_renderer
{
    void Renderer::Render(std::ostream& output_stream) {
        RenderRoutes();
        document_.Render(output_stream);
        output_stream << std::endl;
    }

    void Renderer::SetSettings(RenderSettings&& settings) {
        settings_ = std::move(settings);
        color_it_ = settings_.palette.end();
        projector_ = SphereProjector(unique_coordinates_.begin()
                                    , unique_coordinates_.end()
                                    , settings_.width
                                    , settings_.height
                                    , settings_.padding);
    }

    void Renderer::AddRoute(RouteData&& route_data) {
        for (const StopData& stop_data : route_data.stops) {
            unique_coordinates_.insert(stop_data.location);
            stops_data_.insert(stop_data);
        }

        routes_data_.insert(std::move(route_data));
    }

    void Renderer::RenderRoutes() {      
        DrawRoutePolylines();

        DrawRouteTextWithBackground();

        DrawStopsPoints();

        DrawStopTextWithBackground();
    }
    void Renderer::DrawRoutePolylines()
    {
        for (const RouteData& route_data : routes_data_) {
            if (route_data.stops.empty()) {
                continue;
            }
            svg::Polyline route_polyline{};
            for (const StopData& stop : route_data.stops) {
                route_polyline.AddPoint(projector_(stop.location));
            }

            route_polyline.SetStrokeColor(ConvertColorToString(GetNextColor()))
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            document_.Add(route_polyline);
        }
    }
    void Renderer::DrawRouteTextWithBackground()
    {
        color_it_ = settings_.palette.end();
        for (const RouteData& route_data : routes_data_) {
            auto first_stop_it = route_data.stops.begin();
            auto last_stop_it = route_data.stops.end();
            if (!route_data.is_round_trip && route_data.stops.size() > 1) {
                last_stop_it = std::next(route_data.stops.begin(), route_data.stops.size() / 2);
                if (first_stop_it->name == last_stop_it->name) {
                    last_stop_it = route_data.stops.end();
                }
            }
            svg::Text route_name_text{};
            route_name_text.SetData(route_data.name)
                .SetPosition(projector_(first_stop_it->location))
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold");

            svg::Text route_name_background{ route_name_text };
            route_name_background.SetFillColor(ConvertColorToString(settings_.underlayer_color))
                .SetStrokeColor(ConvertColorToString(settings_.underlayer_color))
                .SetStrokeWidth(settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            route_name_text.SetFillColor(ConvertColorToString(GetNextColor()));

            document_.Add(route_name_background);
            document_.Add(route_name_text);

            if (last_stop_it != route_data.stops.end()) {
                svg::Text route_name_text_copy{ route_name_text };
                svg::Text route_name_background_copy{ route_name_background };

                route_name_text_copy.SetPosition(projector_(last_stop_it->location));
                route_name_background_copy.SetPosition(projector_(last_stop_it->location));

                document_.Add(route_name_background_copy);
                document_.Add(route_name_text_copy);
            }
        }
    }
    void Renderer::DrawStopsPoints()
    {
        for (const StopData& stop_data : stops_data_) {
            svg::Circle stop_circle{};
            stop_circle.SetCenter(projector_(stop_data.location))
                .SetRadius(settings_.stop_radius)
                .SetFillColor("white");
            document_.Add(stop_circle);
        }
    }
    void Renderer::DrawStopTextWithBackground()
    {
        for (const StopData& stop_data : stops_data_) {
            svg::Text stop_text;
            stop_text.SetPosition(projector_(stop_data.location))
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(static_cast<std::string>(stop_data.name));

            svg::Text stop_background{ stop_text };
            stop_background.SetFillColor(ConvertColorToString(settings_.underlayer_color))
                .SetStrokeColor(ConvertColorToString(settings_.underlayer_color))
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            stop_text.SetFillColor("black");

            document_.Add(stop_background);
            document_.Add(stop_text);
        }
    }
    const Color& Renderer::GetNextColor() {
        if (color_it_ != settings_.palette.end()) {
            color_it_++;
        }
        if (color_it_ == settings_.palette.end()) {
            color_it_ = settings_.palette.begin();
        }
        return *color_it_;
    }
    std::string Renderer::ConvertColorToString(const Color& color) const {
        if (auto it = std::get_if<std::string>(&color)) {
            return *it;
        }
        if (auto it = std::get_if<std::tuple<int, int, int>>(&color)) {
            return std::string{ "rgb(" + std::to_string(std::get<0>(*it))
                                + "," + std::to_string(std::get<1>(*it))
                                + "," + std::to_string(std::get<2>(*it))
                                + ")"
            };
        }
        if (auto it = std::get_if<std::tuple<int, int, int, double>>(&color)) {
            std::stringstream ss;
            ss << std::get<3>(*it);
            return std::string{ "rgba(" + std::to_string(std::get<0>(*it))
                                + "," + std::to_string(std::get<1>(*it))
                                + "," + std::to_string(std::get<2>(*it))
                                + "," + ss.str()
                                + ")"
            };
        }
        throw std::logic_error{"std::string Renderer::ConvertColorToString(const Color&) const: Unknown color type!"};
    }
}//svg_renderer