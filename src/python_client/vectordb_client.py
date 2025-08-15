import requests
from typing import List, Union, Optional
from vectordb_models  import CreateCollectionRequest, PointStruct, Batch


class VectorDBClient:
    def __init__(self, host: str = "http://127.0.0.1:8989"):
        self.host = host

    def create_collection(self, name: str, config: CreateCollectionRequest) -> Optional[dict]:
        url = f"{self.host}/collections/{name}"
        return self._put(url, config.to_dict())

    def list_collections(self) -> Optional[dict]:
        url = f"{self.host}/collections"
        try:
            response = requests.get(url)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None
    
    def delete_collection(self, name: str) -> Optional[dict]:
        url = f"{self.host}/collections/{name}"
        return self._delete(url)
    
    def upsert(self, collection_name: str, points: Union[PointStruct, List[PointStruct], Batch]) -> Optional[dict]:
        payload = {"collection_name": collection_name}

        # Single PointStruct
        if isinstance(points, PointStruct):
            self._validate_point_id(points.id)
            payload["points"] = [points.to_dict()]

        # List of PointStruct
        elif isinstance(points, list):
            for p in points:
                self._validate_point_id(p.id)
            payload["points"] = [p.to_dict() for p in points]

        # Batch
        elif isinstance(points, Batch):
            for pid in points.ids:
                self._validate_point_id(pid)
            payload["points"] = points.to_dict()

        else:
            raise ValueError("Unsupported points format")

        url = f"{self.host}/upsert"
        return self._post(url, payload)


    def _validate_point_id(self, pid):
        if not isinstance(pid, str):
            raise TypeError(f"Point ID must be a string, got {type(pid).__name__}")



    # Some helpers-------------------------------------------------
    def _post(self, url: str, data: dict) -> Optional[dict]:
        try:
            response = requests.post(url, json=data)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None

    def _put(self, url: str, data: dict) -> Optional[dict]:
        try:
            response = requests.put(url, json=data)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None

    def _delete(self, url: str) -> Optional[dict]:
        try:
            response = requests.delete(url)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None
