from dataclasses import dataclass
from typing import Dict, List, Optional, Union, Literal

"""
OK, so for now I only supoprt ids of string type, no integer type.
I can always add it later.
"""

@dataclass
class VectorParams:
    size: int
    distance: Literal["Cosine", "L2", "Dot"]
    # on_disk: Optional[bool] = False

    def to_dict(self):
        return {
            "size": self.size,
            "distance": self.distance,
            # "on_disk": self.on_disk
        }

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
    
@dataclass
class ScoredPoint:
    id: str
    score: float


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

        # Single query
        if len(raw_result) > 0 and isinstance(raw_result[0], dict):
            parsed_result = [
                ScoredPoint(id=p["id"], score=p["score"])
                for p in raw_result
            ]

        # Batch query
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