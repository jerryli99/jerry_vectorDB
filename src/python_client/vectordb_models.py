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
#I might keep this
@dataclass
class QueryResponse:
    # For single-query: a flat list of ScoredPoint
    # For batch-query: a list of lists of ScoredPoint
    result: Union[List[ScoredPoint], List[List[ScoredPoint]]]
    status: str
    time: float

    @classmethod
    def from_dict(cls, data: dict) -> "QueryResponse":
        raw_result = data.get("result")

        #single query
        if len(raw_result) > 0 and isinstance(raw_result[0], dict):
            parsed_result = [
                ScoredPoint(id=p["id"], score=p["score"])
                for p in raw_result
            ]

        #batch query
        else:
            parsed_result = [
                [ScoredPoint(id=p["id"], score=p["score"]) for p in group]
                for group in raw_result
            ]

        return cls(
            result=parsed_result,
            status=data.get("status", ""),
            time=data.get("time", 0.0)
        )