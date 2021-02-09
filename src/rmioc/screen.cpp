#include "screen.hpp"

namespace rmioc
{

auto component_format::max() const -> std::uint32_t
{
    return (1U << this->length) - 1;
}

} // namespace rmioc
