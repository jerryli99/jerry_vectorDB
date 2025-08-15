#pragma once

#include "DataTypes.h"

/*
Let user do a http request to do snapshot of a collection or just all collection...right now just collection
in brief
*/

namespace vectordb {
    class SnapShot {
        //iterate all segments in the memory, do faiss::write_index() for each.
        public:
            SnapShot() = default; 
            SnapShot(const std::filesystem::path path) : m_snapshot_path {path} {};
            ~SnapShot() = default;

            //still thinking about these right now, might change all of them or not.
        
            void create();//Creates a new snapshot (writes current in-memory state to disk).
            void restore();//Loads a snapshot back into memory.
            void list_snapshots();//returns available snapshots (e.g., by timestamp or ID).
            void remove();//Deletes a specific snapshot.
            void exists();//Checks if a snapshot exists.
            void get_latest();//Returns the most recent snapshot.
            void get_by_timestamp();//Finds a snapshot by creation time.

        private:
            std::filesystem::path m_snapshot_path; //maybe set a default path

    };
}