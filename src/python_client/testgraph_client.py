from vectordb_client import VectorDBClient
import vectordb_models as models

def test_graph_functionality():
    """Test the graph functionality of the vector database"""
    client = VectorDBClient()
    
    # Create a test collection
    print("=== Creating Test Collection ===")
    config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=128, distance="Cosine"),
        on_disk="false"
    )
    
    result = client.create_collection("test_graph", config)
    print(f"Create collection: {result}")
    
    # Add some test points
    print("\n=== Adding Test Points ===")
    points = [
        models.PointStruct("point_1", [0.1] * 128),
        models.PointStruct("point_2", [0.2] * 128),
        models.PointStruct("point_3", [0.3] * 128),
        models.PointStruct("point_4", [0.4] * 128),
        models.PointStruct("point_5", [0.5] * 128),
    ]
    
    result = client.upsert("test_graph", points)
    print(f"Upsert points: {result}")
    
    # Test graph relationships
    print("\n=== Testing Graph Relationships ===")
    
    # Add relationships
    relationships = [
        ("point_1", "point_2", "similar_to", 0.9),
        ("point_1", "point_3", "similar_to", 0.8),
        ("point_2", "point_4", "contains", 0.95),
        ("point_3", "point_5", "derived_from", 0.7),
        ("point_4", "point_5", "related", 0.6),
    ]
    
    for from_id, to_id, rel, weight in relationships:
        result = client.add_graph_relationship("test_graph", from_id, to_id, rel, weight)
        print(f"Add relationship {from_id} -> {to_id}: {result.get('status', 'error')}")
    
    # Get node relationships
    print("\n=== Testing Node Relationships ===")
    relationships = client.get_node_relationships("test_graph", "point_1")
    if relationships:
        print(f"Relationships for point_1:")
        for rel in relationships:
            print(f"  - {rel.from_id} -> {rel.to_id} ({rel.relationship}, weight: {rel.weight})")
    
    # Test graph traversal
    print("\n=== Testing Graph Traversal ===")
    traversal = client.graph_traversal(
        collection_name="test_graph",
        start_id="point_1",
        direction="outwards",
        max_hops=2
    )
    if traversal:
        print(f"Traversal from point_1: {traversal.nodes}")
    
    # Test shortest path
    print("\n=== Testing Shortest Path ===")
    shortest_path = client.find_shortest_path("test_graph", "point_1", "point_5")
    if shortest_path:
        print(f"Shortest path from point_1 to point_5: {shortest_path.path}")
        print(f"Path length: {shortest_path.path_length}")
    
    # Test related nodes by weight
    print("\n=== Testing Related Nodes by Weight ===")
    related = client.find_related_by_weight("test_graph", "point_1", min_weight=0.8)
    if related:
        print(f"Nodes strongly related to point_1 (weight >= 0.8): {related.related_nodes}")
        print(f"Count: {related.count}")
    
    # Get complete graph data
    print("\n=== Testing Graph Data Retrieval ===")
    graph_data = client.get_graph_data("test_graph")
    if graph_data:
        print(f"Graph data keys: {list(graph_data.keys())}")
        if 'nodes' in graph_data and 'edges' in graph_data:
            print(f"Graph has {len(graph_data['nodes'])} nodes and {len(graph_data['edges'])} edges")
    
    # Clean up
    print("\n=== Cleaning Up ===")
    result = client.delete_collection("test_graph")
    print(f"Delete collection: {result}")

def test_advanced_graph_scenarios():
    """Test more advanced graph scenarios"""
    client = VectorDBClient()
    
    # Create collection for advanced testing
    config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=64, distance="Cosine"),
        on_disk="false"
    )
    client.create_collection("advanced_graph", config)
    
    # Add points representing different entities
    points = [
        models.PointStruct("user_1", [0.1] * 64, {"type": "user", "name": "Alice"}),
        models.PointStruct("user_2", [0.2] * 64, {"type": "user", "name": "Bob"}),
        models.PointStruct("product_1", [0.3] * 64, {"type": "product", "name": "Laptop"}),
        models.PointStruct("product_2", [0.4] * 64, {"type": "product", "name": "Phone"}),
        models.PointStruct("category_1", [0.5] * 64, {"type": "category", "name": "Electronics"}),
    ]
    client.upsert("advanced_graph", points)
    
    # Build a complex relationship graph
    relationships = [
        # User relationships
        ("user_1", "user_2", "friend", 0.8),
        
        # User-product interactions
        ("user_1", "product_1", "purchased", 0.9),
        ("user_1", "product_2", "viewed", 0.6),
        ("user_2", "product_2", "purchased", 0.95),
        
        # Product-category relationships
        ("product_1", "category_1", "belongs_to", 1.0),
        ("product_2", "category_1", "belongs_to", 1.0),
        
        # Similarity relationships (could be based on vector similarity)
        ("product_1", "product_2", "similar", 0.7),
    ]
    
    for from_id, to_id, rel, weight in relationships:
        client.add_graph_relationship("advanced_graph", from_id, to_id, rel, weight)
    
    print("=== Advanced Graph Scenarios ===")
    
    # Find what users might like based on friend's purchases
    print("\n1. Recommendation based on friend's purchases:")
    friend_purchases = client.graph_traversal(
        "advanced_graph", "user_1", direction="outwards", 
        max_hops=2, min_weight=0.7
    )
    if friend_purchases:
        print(f"Potential recommendations for user_1: {friend_purchases.nodes}")
    
    # Find product categories through relationships
    print("\n2. Product categorization path:")
    category_path = client.find_shortest_path("advanced_graph", "user_1", "category_1")
    if category_path:
        print(f"Path from user to category: {category_path.path}")
    
    # Find strongly connected products
    print("\n3. Strongly connected products:")
    related_products = client.find_related_by_weight("advanced_graph", "product_1", min_weight=0.8)
    if related_products:
        print(f"Products strongly related to product_1: {related_products.related_nodes}")
    
    # Clean up
    client.delete_collection("advanced_graph")

if __name__ == "__main__":
    print("Testing VectorDB Graph Functionality")
    print("=" * 50)
    
    try:
        test_graph_functionality()
        print("\n" + "=" * 50)
        test_advanced_graph_scenarios()
        print("\nAll tests completed successfully!")
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        import traceback
        traceback.print_exc()