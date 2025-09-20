#pragma once

#include "DataTypes.h"
#include "NamedVectors.h"

#include <mutex>
#include <shared_mutex>

/*
A point can also have versions, but for now just ignore it.
std::string version?
getVersion()

Should Point directly contain a PointPayloadStore?
Eh, no.
A Point is just a lightweight piece of data (id + vectors).
PointPayloadStore is a heavyweight database component with its own RocksDB handle, cache, etc.
If you put the store inside Point, every point would try to own/open its own RocksDB store, crazy~~
In a DB, you want a single payload store per segment(I will mostly do this) 
[could also be per collection (shared service), but 
you can flush, compact, or drop an entire segment (including payloads) without touching the rest in a collection.], 
so yeah, speration of concerns I will say. 
*/
namespace vectordb {
// A Point holds an ID + NamedVectors
template <std::size_t N>
class Point {
    public:
        explicit Point(PointIdType id) : point_id{id} {}

        bool addVector(const VectorName& name, const DenseVector& vec) {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            return named_vecs.addVector(name, vec);
        }

        std::optional<DenseVector> getVector(const VectorName& name) const {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return named_vecs.getVector(name);
        }

        std::map<VectorName, DenseVector> getAllVectors() const {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            std::map<VectorName, DenseVector> result;
            for (const auto& [name, vec] : named_vecs) {
                result[name] = vec;  //copy vector data
            }
            return result;
        }

    private:
        PointIdType point_id;
        NamedVectors<N> named_vecs;
        mutable std::shared_mutex m_mutex;
};

}
//oh, multi tenancy? group_id? maybe i can put it in the payloadstore class?
/*
example 
vectordb::Point<4> p(42);
p.named_vecs.addVector("text", {0.1f, 0.2f});
p.named_vecs.addVector("image", {0.5f, 0.9f});

if (auto v = p.named_vecs.getVector("text")) {
    for (float f : v->get()) std::cout << f << " ";
}
*/