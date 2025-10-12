## Interal Index management in ImmutableSeg
```
Example 1:

Points in ActiveSegment:
Point "id_1": {img: [0.1,0.2], txt: [0.3,0.4]}
Point "id_2": {img: [0.5,0.6], txt: [0.7,0.8]}  
Point "id_3": {img: [0.9,1.0], txt: [1.1,1.2]}

ImmutableSegment IdTracker:
Vector Space "img":
  PointID -> Offset: {"id_1"->0, "id_2"->1, "id_3"->2}
  Offset -> PointID: ["id_1", "id_2", "id_3"]
  Bitmap: [1, 1, 1]

Vector Space "txt":
  PointID -> Offset: {"id_1"->0, "id_2"->1, "id_3"->2}
  Offset -> PointID: ["id_1", "id_2", "id_3"] 
  Bitmap: [1, 1, 1]

FAISS Indexes:
- "img": 3 vectors, internal IDs map to offsets [0,1,2]
- "txt": 3 vectors, internal IDs map to offsets [0,1,2]
```

Handle cases where the user might not insert vector data covering all the named vectors per Point struct. Here, we ignore the missing named vector values.
```
Example 2:

Points in ActiveSegment:
Point "user_alice": {img: [0.1,0.2], txt: [0.3,0.4]}
Point "user_bob":   {img: [0.5,0.6]}                    // Missing txt
Point "user_charlie": {txt: [0.7,0.8]}                  // Missing img  
Point "doc_xyz": {img: [0.9,1.0], txt: [1.1,1.2]}

ImmutableSegment IdTracker:
Vector Space "img":
  PointID -> Offset: {"user_alice"->0, "user_bob"->1, "doc_xyz"->2}
  Offset -> PointID: ["user_alice", "user_bob", "doc_xyz"]
  Bitmap: [1, 1, 1]

Vector Space "txt":
  PointID -> Offset: {"user_alice"->0, "user_charlie"->1, "doc_xyz"->2}
  Offset -> PointID: ["user_alice", "user_charlie", "doc_xyz"]
  Bitmap: [1, 1, 1]

```

## IdTracker -- the Table struct
```
For example:

// After inserting points with string IDs:
Table for "img" vector space entry:

point_id_to_offset (std::map<std::string, size_t>):
{
    "user_alice": 0,
    "user_bob":   1, 
    "doc_xyz":    2
}

offset_to_pointid (std::vector<std::optional<std::string>>):
[0] = "user_alice"
[1] = "user_bob"  
[2] = "doc_xyz"
[3] = std::nullopt  // (empty slot)
[4] = std::nullopt  // (empty slot)

bitmap: [1, 1, 1, 0, 0]  // 1=occupied, 0=free
next_free_offset: 3

...

Table for "txt" vector space entry ... you know the idea
```