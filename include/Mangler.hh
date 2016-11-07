#pragma once

#include <unordered_map>

class Mangler {
public:
    inline std::string mangle(std::string const &) const
    { return ""; }
    inline void addLink(std::string const &base, std::string const &child) {
        _links[child] = base;
    }

protected:
    std::unordered_map<std::string, std::string> _links;
};
