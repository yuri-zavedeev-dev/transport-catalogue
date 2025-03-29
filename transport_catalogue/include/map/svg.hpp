#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <deque>
#include <list>
#include <optional>
#include <variant>

namespace svg {

    struct Rgb {
        uint8_t red{ 0 };
        uint8_t green{ 0 };
        uint8_t blue{ 0 };
    };

    struct Rgba {
        uint8_t red{ 0 };
        uint8_t green{ 0 };
        uint8_t blue{ 0 };
        double opacity{ 1. };
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const Color NoneColor{ "none" };

    inline std::ostream& operator<<(std::ostream& out, const Color color) {
        using namespace std::literals;
        if (std::get_if<std::monostate>(&color)) {
            out << "none"sv;
            return out;
        }
        if (const auto* color_ptr = std::get_if<std::string>(&color)) {
            if (color_ptr->empty()) {
                out << "none"sv;
                return out;
            }

            out << *color_ptr;
            return out;
        }
        if (const auto* color_ptr = std::get_if<Rgb>(&color)) {
            out << "rgb("sv << static_cast<int>(color_ptr->red)
                << ","sv << static_cast<int>(color_ptr->green)
                << ","sv << static_cast<int>(color_ptr->blue)
                << ")"sv;
            return out;
        }
        if (const auto* color_ptr = std::get_if<Rgba>(&color)) {
            out << "rgba("sv << static_cast<int>(color_ptr->red)
                << ","sv << static_cast<int>(color_ptr->green)
                << ","sv << static_cast<int>(color_ptr->blue)
                << ","sv << static_cast<double>(color_ptr->opacity)
                << ")"sv;
            return out;
        }
        throw std::logic_error{ "std::ostream& operator<<(std::ostream&, const Color): No such type!" };
    }

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    inline std::ostream& operator<<(std::ostream& out, const StrokeLineCap object) {
        using namespace std::literals;
        switch (object) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        default:
            throw std::logic_error{ "operator<< (std::ostream&, StrokeLineCap): No such type!" };
        }
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const StrokeLineJoin object) {
        using namespace std::literals;
        switch (object) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        default:
            throw std::logic_error{ "operator<< (std::ostream&, StrokeLineJoin): No such type!" };
        }
        return out;
    }

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_line_cap_ = std::move(line_cap);
            return AsOwner();

        }
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_line_join_ = std::move(line_join);
            return AsOwner();
        }


    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_line_cap_) {
                out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
    };

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }


        double x = 0;
        double y = 0;
    };

    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::list<Point> points_;
    };

    class Text final : public Object, public PathProps<Text> {
    public:
        Text& SetPosition(Point pos);

        Text& SetOffset(Point offset);

        Text& SetFontSize(uint32_t size);

        Text& SetFontFamily(std::string font_family);

        Text& SetFontWeight(std::string font_weight);

        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::optional<Point> pos_{ {0., 0.} };
        std::optional<Point> offset_{ {0., 0.} };
        std::optional<uint32_t> size_{ 1 };
        std::optional<std::string> other_attributes_{};

        std::string data_;
    };

    class ObjectContainer {
    public:
        virtual ~ObjectContainer() = default;

        template<typename T>
        void Add(T object) {
            AddPtr(std::move(std::make_unique<T>(std::move(object))));
        }

        virtual void AddPtr(std::unique_ptr<Object>&& object_ptr) = 0;

    protected:
        std::deque<std::unique_ptr<Object>> objects_;
    };

    class Document final : public ObjectContainer {
    public:

        void AddPtr(std::unique_ptr<Object>&& obj) override;

        void Render(std::ostream& out) const;
    };

}  // namespace svg