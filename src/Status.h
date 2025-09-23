// Status.h
#pragma once
#include <string>

/*
The overall code is not good at all, need to change it later, I am just lazy now sigh...
The Status here needs to have http code response 

something in the DB.cpp like this

// Helper to convert Status to HTTP response
void statusToHttpResponse(const Status& status, httplib::Response& res) {
    if (status.ok()) {
        res.set_content(R"({"status":"ok"})", "application/json");
    } else {
        // Map Status codes to HTTP status codes
        int http_status;
        switch (status.code()) {
            case Status::NOT_FOUND:
                http_status = 404;
                break;
            case Status::ALREADY_EXISTS:
                http_status = 409;
                break;
            case Status::INVALID_ARGUMENT:
                http_status = 400;
                break;
            case Status::IO_ERROR:
                http_status = 503;
                break;
            default:
                http_status = 500;
        }
        
        nlohmann::json error_json = {
            {"status", "error"},
            {"code", status.code()},
            {"message", status.message()}
        };
        res.status = http_status;
        res.set_content(error_json.dump(), "application/json");
    }
}
*/
namespace vectordb {
struct Status {
    bool ok;
    std::string message;

    static Status OK() { 
        return {true, "ok"}; 
    }

    static Status Error(const std::string& msg) { 
        return {false, msg}; 
    }
};


//This StatusOr pattern is kind of common in Google’s Abseil
// Either you get a valid value of type T,
// Or you get an error status describing what went wrong.
//i find it interesting to use it here.
template <typename T>
class StatusOr {
public:
    StatusOr(const Status& status) : m_status(status), m_has_value(false) {}
    StatusOr(const T& value) : m_status(Status::OK()), m_value(value), m_has_value(true) {}
    StatusOr(T&& value) : m_status(Status::OK()), m_value(std::move(value)), m_has_value(true) {}

    bool ok() const { 
        return m_status.ok; 
    }
    
    const Status& status() const { 
        return m_status; 
    }
    
    T& value() { 
        return m_value; 
    }

    const T& value() const { 
        return m_value; 
    }

private:
    Status m_status;
    T m_value{};
    bool m_has_value;
};

}

/*
planning to use this instead:
class Status {
public:
    // Error codes for programmatic error handling
    enum Code {
        OK = 0,
        NOT_FOUND = 1,
        ALREADY_EXISTS = 2,
        INVALID_ARGUMENT = 3,
        INTERNAL_ERROR = 4,
        IO_ERROR = 5,
        // Add more as needed
    };
    
    // Default constructor creates OK status
    Status() : code_(OK), message_("OK") {}
    
    // Constructor with code and message
    Status(Code code, std::string message) 
        : code_(code), message_(std::move(message)) {}
    
    // Factory methods
    static Status OK() { return Status(); }
    static Status Error(Code code, const std::string& message) {
        return Status(code, message);
    }
    static Status NotFound(const std::string& what) {
        return Status(NOT_FOUND, what + " not found");
    }
    static Status AlreadyExists(const std::string& what) {
        return Status(ALREADY_EXISTS, what + " already exists");
    }
    static Status InvalidArgument(const std::string& what) {
        return Status(INVALID_ARGUMENT, "Invalid argument: " + what);
    }
    
    // Check methods
    bool ok() const { return code_ == OK; }
    bool isNotFound() const { return code_ == NOT_FOUND; }
    bool isAlreadyExists() const { return code_ == ALREADY_EXISTS; }
    
    // Getters
    Code code() const { return code_; }
    const std::string& message() const { return message_; }
    const char* what() const { return message_.c_str(); }
    
    // For debugging
    std::string toString() const {
        if (ok()) return "OK";
        return "Error[" + std::to_string(code_) + "]: " + message_;
    }
    
    // Comparison
    bool operator==(const Status& other) const {
        return code_ == other.code_ && message_ == other.message_;
    }
    bool operator!=(const Status& other) const {
        return !(*this == other);
    }

private:
    Code code_;
    std::string message_;
};

// For easy streaming
inline std::ostream& operator<<(std::ostream& os, const Status& status) {
    return os << status.toString();
}

// Improved StatusOr
template <typename T>
class StatusOr {
public:
    // Constructor from Status (error)
    StatusOr(Status status) : status_(std::move(status)) {}
    
    // Constructor from value (success)
    StatusOr(T value) : status_(Status::OK()), value_(std::move(value)) {}
    
    // Copy/move constructors
    StatusOr(const StatusOr&) = default;
    StatusOr(StatusOr&&) = default;
    
    // Check methods
    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }
    
    // Value access (with checking)
    T& value() & {
        if (!ok()) throw std::runtime_error("Attempt to access value of errored StatusOr");
        return value_;
    }
    
    const T& value() const & {
        if (!ok()) throw std::runtime_error("Attempt to access value of errored StatusOr");
        return value_;
    }
    
    T&& value() && {
        if (!ok()) throw std::runtime_error("Attempt to access value of errored StatusOr");
        return std::move(value_);
    }
    
    // Safe value access with default
    T value_or(T default_value) const {
        return ok() ? value_ : std::move(default_value);
    }

private:
    Status status_;
    T value_{};
};


*/