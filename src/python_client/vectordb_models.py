from dataclasses import dataclass
from typing import Dict, List, Optional, Union, Literal
from collections import OrderedDict

"""
OK, so for now I only supoprt point ids of string type, no integer type.
I can always add it later.
"""

@dataclass
class VectorParams:
    size: int
    distance: Literal["Cosine", "L2", "Dot"]

    def to_dict(self):
        return {
            "size": self.size,
            "distance": self.distance,
        }

#----------------

@dataclass
class CreateCollectionRequest:
    vectors: Union[VectorParams, Dict[str, VectorParams]]
    on_disk: Literal["true", "false"] # Type hint for valid values

    def __post_init__(self):
        # Validation still good for runtime safety
        if self.on_disk not in ["true", "false"]:
            raise ValueError(f'on_disk must be "true" or "false", got "{self.on_disk}"')
        
    def to_dict(self):
        result = {}
        
        if isinstance(self.vectors, dict):
            result["vectors"] = {k: v.to_dict() for k, v in self.vectors.items()}
        else:
            result["vectors"] = self.vectors.to_dict()
        
        result["on_disk"] = self.on_disk
        return result

#----------------
    
@dataclass
class PointStruct:
    id: str 
    vector: Union[List[float], Dict[str, List[float]]]
    payload: Optional[Dict] = None 

    def to_dict(self):
        d = {"id": self.id, "vector": self.vector}
        if self.payload is not None:
            d["payload"] = self.payload
        return d

#-------------------

@dataclass
class UpsertBatch:
    ids: List[str]
    vectors: List[List[float]]
    payloads: Optional[List[Dict]] = None

    def to_dict(self):
        d = {"ids": self.ids, "vectors": self.vectors}
        if self.payloads is not None:
            d["payloads"] = self.payloads
        return d
    
#--------------------

@dataclass
class QueryRequest:
    collection_name: str
    query_vectors: Optional[List[List[float]]] = None
    query_pointids: Optional[List[str]] = None
    using: str = "default"
    top_k: Optional[int] = 10

    def __post_init__(self):
        # Validate collection name
        if not isinstance(self.collection_name, str) or not self.collection_name.strip():
            raise ValueError("`collection_name` must be a non-empty string.")

        # Determine which field is active
        has_vectors = self.query_vectors is not None and len(self.query_vectors) > 0
        has_ids = self.query_pointids is not None and len(self.query_pointids) > 0

        if has_vectors and has_ids:
            raise ValueError("Provide only one of 'query_vectors' or 'query_pointids', not both.")
        if not has_vectors and not has_ids:
            raise ValueError("Must provide at least one non-empty 'query_vectors' or 'query_pointids'.")

        # Validate query vectors
        if has_vectors:
            for i, vec in enumerate(self.query_vectors):
                if not isinstance(vec, list) or not all(isinstance(x, (int, float)) for x in vec):
                    raise TypeError(f"query_vectors[{i}] must be a list of numbers, got {vec}")
                if len(vec) == 0:
                    raise ValueError(f"query_vectors[{i}] cannot be empty")

        # Validate query point IDs
        if has_ids:
            for pid in self.query_pointids:
                if not isinstance(pid, str) or not pid.strip():
                    raise ValueError(f"Invalid point ID: {pid!r}")

        # Validate `using`
        if not isinstance(self.using, str) or not self.using.strip():
            raise ValueError("`using` must be a non-empty string.")
        if len(self.using) > 64:
            raise ValueError("`using` name too long (max 64 characters).")

        # Only check top_k if query is by vectors
        if has_vectors:
            if self.top_k is None:
                raise ValueError("`top_k` is required when using 'query_vectors'.")
            if not isinstance(self.top_k, int):
                raise TypeError("`top_k` must be an integer.")
            if self.top_k <= 0:
                raise ValueError("`top_k` must be positive.")
            if self.top_k > 100:
                raise ValueError("`top_k` is too large (must be <= 100).")
        else:
            # For ID-based queries, ignore top_k entirely
            self.top_k = None

    def to_dict(self):
        data = OrderedDict()
        data["collection_name"] = self.collection_name

        if self.query_vectors is not None:
            data["query_vectors"] = self.query_vectors
        elif self.query_pointids is not None:
            data["query_pointids"] = self.query_pointids

        data["using"] = self.using

        # Only include top_k if present
        if self.top_k is not None:
            data["top_k"] = self.top_k

        return data

