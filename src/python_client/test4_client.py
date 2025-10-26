from vectordb_client import VectorDBClient
import vectordb_models as models
import random
import time
import statistics
import numpy as np


def generate_random_vector(dim: int):
    """Generate random vector using numpy for better performance"""
    return [float(x) for x in np.random.random(dim).astype(np.float32)]


def generate_random_vectors_batch(dim: int, count: int):
    """Generate multiple random vectors efficiently"""
    return np.random.random((count, dim)).astype(np.float32).tolist()


def warmup_queries(client, collection_name, dim, num_warmup=10):
    """Warm up the system with some initial queries"""
    print(f"Warming up with {num_warmup} queries...")
    for i in range(num_warmup):
        query_vec = generate_random_vector(dim)
        client.query_points(
            collection_name=collection_name,
            query_vectors=[query_vec],
            using="default",
            top_k=10
        )


def benchmark_max_qps_single_thread(client, collection_name, dim, test_duration=10, top_k=10):
    """Benchmark maximum QPS in a single thread for sustained period"""
    print(f"Single-thread max QPS test for {dim}D (running {test_duration}s)...")
    
    query_count = 0
    latencies = []
    start_time = time.time()
    end_time = start_time + test_duration
    
    # Pre-generate query vectors to avoid generation overhead during timing
    batch_vectors = generate_random_vectors_batch(dim, 1000)
    batch_index = 0
    
    while time.time() < end_time:
        # Use pre-generated vectors, cycle through them
        if batch_index >= len(batch_vectors):
            batch_index = 0
        
        query_start = time.time()
        result = client.query_points(
            collection_name=collection_name,
            query_vectors=[batch_vectors[batch_index]],
            using="default",
            top_k=top_k
        )
        query_end = time.time()
        
        latencies.append((query_end - query_start) * 1000)
        query_count += 1
        batch_index += 1
    
    actual_duration = time.time() - start_time
    qps = query_count / actual_duration
    
    return qps, latencies, query_count


