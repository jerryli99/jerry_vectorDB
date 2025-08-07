import requests
from typing import List, Union, Optional
from vectordb_models  import PointStruct, Batch


class VectorDBClient:
    def __init__(self, host: str = "http://127.0.0.1:8989"):
        self.host = host

    def upsert(self, collection_name: str, points: Union[List[PointStruct], Batch]) -> Optional[dict]:
        payload = {"collection_name": collection_name}

        if isinstance(points, list):
            payload["points"] = [p.to_dict() for p in points]
        elif isinstance(points, Batch):
            payload["points"] = points.to_dict()
        else:
            raise ValueError("Unsupported points format")

        url = f"{self.host}/upsert"
        return self._post(url, payload)

    def _post(self, url: str, data: dict) -> Optional[dict]:
        try:
            response = requests.post(url, json=data)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None
