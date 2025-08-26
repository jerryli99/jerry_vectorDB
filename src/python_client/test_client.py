from vectordb_client import VectorDBClient
import vectordb_models as models
def run_tests():
    client = VectorDBClient()

    # Initial list
    print("Test 0: List collections (initially empty)")
    print(client.list_collections())

    # Create collection with a single unnamed vector config
    single_config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=4, distance="Cosine")
    )
    print("Test 1: Create single-vector collection")
    print(client.create_collection("single_collection", single_config))

    # Create collection with multiple named vectors
    multi_config = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=4, distance="Dot", on_disk=True),
            "text": models.VectorParams(size=5, distance="Cosine"),
            "audio": models.VectorParams(size=3, distance="L2")
        }
    )
    print("Test 2: Create multi-vector collection")
    print(client.create_collection("multi_collection", multi_config))

    # List after creation
    print("Test 2.5: List collections after creation")
    print(client.list_collections())

    # Upsert with list of PointStruct
    points_list = [
        models.PointStruct(id="22s3", vector=[0.1, 0.2, 0.3, 0.4], payload={"label": "cat"}),
        models.PointStruct(id="12wer", vector=[0.5, 0.6, 0.7, 0.8], payload={"label": "dog"}),
    ]
    print("Test 3: Upsert with list of PointStruct")
    print(client.upsert("single_collection", points_list))

    # Upsert with Batch
    batch = models.Batch(
        ids=["12", "ew"],
        vectors=[[0.9, 1.0, 1.1, 1.2], [1.3, 1.4, 1.5, 1.6]],
        payloads=[{"label": "bird"}, {"label": "fish"}],
    )
    print("Test 4: Upsert with Batch")
    print(client.upsert("multi_collection", batch))

    # # Delete collections
    print("Test 5: Delete single_collection")
    print(client.delete_collection("single_collection"))

    print("Test 6: Delete multi_collection")
    print(client.delete_collection("multi_collection"))

    # List after deletion
    print("Test 7: List collections after deletion")
    print(client.list_collections())


if __name__ == "__main__":
    run_tests()

