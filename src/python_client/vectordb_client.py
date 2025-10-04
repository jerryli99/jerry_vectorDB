import requests
from typing import List, Union, Optional
from vectordb_models import CreateCollectionRequest, PointStruct, UpsertBatch, QueryResponse


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
    
    def upsert(self, collection_name: str, points: Union[PointStruct, List[PointStruct], UpsertBatch]) -> Optional[dict]:
        payload_base = {"collection_name": collection_name}

        # --- Normalize input ---
        if isinstance(points, PointStruct):
            points = [points]
        elif isinstance(points, UpsertBatch):
            points = [PointStruct(pid, vecs) for pid, vecs in zip(points.ids, points.vectors)]
        elif not isinstance(points, list):
            raise ValueError("Unsupported points format")

        if not points:
            raise ValueError("No points provided for upsert")

        # --- Deduplicate by point_id ---
        merged_points = {}
        for p in points:
            self._validate_point_id(p.id)
            if p.id not in merged_points:
                merged_points[p.id] = p
            else:
                merged_points[p.id].vectors.update(p.vectors)

        # --- Remove identical named vectors ---
        unique_points = []
        for p in merged_points.values():
            seen = set()
            filtered_vectors = {}
            for name, vec in p.vectors.items():
                vec_tuple = tuple(vec)
                if (name, vec_tuple) not in seen:
                    seen.add((name, vec_tuple))
                    filtered_vectors[name] = vec
            p.vectors = filtered_vectors
            unique_points.append(p)

        total_points = len(unique_points)
        if total_points == 0:
            print("[WARN] No unique points to upsert (all duplicates removed).")
            return None

        # --- Skip batching if <= 1000 ---
        if total_points <= 1000:
            print(f"[INFO] Upserting {total_points} unique points in a single request")
            payload = dict(payload_base)
            payload["points"] = [p.to_dict() for p in unique_points]
            response = self._post(f"{self.host}/upsert", payload)
            if not response or response.get("status") != "ok":
                print(f"[ERROR] Upsert failed: {response}")
            else:
                print(f"[SUCCESS] Upsert completed")
            return response

        # --- Otherwise, batch in chunks of 1000 ---
        batch_size = 1000
        batches = [unique_points[i:i + batch_size] for i in range(0, total_points, batch_size)]
        print(f"[INFO] Upserting {total_points} unique points in {len(batches)} batch(es) (max 1000 per batch)")

        last_response = None
        for i, batch in enumerate(batches, start=1):
            payload = dict(payload_base)
            payload["points"] = [p.to_dict() for p in batch]
            url = f"{self.host}/upsert"

            print(f"[INFO] Uploading batch {i}/{len(batches)} ({len(batch)} points)...")
            response = self._post(url, payload)
            last_response = response

            if not response or response.get("status") != "ok":
                print(f"[ERROR] Batch {i} failed: {response}")
                break

            print(f"[SUCCESS] Batch {i}/{len(batches)} completed")

        return last_response


    def _validate_point_id(self, pid):
        if not isinstance(pid, str):
            raise TypeError(f"Point ID must be a string, got {type(pid).__name__}")

    def query_points(
        self,
        collection_name: str,
        query_vectors: Optional[Union[List[float], List[List[float]]]] = None,
        query_pointids: Optional[Union[str, List[str]]] = None,
        top_k: int = 5 # default to 5
    ) -> Optional[QueryResponse]:

        if (query_vectors is None) and (query_pointids is None):
            raise ValueError("Either query_vectors or query_pointids fields must be provided")

        if not isinstance(top_k, int) or top_k <= 0:
            raise ValueError(f"top_k must be a positive integer, got {top_k}")

        payload = {"collection_name": collection_name, "top_k": top_k}

        if query_vectors is not None:
            if isinstance(query_vectors[0], (float, int)):
                payload["query_vectors"] = [query_vectors]
            else:
                payload["query_vectors"] = query_vectors

        if query_pointids is not None:
            if isinstance(query_pointids, str):
                self._validate_point_id(query_pointids)
                payload["query_pointids"] = [query_pointids]
            else:
                for pid in query_pointids:
                    self._validate_point_id(pid)
                payload["query_pointids"] = query_pointids

        url = f"{self.host}/collections/{collection_name}/query"
        raw = self._post(url, payload)
        return QueryResponse.from_dict(raw) if raw else None

    # Some helpers-------------------------------------------------
    def _post(self, url: str, data: dict) -> Optional[dict]:
        try:
            response = requests.post(url, json=data)
            response.raise_for_status()
            return response.json()
        except requests.HTTPError:
            try:
                return response.json()
            except ValueError:
                return {"status": "error", "message": response.text}
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None

    def _put(self, url: str, data: dict) -> Optional[dict]:
        try:
            response = requests.put(url, json=data)
            response.raise_for_status()
            return response.json()
        except requests.HTTPError:
            try:
                return response.json()
            except ValueError:
                return {"status": "error", "message": response.text}
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None

    def _delete(self, url: str) -> Optional[dict]:
        try:
            response = requests.delete(url)
            response.raise_for_status()
            return response.json()
        except requests.HTTPError:
            try:
                return response.json()
            except ValueError:
                return {"status": "error", "message": response.text}
        except requests.RequestException as e:
            print(f"[ERROR] {e}")
            return None
