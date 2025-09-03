from vectordb_client import VectorDBClient
import vectordb_models as models

def run_tests():
    client = VectorDBClient()

    # Initial list
    print("Test 0: List collections (initially empty)")
    print(client.list_collections())

    # Create single-vector collection
    single_config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=480, distance="Cosine")
    )
    print("Test 1: Create single-vector collection")
    print(client.create_collection("single_collection", single_config))

    # Create multi-vector collection
    multi_config = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=3, distance="Dot"),
            "text": models.VectorParams(size=10, distance="Cosine"),
            "audio": models.VectorParams(size=100, distance="Cosine"),
        }
    )
    print("Test 2: Create multi-vector collection")
    print(client.create_collection("multi_collection", multi_config))

    # List after creation
    print("Test 2.5: List collections after creation")
    print(client.list_collections())

    # Upsert with list of PointStruct (single vector)
    points_list = [
        models.PointStruct(id="22s3", vector=[0.1, 0.2, 0.3, 0.4], payload={"label": "cat"}),
        models.PointStruct(id="12wer", vector=[0.5, 0.6, 0.7, 0.8], payload={"label": "dog"}),
        models.PointStruct(id="not-teder", vector=[0.12, 0.436, 0.7, 0.18], payload={"label": "wowow"}),
    ]
    print("Test 3: Upsert with single vectors")
    print(client.upsert("single_collection", points_list))

    # Upsert with multi-vector schema
    multi_vec_points = [
        models.PointStruct(
            id="img_1",
            vector={
                "image": [0.1, 0.2, 0.3],
                "text": [0.5, 0.6, 0.7],
            },
            payload={"type": "image+text"}
        ),
        models.PointStruct(
            id="img_q",
            vector={
                "image": [0.4, 0.13, 0.23],
                "text": [0.35, 0.16, 0.7],
            },
            payload={"type": "image+text", "key": 23}
        )
    ]
    print("Test 4: Upsert with multi-vector schema")
    print(client.upsert("multi_collection", multi_vec_points))

    # Delete collections
    print("Test 5: Delete single_collection")
    print(client.delete_collection("single_collection"))

    print("Test 6: Delete multi_collection")
    print(client.delete_collection("multi_collection"))

    # List after deletion
    print("Test 7: List collections after deletion")
    print(client.list_collections())




if __name__ == "__main__":
    run_tests()

