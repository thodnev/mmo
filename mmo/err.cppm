module;
#include <exception>
#include <format>
#include <string>

export module err;
export namespace err {
// Contains project exception classes hierarchy

/// Base class and the most generic project exception
class Error : public std::exception {
protected:
    std::string message;

public:
    template <typename ...Args>
    explicit Error(std::format_string<Args...> fmt, Args && ...args)
        : message(std::format(fmt, std::forward<Args>(args)...)) {}

    virtual ~Error() = default;
    
    virtual const char *what() const noexcept override
    {
        return message.c_str();
    }
};

// Common project exceptions

/// Argument or parameter has wrong value
class ValueError : public Error {
public:
    using Error::Error;     // Inherit constructors
};

/// Something was expected to be found, but wasn't found
class LookupError : public Error {
public:
    using Error::Error;
};

/// Tried to access element out of valid bounds
class BoundsError : public LookupError {
public:
    using LookupError::LookupError;
};

/// Access to some resource failed
class ResourceError : public Error {
public:
    using Error::Error;
};

/// File (or generally filesystem) access error
class FSError : public ResourceError {
public:
    using ResourceError::ResourceError;
};

/// Errors not falling into predefined categories
/// and usually related to foreign software parts of the project
class RuntimeError : public Error {
public:
    using Error::Error;
};


// Exceptions for different project modules


}   // namespace