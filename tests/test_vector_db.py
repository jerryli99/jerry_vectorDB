import numpy as np
import vector_db

dim = 5
index = vector_db.BruteForceIndex(dim)

# Add random vectors
np.random.seed(42)
for _ in range(100):
    index.add(np.random.rand(dim).astype(np.float32))

# Print all vectors
vectors = index.get_all()
for i, v in enumerate(vectors):
    print(f"Vec[{i}] = {v}")

# Query
query = np.random.rand(dim).astype(np.float32)
top_k_indices = index.search(query, 5)

print("Query:", query)
print("Top 5 nearest indices:", top_k_indices)
print("Top 5 nearest vectors:")
for i in top_k_indices:
    vec = index.get_vector(i)
    print(f"Index {i}: {vec}")