#-------------------

@dataclass
class ScoredPoint:
    id: str
    score: float

#-------------------
@dataclass
class QueryResponse:
    result: Union[List[ScoredPoint], List[List[ScoredPoint]]]
    status: str
    time: float

    @classmethod
    def from_dict(cls, data: dict) -> "QueryResponse":
        raw_result = data.get("result", [])

        if not raw_result:
            parsed_result = []
        # batch query
        elif isinstance(raw_result[0], dict) and "hits" in raw_result[0]:
            parsed_result = [
                [ScoredPoint(id=p["id"], score=p["score"]) for p in group["hits"]]
                for group in raw_result
            ]
        else:
            # fallback: treat as single flat list
            parsed_result = [ScoredPoint(id=p["id"], score=p["score"]) for p in raw_result]

        return cls(
            result=parsed_result,
            status=data.get("status", ""),
            time=data.get("time", 0.0)
        )

#--------------------------------------
# Add these to your existing vectordb_models.py

@dataclass
class GraphRelationship:
    from_id: str
    to_id: str
    relationship: str
    weight: float = 1.0
    
    def to_dict(self) -> dict:
        return {
            "from_id": self.from_id,
            "to_id": self.to_id,
            "relationship": self.relationship,
            "weight": self.weight
        }

@dataclass
class GraphEdge:
    from_id: str
    to_id: str
    relationship: str
    weight: float
    
    @classmethod
    def from_dict(cls, data: dict) -> 'GraphEdge':
        return cls(
            from_id=data["from_id"],
            to_id=data["to_id"],
            relationship=data["relationship"],
            weight=data["weight"]
        )

@dataclass
class GraphTraversalRequest:
    start_id: str
    direction: Literal["outwards", "inwards", "both"] = "outwards"  # "outwards", "inwards", "both"
    max_hops: int = 2
    min_weight: float = 0.0
    relationship_filter: Optional[str] = None
    
    def to_dict(self) -> dict:
        data = {
            "start_id": self.start_id,
            "direction": self.direction,
            "max_hops": self.max_hops,
            "min_weight": self.min_weight
        }
        if self.relationship_filter:
            data["relationship_filter"] = self.relationship_filter
        return data

@dataclass
class ShortestPathRequest:
    start_id: str
    end_id: str
    
    def to_dict(self) -> dict:
        return {
            "start_id": self.start_id,
            "end_id": self.end_id
        }

@dataclass
class GraphTraversalResponse:
    status: str
    start_id: str
    direction: str
    max_hops: int
    min_weight: float
    nodes: List[str]
    
    @classmethod
    def from_dict(cls, data: dict) -> 'GraphTraversalResponse':
        return cls(
            status=data.get("status", ""),
            start_id=data.get("start_id", ""),
            direction=data.get("direction", ""),
            max_hops=data.get("max_hops", 0),
            min_weight=data.get("min_weight", 0.0),
            nodes=data.get("nodes", [])
        )

@dataclass
class ShortestPathResponse:
    status: str
    start_id: str
    end_id: str
    path: List[str]
    path_length: int
    
    @classmethod
    def from_dict(cls, data: dict) -> 'ShortestPathResponse':
        return cls(
            status=data.get("status", ""),
            start_id=data.get("start_id", ""),
            end_id=data.get("end_id", ""),
            path=data.get("path", []),
            path_length=data.get("path_length", 0)
        )

@dataclass
class RelatedNodesResponse:
    status: str
    point_id: str
    min_weight: float
    related_nodes: List[str]
    count: int
    
    @classmethod
    def from_dict(cls, data: dict) -> 'RelatedNodesResponse':
        return cls(
            status=data.get("status", ""),
            point_id=data.get("point_id", ""),
            min_weight=data.get("min_weight", 0.0),
            related_nodes=data.get("related_nodes", []),
            count=data.get("count", 0)
        )