def run_max_qps_tests():
    client = VectorDBClient()

    print("🚀 MAXIMUM QPS PERFORMANCE TEST SUITE")
    print("Setting up high-performance test environment...")
    
    # Test dimensions - realistic production sizes
    test_configs = [
        {"dim": 512, "points": 10000, "name": "512D (Sentence Embeddings)"},
        {"dim": 768, "points": 10000, "name": "768D (BERT-base)"},
        {"dim": 128, "points": 10000, "name": "128D (Image/LLM Embeddings)"},
    ]
    
    collections = {}
    
    # Setup collections with LOTS of data
    for config in test_configs:
        dim = config["dim"]
        collection_name = f"qps_test_{dim}"
        
        print(f"\n📊 Setting up {config['name']} with {config['points']} points...")
        
        cfg = models.CreateCollectionRequest(
            vectors=models.VectorParams(size=dim, distance="Cosine"),
            on_disk="false"
        )
        client.create_collection(collection_name, cfg)
        collections[dim] = collection_name
        
        # Batch insert for performance - do this in chunks
        print("Inserting data...")
        batch_size = 1000
        for batch_start in range(0, config["points"], batch_size):
            batch_end = min(batch_start + batch_size, config["points"])
            points = [
                models.PointStruct(
                    id=f"vec_{dim}_{i}",
                    vector=generate_random_vector(dim),
                    payload={"dim": dim, "batch": batch_start // batch_size, "index": i}
                )
                for i in range(batch_start, batch_end)
            ]
            client.upsert(collection_name, points)
            
            if (batch_start // batch_size) % 5 == 0:
                print(f"  Inserted {batch_end}/{config['points']} points...")
    
    print("\n" + "="*80)
    print("🎯 STARTING MAXIMUM QPS TESTS")
    print("="*80)
    
    # Warm up all collections
    for config in test_configs:
        dim = config["dim"]
        warmup_queries(client, collections[dim], dim, num_warmup=20)
    
    time.sleep(1)  # Let system stabilize
    
    # Test configurations: (test_duration)
    test_durations = [15, 30, 60]  # Test for 15, 30, and 60 seconds
    
    results = {}
    
    for duration in test_durations:
        print(f"\n🔥 TEST DURATION: {duration} seconds")
        print("-" * 50)
        
        for config in test_configs:
            dim = config["dim"]
            collection_name = collections[dim]
            
            qps, latencies, total_queries = benchmark_max_qps_single_thread(
                client, collection_name, dim, duration, top_k=10
            )
            
            # Calculate statistics
            avg_latency = statistics.mean(latencies) if latencies else 0
            p95 = sorted(latencies)[int(0.95 * len(latencies))] if latencies else 0
            p99 = sorted(latencies)[int(0.99 * len(latencies))] if latencies else 0
            
            if dim not in results:
                results[dim] = {}
            results[dim][duration] = {
                'qps': qps,
                'avg_latency': avg_latency,
                'p95_latency': p95,
                'p99_latency': p99,
                'total_queries': total_queries
            }
            
            print(f"  {config['name']:25} | QPS: {qps:7.1f} | Avg: {avg_latency:6.1f}ms | P95: {p95:6.1f}ms | Total: {total_queries} queries")
    
    # Print comprehensive results table
    print("\n" + "="*100)
    print("🎯 FINAL MAXIMUM QPS RESULTS")
    print("="*100)
    print(f"{'Dimension':<25} {'Duration':<10} {'QPS':<12} {'Avg Latency':<12} {'P95 Latency':<12} {'Total Queries':<15}")
    print("-" * 100)
    
    for config in test_configs:
        dim = config["dim"]
        for duration in test_durations:
            if dim in results and duration in results[dim]:
                r = results[dim][duration]
                print(f"{config['name']:<25} {duration:<10} {r['qps']:<12.1f} {r['avg_latency']:<12.1f} {r['p95_latency']:<12.1f} {r['total_queries']:<15}")
        print("-" * 100)
    
    # Find maximum achieved QPS
    max_qps = 0
    best_config = None
    for dim in results:
        for duration in results[dim]:
            if results[dim][duration]['qps'] > max_qps:
                max_qps = results[dim][duration]['qps']
                best_config = (dim, duration)
    
    print(f"\n🏆 MAXIMUM ACHIEVED QPS: {max_qps:.1f} queries/second")
    if best_config:
        print(f"🏆 Best Configuration: {best_config[0]}D with {best_config[1]}s test duration")
    
    # Performance summary by dimension
    print("\n📈 PERFORMANCE SUMMARY BY DIMENSION:")
    for config in test_configs:
        dim = config["dim"]
        if dim in results:
            best_duration = max(results[dim].keys(), key=lambda d: results[dim][d]['qps'])
            best_result = results[dim][best_duration]
            print(f"  {config['name']:25} | Max QPS: {best_result['qps']:7.1f} | Avg Latency: {best_result['avg_latency']:6.1f}ms")
    
    # Test different top_k values
    print("\n🔍 TESTING DIFFERENT TOP_K VALUES (512D):")
    top_k_values = [1, 5, 10, 20, 50]
    test_duration = 10
    
    for top_k in top_k_values:
        qps, latencies, total_queries = benchmark_max_qps_single_thread(
            client, "qps_test_512", 512, test_duration, top_k=top_k
        )
        avg_latency = statistics.mean(latencies) if latencies else 0
        print(f"  top_k={top_k:2d}: {qps:6.1f} QPS, {avg_latency:6.1f}ms avg latency")
    
    print("\n" + "="*80)
    print("💡 PERFORMANCE INSIGHTS:")
    print("  • Single-threaded QPS shows raw database performance")
    print("  • Lower dimensions generally allow higher QPS")
    print("  • Longer test durations help identify performance consistency")
    print("  • Smaller top_k values typically yield higher QPS")
    print("="*80)


if __name__ == "__main__":
    # Use current time for random seed but keep it consistent per run
    random.seed(int(time.time()))
    np.random.seed(int(time.time()))
    
    run_max_qps_tests()