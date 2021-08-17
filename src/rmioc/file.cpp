#include "file.hpp"
#include <system_error>
#include <cerrno>
#include <utility>
#include <string>
#include <fcntl.h>
#include <unistd.h>

namespace rmioc
{

file_descriptor::file_descriptor(const char* path, int flags)
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-signed-bitwise)
: fd(open(path, flags))
{
    if (this->fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::file_descriptor) Open file " + std::string(path)
        );
    }
}

file_descriptor::file_descriptor(int fd)
: fd(fd)
{}

file_descriptor::file_descriptor(file_descriptor&& other) noexcept
: fd(std::exchange(other.fd, -1))
{}

auto file_descriptor::operator=(file_descriptor&& other) noexcept
-> file_descriptor&
{
    if (this->fd != -1)
    {
        close(this->fd);
    }

    this->fd = std::exchange(other.fd, -1);
    return *this;
}

file_descriptor::operator int() const
{
    return this->fd;
}

file_descriptor::~file_descriptor()
{
    if (this->fd != -1)
    {
        close(this->fd);
        this->fd = -1;
    }
}

} // namespace rmioc
