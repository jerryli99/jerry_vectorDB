/*
bitmap indexing for mark delete and pre-filtering, and 
multi-tenancy (store multiple users’ data in the same collection and 
filter queries by user_id (or tenant_id).)

Multi-tenancy = multiple users (tenants) share the same collection and storage.
Each point has a tenant/user ID in its payload.
Pre-filtering uses an inverted index (or bitmap) to quickly select only the points belonging to that user.
Then the filtered internal IDs are passed to HNSW (or brute-force) for vector search.
still thinking: authentication/authorization ensures users can only query their own data.

ID:       0 1 2 3 4 5 6 7 8 9
bitmap:   0 1 0 1 0 0 1 0 0 1

Now if query also requires category="image", that bitmap might be:

ID:       0 1 2 3 4 5 6 7 8 9
bitmap:   0 1 1 0 0 0 1 0 0 0

Intersection = bitwise AND:
ID:       0 1 2 3 4 5 6 7 8 9
result    0 1 0 0 0 0 1 0 0 0 = candidate IDs {1, 6}
*/

namespace vectordb {
class BitmapIndex {

    
};
}