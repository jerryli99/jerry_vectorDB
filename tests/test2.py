import vector_db
import numpy as np

# Single vector addition
index = vector_db.IndexFlat(128)
vec = np.random.rand(128).astype('float32')
index.add(vec)

# Batch addition
vectors = np.random.rand(100, 128).astype('float32')
index.add_batch(vectors)

# Search
query = np.random.rand(128).astype('float32')
ids = index.search(query, k=5)
print(ids)