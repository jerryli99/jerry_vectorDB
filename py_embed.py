import numpy as np
import vector_db

db = vector_db.VectorDB()

# Generate random vectors (float32 for optimal performance)
vec1 = np.random.rand(5).astype(np.float32)
vec2 = np.random.rand(5).astype(np.float32)

print("vec1", vec1)
print("vec2", vec2)

# Add to database (zero-copy transfer)
db.add_vector(vec1)
db.add_vector(vec2)

# Perform operations
sum_result = db.add_vectors(0, 1)
diff_result = db.subtract_vectors(0, 1)

print("Vector addition result:", sum_result)
print("Vector subtraction result:", diff_result)