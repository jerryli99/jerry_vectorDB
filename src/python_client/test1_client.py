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
    # STEP 1: Create single-vector collection with on_disk=True
    # ------------------------------------------------------------
    single_cfg = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=8, distance="L2"),
        on_disk="true"  # Changed to true
    )
    print("\n[Test 1] Create 'single_collection' with on_disk=true")
    print(client.create_collection("single_collection", single_cfg))

    # ------------------------------------------------------------
    # STEP 2: Create multi-vector collection with on_disk=True
    # ------------------------------------------------------------
    multi_cfg = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=4, distance="Dot"),
            "text": models.VectorParams(size=4, distance="Cosine")
        },
        on_disk="true"  # Changed to true
    )
    print("\n[Test 2] Create 'multi_collection' with on_disk=true")
    print(client.create_collection("multi_collection", multi_cfg))

    # ------------------------------------------------------------
    # STEP 3: Insert 50 random points into single_collection
    # ------------------------------------------------------------
    print("\n[Test 3] Upsert 100000 random points to single_collection")
    points = [
        models.PointStruct(
            id=f"p_{i}",
            vector=generate_random_vector(8),
            payload={"category": "test", "index": i}
        )
        for i in range(100000)
    ]
    print(client.upsert("single_collection", points))

    # ------------------------------------------------------------
    # STEP 4: Insert 100 random points into multi_collection
    # ------------------------------------------------------------
    print("\n[Test 4] Upsert 10000 random points to multi_collection")
    multi_points = [
        models.PointStruct(
            id=f"m_{i}",
            vector={
                "image": generate_random_vector(4),
                "text": generate_random_vector(4)
            },
            payload={"category": "multi", "index": i}
        )
        for i in range(10000)  # Reduced from 10000 to 100 for testing
    ]
    print(client.upsert("multi_collection", multi_points))

    # ------------------------------------------------------------
    # STEP 5: List collections at the end
    # ------------------------------------------------------------
    print("\n[Test 5] List collections after all operations:")
    print(client.list_collections())

    print("\n[INFO] All data operations completed with on_disk=true")
    print("[INFO] Query functionality has been removed as requested")


if __name__ == "__main__":
    random.seed(time.time())
    run_tests()