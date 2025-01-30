#include "dispatcher.hpp"

namespace dispatcher {
boost::optional<boost::asio::deadline_timer::traits_type::time_type> now_;
}