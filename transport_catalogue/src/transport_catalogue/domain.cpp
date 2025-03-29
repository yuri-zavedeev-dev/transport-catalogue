#include "domain.hpp"

namespace transport_catalogue
{
	namespace details
	{
		Stop::Stop(const std::string_view&& name, const geo::Coordinates location)
			: name{ std::move(name) }, location{ std::move(location) }
		{}
		Route::Route(std::string_view&& name, std::forward_list<Stop*>&& stops)
			: name{ std::move(name) }, stops{ std::move(stops) }
		{}
		Route::Route(const std::string_view& name, std::forward_list<Stop*>&& stops)
			: name{ std::move(name) }, stops{ std::move(stops) }
		{}

	}//details
}