from vectordb_client import VectorDBClient
import vectordb_models as models

def run_tests():
    client = VectorDBClient()

    # Create collection with a single unnamed vector config
    single_config = models.CreateCollectionRequest(
        vectors=models.VectorParams(size=4, distance="Cosine")
    )
    print("Test 1: Create single-vector collection")
    print(client.create_collection("single_collection", single_config))

    # Create collection with multiple named vectors
    multi_config = models.CreateCollectionRequest(
        vectors={
            "image": models.VectorParams(size=4, distance="Dot"),
            "text": models.VectorParams(size=5, distance="Cosine"),
        }
    )
    print("Test 2: Create multi-vector collection")
    print(client.create_collection("multi_collection", multi_config))

    # Upsert with list of PointStruct
    points_list = [
        models.PointStruct(id=1, vector=[0.1, 0.2, 0.3, 0.4], payload={"label": "cat"}),
        models.PointStruct(id=2, vector=[0.5, 0.6, 0.7, 0.8], payload={"label": "dog"}),
    ]
    print("Test 3: Upsert with list of PointStruct")
    print(client.upsert("single_collection", points_list))

    # Upsert with Batch
    batch = models.Batch(
        ids=[3, 4],
        vectors=[[0.9, 1.0, 1.1, 1.2], [1.3, 1.4, 1.5, 1.6]],
        payloads=[{"label": "bird"}, {"label": "fish"}],
    )
    print("Test 4: Upsert with Batch")
    print(client.upsert("multi_collection", batch))

    # Delete collections
    print("Test 5: Delete single_collection")
    print(client.delete_collection("single_collection"))

    print("Test 6: Delete multi_collection")
    print(client.delete_collection("multi_collection"))


if __name__ == "__main__":
    run_tests()



# import unittest
# from vectordb_client import VectorDBClient
# from vectordb_models import PointStruct, Batch, VectorParams, CreateCollectionRequest

# class TestVectorDBClient(unittest.TestCase):
#     @classmethod
#     def setUpClass(cls):
#         cls.client = VectorDBClient()
#         cls.collection_name = "test_collection"

#     def test_01_create_collection(self):
#         config = CreateCollectionRequest(
#             vectors=VectorParams(size=4, distance="Cosine", on_disk=True)
#         )
#         res = self.client.create_collection(self.collection_name, config)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

#     def test_02_upsert_points(self):
#         points = [
#             PointStruct(id="1", vector=[0.1, 0.2, 0.3, 0.4], payload={"color": "red"}),
#             PointStruct(id="2", vector=[0.5, 0.6, 0.7, 0.8], payload={"color": "blue"})
#         ]
#         res = self.client.upsert(self.collection_name, points)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

#     def test_03_delete_collection(self):
#         res = self.client.delete_collection(self.collection_name)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

# if __name__ == "__main__":
#     unittest.main()



# import unittest
# from vectordb_client import VectorDBClient, PointStruct, Batch

# class TestVectorDBClient(unittest.TestCase):
#     @classmethod
#     def setUpClass(cls):
#         cls.client = VectorDBClient()
#         cls.collection_name = "test_collection"

#     def test_named_vectors_upsert(self):
#         points = [
#             PointStruct(
#                 id=1,
#                 vector={
#                     "image": [0.9, 0.1, 0.1, 0.2],
#                     "text": [0.4, 0.7, 0.1, 0.8, 0.1, 0.1, 0.9, 0.2],
#                 },
#                 payload={"hello" : 123}
#             ),
#             PointStruct(
#                 id=2,
#                 vector={
#                     "image": [0.2, 0.1, 0.3, 0.9],
#                     "text": [0.5, 0.2, 0.7, 0.4, 0.7, 0.2, 0.3, 0.9],
#                 },
#             ),
#         ]
#         res = self.client.upsert(self.collection_name, points)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

#     def test_vector_payload_upsert(self):
#         points = [
#             PointStruct(id=1, vector=[0.9, 0.1, 0.1], payload={"color": "red"}),
#             PointStruct(id=2, vector=[0.1, 0.9, 0.1], payload={"color": "green"}),
#         ]
#         res = self.client.upsert(self.collection_name, points)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

#     def test_batch_upsert(self):
#         batch = Batch(
#             ids=[1, 2, 3],
#             vectors=[
#                 [0.9, 0.1, 0.1],
#                 [0.1, 0.9, 0.1],
#                 [0.1, 0.1, 0.9],
#             ],
#             payloads=[
#                 {"color": "red", "age" : 12},
#                 {"color": "green"},
#                 {"color": "blue"},
#                 {"tags": ["cat", "meme", "2023"],
#                 "metadata": {
#                     "created_at": "2025-08-06T10:30:00Z",
#                     "source": {
#                         "platform": "twitter",
#                         "id": "tweet_123456"
#                     },
#                     "confidence": 0.991}
#                 }
#             ],
#         )
#         res = self.client.upsert(self.collection_name, batch)
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")

#     def test_simple_point_upsert(self):
#         point = PointStruct(
#             id=129,
#             vector=[0.1, 0.2, 0.3, 0.4],
#             payload={"color": "red"}
#         )
#         res = self.client.upsert(self.collection_name, [point])
#         self.assertIsNotNone(res)
#         self.assertEqual(res.get("status"), "ok")
        
# if __name__ == "__main__":
#     unittest.main()
