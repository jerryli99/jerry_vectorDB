import requests
import json
from vectordb_models import *
from typing import List, Union, Optional

# from vectordb_models import CreateCollectionRequest, PointStruct, UpsertBatch, QueryRequest


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

        total_points = len(points)
        
        # --- Skip batching if <= 1000 ---
        if total_points <= 1000:
            print(f"[INFO] Upserting {total_points} points in a single request")
            payload = dict(payload_base)
            payload["points"] = [p.to_dict() for p in points]
            response = self._post(f"{self.host}/upsert", payload)
            if not response or response.get("status") != "ok":
                print(f"[ERROR] Upsert failed: {response}")
            else:
                print(f"[SUCCESS] Upsert completed")
            return response

        # --- Otherwise, batch in chunks of 1000 ---
        batch_size = 1000
        batches = [points[i:i + batch_size] for i in range(0, total_points, batch_size)]
        print(f"[INFO] Upserting {total_points} points in {len(batches)} batch(es) (max 1000 per batch)")

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
    
    def query_points(
            self,
            collection_name: str,
            query_vectors: Optional[List[List[float]]] = None,
            query_pointids: Optional[List[str]] = None,
            using: str = "default",
            top_k: Optional[int] = 10,
        ) -> Optional[QueryResponse]:
            """
            Query the collection using either query vectors or existing point IDs.
            """

            def round_selected_fields(obj, digits=4): 
                if isinstance(obj, dict): 
                    new_obj = {} 
                    for k, v in obj.items(): 
                        if k in ("score", "time") and isinstance(v, (int, float)): 
                            new_obj[k] = round(v, digits) 
                        else: 
                            new_obj[k] = round_selected_fields(v, digits)

                    return new_obj 
                elif isinstance(obj, list): 
                    return [round_selected_fields(x, digits) for x in obj] 
                return obj
            
            # Build and validate request object
            try:
                req = QueryRequest(
                    collection_name=collection_name,
                    query_vectors=query_vectors,
                    query_pointids=query_pointids,
                    using=using,
                    top_k=top_k if top_k is not None else 0
                )
            except ValueError as e:
                print(f"[ERROR] Invalid query request: {e}")
                return None
            except TypeError as e:
                print(f"[ERROR] Invalid query request: {e}")
                return None

            payload = req.to_dict()
            url = f"{self.host}/collections/{collection_name}/query"

            # Send to backend
            response = self._post(url, payload)
            if not response:
                print("[ERROR] No response from server.")
                return None

            response = round_selected_fields(response) 
            
            if response.get("status") == "ok": 
                try: 
                    return QueryResponse.from_dict(response) 
                except Exception as e: 
                    print(f"[WARN] Failed to parse QueryResponse: {e}") 
                    return None 
            else: 
                print(f"[ERROR] Query failed - status not 'ok': {response}") 
                return None


    def parse_query_response(self, response: QueryResponse, show: Optional[bool] = None):
        """
        Pretty-print and also return a parsed JSON dictionary of the response.
        """
        if response is None:
            print("[ERROR] No response to display.")
            return None

        # Convert the QueryResponse dataclass into a clean dict
        def to_dict(obj):
            if isinstance(obj, QueryResponse):
                return {
                    "status": obj.status,
                    "time": obj.time,
                    "result": [
                        [{"id": p.id, "score": p.score} for p in group]
                        for group in obj.result
                    ] if isinstance(obj.result, list) and len(obj.result) > 0 and isinstance(obj.result[0], list)
                    else [{"id": p.id, "score": p.score} for p in obj.result]
                }
            return obj

        parsed = to_dict(response)

        # Pretty print
        if show is True:
            print(json.dumps(parsed, indent=2))

        return parsed

    # Graph Methods+++++++++++++++++++++++++
    def add_graph_relationship(
        self, 
        collection_name: str, 
        from_id: str, 
        to_id: str, 
        relationship: str, 
        weight: float = 1.0
    ) -> Optional[dict]:
        """
        Add a relationship between two points in the graph
        
        Args:
            collection_name: Name of the collection
            from_id: Source point ID
            to_id: Target point ID  
            relationship: Type of relationship ("similar_to", "contains", etc.)
            weight: Relationship strength (0.0 to 1.0)
        """
        url = f"{self.host}/collections/{collection_name}/graph/relationships"
        payload = {
            "from_id": from_id,
            "to_id": to_id,
            "relationship": relationship,
            "weight": weight
        }
        return self._post(url, payload)

    def get_node_relationships(
        self, 
        collection_name: str, 
        node_id: str
    ) -> Optional[List[GraphEdge]]:
        """
        Get all relationships for a specific node
        """
        url = f"{self.host}/collections/{collection_name}/graph/nodes/{node_id}/relationships"
        
        response = self._get(url)
        if response and response.get("status") == "ok":
            relationships_data = response.get("relationships", [])
            return [GraphEdge.from_dict(edge_data) for edge_data in relationships_data]
        return None

    def graph_traversal(
        self, 
        collection_name: str, 
        start_id: str,
        direction: Literal["outwards", "inwards", "both"] = "outwards",
        max_hops: int = 2,
        min_weight: float = 0.0
    ) -> Optional[GraphTraversalResponse]:
        """
        Traverse the graph from a starting node
        
        Args:
            collection_name: Name of the collection
            start_id: Starting point ID
            direction: "outwards", "inwards", or "both"
            max_hops: Maximum number of hops to traverse
            min_weight: Minimum relationship weight to follow
        """
        url = f"{self.host}/collections/{collection_name}/graph/traverse"
        payload = {
            "start_id": start_id,
            "direction": direction,
            "max_hops": max_hops,
            "min_weight": min_weight
        }
            
        response = self._post(url, payload)
        if response and response.get("status") == "ok":
            return GraphTraversalResponse.from_dict(response)
        return None

    def find_shortest_path(
        self, 
        collection_name: str, 
        start_id: str, 
        end_id: str
    ) -> Optional[ShortestPathResponse]:
        """
        Find the shortest path between two nodes
        """
        url = f"{self.host}/collections/{collection_name}/graph/shortest-path"
        payload = {
            "start_id": start_id,
            "end_id": end_id
        }
        response = self._post(url, payload)
        if response and response.get("status") == "ok":
            return ShortestPathResponse.from_dict(response)
        return None

    def find_related_by_weight(
        self, 
        collection_name: str, 
        point_id: str, 
        min_weight: float = 0.7
    ) -> Optional[RelatedNodesResponse]:
        """
        Find strongly connected nodes by weight threshold
        """
        url = f"{self.host}/collections/{collection_name}/graph/nodes/{point_id}/related"
        params = {"min_weight": str(min_weight)}
        
        response = self._get(url, params=params)
        if response and response.get("status") == "ok":
            return RelatedNodesResponse.from_dict(response)
        return None

    def get_graph_data(
        self, 
        collection_name: str
    ) -> Optional[dict]:
        """
        Get the complete graph data for a collection
        """
        url = f"{self.host}/collections/{collection_name}/graph"
        return self._get(url)

    def batch_add_relationships(
        self,
        collection_name: str,
        relationships: List[GraphRelationship]
    ) -> Optional[dict]:
        """
        Add multiple relationships in batch
        """
        # Note: You'll need to implement a batch endpoint on the server side
        # For now, we'll do sequential requests
        results = []
        for rel in relationships:
            result = self.add_graph_relationship(
                collection_name, rel.from_id, rel.to_id, rel.relationship, rel.weight
            )
            results.append(result)
        return results

    # Add _get helper method if you don't have it
    def _get(self, url: str, params: Optional[dict] = None) -> Optional[dict]:
        try:
            response = requests.get(url, params=params)
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

    def _validate_point_id(self, pid):
        if not isinstance(pid, str):
            raise TypeError(f"Point ID must be a string, got {type(pid).__name__}")

