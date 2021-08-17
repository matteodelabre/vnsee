#ifndef RMIOC_FILE_HPP
#define RMIOC_FILE_HPP

namespace rmioc
{

/** Wrapper around C file descriptors. */
class file_descriptor
{
public:
    /**
     * Open a file.
     *
     * @param path Path to the file to open.
     * @param flags Opening flags.
     * @throws std::system_error If opening fails.
     */
    file_descriptor(const char* path, int flags);

    /** Take ownership of an existing file descriptor. */
    file_descriptor(int fd);

    // Disallow copying input device handles
    file_descriptor(const file_descriptor& other) = delete;
    file_descriptor& operator=(const file_descriptor& other) = delete;

    // Transfer handle ownership
    file_descriptor(file_descriptor&& other) noexcept;
    file_descriptor& operator=(file_descriptor&& other) noexcept;

    /** Get the underlying file descriptor. */
    operator int() const;

    /** Close the file. */
    ~file_descriptor();

private:
    int fd;
}; // class file_descriptor

} // namespace rmioc

#endif // RMIOC_FILE_HPP
