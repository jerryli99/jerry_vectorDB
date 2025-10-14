import random
import time
from vectordb_client import VectorDBClient
import vectordb_models as models


def generate_random_vector(dim: int) -> list[float]:
    return [round(random.random(), 2) for _ in range(dim)]


def run_tests():
    client = VectorDBClient()

    print("\nTest 0: List collections (initially)")
    print(client.list_collections())

    # --- Create single-vector collection ---
    single_config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=100, distance="L2"),
        on_disk="false"
    )
    print("\nTest 1: Create 'single_collection'")
    print(client.create_collection("single_collection", single_config))

    # --- Create multi-vector collection ---
    multi_config = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=10, distance="L2"),
            "text": models.VectorParams(size=20, distance="Cosine"),
        },
        on_disk="false"
    )
    print("\nTest 2: Create 'multi_collection'")
    print(client.create_collection("multi_collection", multi_config))

    # ---List collections ---
    print("\nTest 3: List collections after creation")
    print(client.list_collections())

    # ---Upsert 1000 random points into single_collection ---
    N = 1000
    points = [
        models.PointStruct(
            id=f"p_{i}",
            vector=generate_random_vector(100),
            payload={"idx": i, "type": "single"}
        )
        for i in range(N)
    ]

    print(f"\nTest 4: Upsert {N} points into 'single_collection'")
    start = time.time()
    result = client.upsert("single_collection", points)
    print("Status:", result.get("status") if result else "no response")
    print(f"Time: {time.time() - start:.3f}s\n")

    # ---Upsert 1000 random points into multi_collection ---
    points_multi = [
        models.PointStruct(
            id=f"mv_{i}",
            vector={
                "image": generate_random_vector(10),
                "text": generate_random_vector(20),
            },
            payload={"idx": i, "type": "multi"}
        )
        for i in range(N)
    ]
    print(f"\n📤 Test 5: Upsert {N} points into 'multi_collection'")
    start = time.time()
    result = client.upsert("multi_collection", points_multi)
    print("Status:", result.get("status") if result else "no response")
    print(f"Time: {time.time() - start:.3f}s\n")

    # ---Query test: single vector query ---
    print("\n🔍 Test 6: Query 'single_collection' with random vector")
    query_vec = [generate_random_vector(4)]
    res = client.query_points(
        collection_name="single_collection",
        query_vectors=query_vec,
        using="default",
        top_k=5
    )

    if res:
        print(f"Query status: {res.status}")
        print("Top 5 results:")
        for sp in res.result[:5]:
            print(f"  id={sp.id}, score={sp.score:.4f}")
    else:
        print("Query failed or empty result.")

    # ---Query test: multi collection by ID ---
    print("\n🔍 Test 7: Query 'multi_collection' by point IDs")
    res = client.query_points(
        collection_name="multi_collection",
        query_pointids=["mv_0", "mv_10", "mv_20"],
        using="image",
        top_k=3
    )
    if res:
        print(f"Query status: {res.status}")
        if isinstance(res.result[0], list):
            print(f"Batch result count: {len(res.result)} queries")
            print("First batch:")
            for sp in res.result[0]:
                print(f"  id={sp.id}, score={sp.score:.4f}")
        else:
            print("Single query result:")
            for sp in res.result:
                print(f"  id={sp.id}, score={sp.score:.4f}")
    else:
        print("Query failed or empty result.")

    # print("\n🧹 Test 8: Delete collections")
    # print(client.delete_collection("single_collection"))
    # print(client.delete_collection("multi_collection"))

    print("\nAll tests completed.")


if __name__ == "__main__":
    run_tests()
