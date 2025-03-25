#include "svg.hpp"

namespace svg {

    using namespace std::literals;

    // ---------- Object ------------------

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);

        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;

        if (!points_.empty())
        {
            out << points_.begin()->x << ","sv << points_.begin()->y;
            for (auto it{ std::next(points_.begin()) }; it != points_.end(); it++) {
                out << " "sv << it->x << ","sv << it->y;
            }
        }
        out << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        pos_.emplace(pos);

        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_.emplace(offset);

        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        size_.emplace(size);

        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        if (other_attributes_.has_value()) {
            other_attributes_.emplace(other_attributes_.value() + " font-family=\"" + font_family + "\"");
        }
        else {
            other_attributes_.emplace(" font-family=\"" + font_family + "\"");
        }

        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        if (other_attributes_.has_value())
        {
            other_attributes_.emplace(other_attributes_.value() + " font-weight=\"" + font_weight + "\"");
        }
        else {
            other_attributes_.emplace(" font-weight=\"" + font_weight + "\"");
        }

        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_.reserve(data.size() * 2);
        for (auto it{ data.begin() }; it != data.end(); it++) {
            switch (*it) {
            case '"':
                data_.append("&quot;"sv);
                break;
            case '<':
                data_.append("&lt;"sv);
                break;
            case '>':
                data_.append("&gt;"sv);
                break;
            case '\'':
                data_.append("&apos;"sv);
                break;
            case '&':
                data_.append("&amp;"sv);
                break;
            default:
                data_.append(1, *it);
                break;
            }
        }

        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<text"sv;
        RenderAttrs(out);

        if (pos_.has_value()) {
            out << " x=\"" << pos_.value().x << "\" y=\"" << pos_.value().y << "\"";
        }
        if (offset_.has_value()) {
            out << " dx=\"" << offset_.value().x << "\" dy=\"" << offset_.value().y << "\"";
        }
        if (size_.has_value()) {
            out << " font-size=\"" << size_.value() << "\"";
        }

        if (other_attributes_.has_value()) {
            out << other_attributes_.value();
        }

        out << ">"sv << data_ << "</text>"sv;
    }

    // ---------- Document ------------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        RenderContext ctx{ out, 2, 2 };

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        for (auto it{ objects_.begin() }; it != objects_.end(); it++) {
            it->get()->Render(ctx);
        }

        out << "</svg>"sv;
    }

}  // namespace svg