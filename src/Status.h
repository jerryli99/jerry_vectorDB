// Status.h
#pragma once
#include <string>

/*
The overall code is not good at all, need to change it later, I am just lazy now sigh...
The Status here needs to have http code response 
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

    bool ok() const { return m_status.ok; }
    const Status& status() const { return m_status; }
    
    T& value() { 
        if (!m_has_value) {
            throw std::runtime_error("StatusOr does not contain a value: " + m_status.message);
        }
        return m_value;
    }
    
    const T& value() const { 
        if (!m_has_value) {
            throw std::runtime_error("StatusOr does not contain a value: " + m_status.message);
        }
        return m_value;
    }

private:
    Status m_status;
    T m_value{};
    bool m_has_value;
};

} // namespace vectordb
