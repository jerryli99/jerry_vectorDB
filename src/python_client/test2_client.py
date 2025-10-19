from vectordb_client import VectorDBClient
import vectordb_models as models
import random
import time


def generate_random_vector(dim: int):
    """Generate a random float vector of length `dim`."""
    return [round(random.random(), 4) for _ in range(dim)]


def run_tests():
    client = VectorDBClient()

    # ------------------------------------------------------------
    # STEP 0: Initial collections list
    # ------------------------------------------------------------
    print("\n[Test 0] List collections (initially):")
    print(client.list_collections())

    # ------------------------------------------------------------
    # STEP 1: Create single-vector collection
    # ------------------------------------------------------------
    single_cfg = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=8, distance="L2"),
        on_disk="false"
    )
    print("\n[Test 1] Create 'single_collection'")
    print(client.create_collection("single_collection", single_cfg))

    # ------------------------------------------------------------
    # STEP 2: Create multi-vector collection
    # ------------------------------------------------------------
    multi_cfg = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=4, distance="Dot"),
            "text": models.VectorParams(size=4, distance="Cosine")
        },
        on_disk="false"
    )
    print("\n[Test 2] Create 'multi_collection'")
    print(client.create_collection("multi_collection", multi_cfg))

    # ------------------------------------------------------------
    # STEP 3: Insert 50 random points into single_collection
    # ------------------------------------------------------------
    print("\n[Test 3] Upsert 50 random points to single_collection")
    points = [
        models.PointStruct(
            id=f"p_{i}",
            vector=generate_random_vector(8),
            payload={"category": "test", "index": i}
        )
        for i in range(50)
    ]
    print(client.upsert("single_collection", points))

    # ------------------------------------------------------------
    # STEP 4: Insert 100 random points into multi_collection
    # ------------------------------------------------------------
    print("\n[Test 4] Upsert 100 random points to multi_collection")
    multi_points = [
        models.PointStruct(
            id=f"m_{i}",
            vector={
                "image": generate_random_vector(4),
                "text": generate_random_vector(4)
            },
            payload={"category": "multi", "index": i}
        )
        for i in range(10000)
    ]
    print(client.upsert("multi_collection", multi_points))

    # ------------------------------------------------------------
    # STEP 5: Query by vector (single_collection)
    # ------------------------------------------------------------
    print("\n[Test 5] Query single_collection by vector")
    query_vec = generate_random_vector(8)
    query_vec2 = generate_random_vector(8)
    query_vec3 = generate_random_vector(8)
    result = client.query_points(
        collection_name="single_collection",
        query_vectors=[query_vec, query_vec2, query_vec3],
        using="default",
        top_k=5
    )
    parsed = client.parse_query_response(result, show=True)

    # ------------------------------------------------------------
    # STEP 6: Query by point IDs (multi_collection)
    # ------------------------------------------------------------
    # print("\n[Test 6] Query multi_collection by existing point IDs")
    # query_ids = ["m_3", "m_10"]
    # query_req2 = models.QueryRequest(
    #     collection_name="multi_collection",
    #     query_pointids=query_ids,
    #     using="default",
    # )
    # result2 = client.query(query_req2)
    # print(result2)

    print("\n[Test 6] Query multi_collection by vector")
    query_vec = generate_random_vector(4)
    query_vec2 = generate_random_vector(4)
    result = client.query_points(
        collection_name="multi_collection",
        query_vectors=[query_vec, query_vec2],
        using="image",
        top_k=15
    )
    parsed = client.parse_query_response(result)
    # ------------------------------------------------------------
    # STEP 7: List collections at the end
    # ------------------------------------------------------------
    print("\n[Test 7] List collections after all operations:")
    print(client.list_collections())


if __name__ == "__main__":
    random.seed(time.time())
    run_tests()
