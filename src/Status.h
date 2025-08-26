// Status.h
#pragma once
#include <string>

namespace vectordb {
    struct Status {
        bool ok;
        std::string message;

        static Status OK() { return {true, "ok"}; }
        static Status Error(const std::string& msg) { return {false, msg}; }
    };
}
