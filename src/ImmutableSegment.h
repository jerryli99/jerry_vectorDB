#include "DataTypes.h"
#include "Status.h"

#include <faiss/IndexHNSW.h>

namespace vectordb {
class ImmutableSegment {
    public:
        ImmutableSegment() = default;
        ~ImmutableSegment() = default;

        Status buildHNSWIndex();//??
              
    private:

};

}
