from vectordb_client import VectorDBClient
import vectordb_models as models
import random

def run_tests():
    client = VectorDBClient()

    # Initial list
    print("Test 0: List collections (initially empty)")
    print(client.list_collections())

    # Create single-vector collection with on_disk=true
    single_config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=128, distance="Cosine"),  # Increased size for more realistic data
        on_disk="true"  # Changed to true for disk persistence
    )
    print("Test 1: Create single-vector collection with on_disk=true")
    print(client.create_collection("large_collection", single_config))

    # List after creation
    print("Test 2: List collections after creation")
    print(client.list_collections())

    # Generate and upsert 100,000 points
    print("Test 3: Generating 100,000 points...")
    points_list = []
    
    for i in range(100000):
        # Generate random vector of size 128
        vector = [random.uniform(-1.0, 1.0) for _ in range(128)]
        
        # Create point with sequential ID and some payload data
        point = models.PointStruct(
            id=f"point_{i}",
            vector=vector,
            payload={
                "index": i,
                "category": f"category_{i % 10}",  # 10 different categories
                "timestamp": i * 1000,
                "active": i % 2 == 0  # Boolean flag
            }
        )
        points_list.append(point)
        
        # Upsert in batches to avoid memory issues
        if len(points_list) >= 1000:  # Batch size of 1000
            print(f"Upserting batch {i//1000 + 1}/100...")
            result = client.upsert("large_collection", points_list)
            points_list = []  # Reset for next batch
    
    # Upsert any remaining points
    if points_list:
        print("Upserting final batch...")
        result = client.upsert("large_collection", points_list)
    
    print("Test 4: Completed upserting 100,000 points")

    # Verify the collection
    print("Test 5: List collections after data insertion")
    print(client.list_collections())

    # Optional: You might want to add a count operation here if your client supports it
    # print("Test 6: Count points in collection")
    # print(client.count("large_collection"))

if __name__ == "__main__":
    run_tests()