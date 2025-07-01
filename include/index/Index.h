#pragma once

#include <Eigen/Dense>
#include <vector>

namespace vectordb {

class Index {
public:
    virtual ~Index() = default;
    virtual int dimension() const = 0;
};

};
