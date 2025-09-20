#pragma once

#include "DataTypes.h"
#include "SegmentHolder.h"

#include <memory>
#include <vector>
#include <utility>
#include <atomic>

/*
Since each collection has a SegmentHolder obj, for multiple collections, we can have 
multiple SegmentHolder objs, and to store these, use SegmentRegister. For this,
I used std::unordered_map. Yeah, i use a lot of maps in this vectordb...
*/
namespace vectordb {

class SegmentRegister {
public:

private:
    std::unordered_map<std::string, std::unique_ptr<SegmentHolder>> registry;

};

}
