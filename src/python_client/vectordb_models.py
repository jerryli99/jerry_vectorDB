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
    on_disk: Optional[bool] = False

    def to_dict(self):
        return {
            "size": self.size,
            "distance": self.distance,
            "on_disk": self.on_disk
        }

@dataclass
class CreateCollectionRequest:
    vectors: Union[VectorParams, Dict[str, VectorParams]]

    def to_dict(self):
        if isinstance(self.vectors, dict):
            return {"vectors": {k: v.to_dict() for k, v in self.vectors.items()}}
        return {"vectors": self.vectors.to_dict()}
    


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
class Batch:
    ids: List[str]
    vectors: List[List[float]]
    payloads: Optional[List[Dict]] = None

    def to_dict(self):
        d = {"ids": self.ids, "vectors": self.vectors}
        if self.payloads is not None:
            d["payloads"] = self.payloads
        return d
    