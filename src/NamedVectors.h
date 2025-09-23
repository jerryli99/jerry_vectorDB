#include "DataTypes.h"
#include "TinyMap.h"

#include <shared_mutex>

/**
 * @brief NamedVectors in the collection are particularly useful when you need to 
 *        represent different aspects or modalities of your data using different 
 *        embedding models. For example, a data point representing a product could 
 *        have separate named vectors for its image, textual description, and other 
 *        features.
 *        Here, we will use a tinymap to store the vector like 
 * 
 *        TinyMap {
 *           "img_vec"  : [1,2,3,4]
 *           "text_vec" : [0.9, 0.4, 0.111, 0.34234]
 *           "audio_vec": [100, 200, 300, 500, 10000, 12312, 23, 12, 31, 23, 123, 1030] 
 *        }
 * 
 *        Well, for all the "img_vec" in this single collection, as long as the dim are
 *        all the same, we are good. For all the "text_vec" in this single collection, if
 *        all the dim are the same, we are good, and so on...
 * 
 *         But do remember in case you got lost, NamedVector(s) are stored in the Point
 *         object, and Point object(s) are stored in the Segment object, and the Segment 
 *         object(s) are stored in the Collection object, and the Collection object(s) are
 *         stored in a HashMap where the CollectionId (string) will be the key in the map.
 * 
 *         note: for prototyping, this vector db only considers dense vector impl for now!! 
 */
/**
 * @brief The magic number here is hardcoded. Max expected entries will depend on how many 
 * models you try to update. But ideally less than 8 (MAX_ENTRIES_TINYMAP)
 */
namespace vectordb {

class NamedVectors {
    public:
        NamedVectors() = default;
        ~NamedVectors() = default;

        bool addVector(const VectorName& name, const DenseVector& vec) {
            return tinymap.insert(name, vec);
        }

        std::optional<DenseVector> getVector(const VectorName& name) const {
            return tinymap.get(name);
        }

        // Expose iteration so Point can copy everything
        auto begin() const { return tinymap.begin(); }
        auto end()   const { return tinymap.end();   }

    private:
        TinyMap<VectorName, DenseVector, TINY_MAP_CAPACITY> tinymap;
};

}