import unittest
from vectordb_client import VectorDBClient, PointStruct, Batch

class TestVectorDBClient(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.client = VectorDBClient()
        cls.collection_name = "test_collection"

    def test_named_vectors_upsert(self):
        points = [
            PointStruct(
                id=1,
                vector={
                    "image": [0.9, 0.1, 0.1, 0.2],
                    "text": [0.4, 0.7, 0.1, 0.8, 0.1, 0.1, 0.9, 0.2],
                },
            ),
            PointStruct(
                id=2,
                vector={
                    "image": [0.2, 0.1, 0.3, 0.9],
                    "text": [0.5, 0.2, 0.7, 0.4, 0.7, 0.2, 0.3, 0.9],
                },
            ),
        ]
        res = self.client.upsert(self.collection_name, points)
        self.assertIsNotNone(res)
        self.assertEqual(res.get("status"), "ok")

    def test_vector_payload_upsert(self):
        points = [
            PointStruct(id=1, vector=[0.9, 0.1, 0.1], payload={"color": "red"}),
            PointStruct(id=2, vector=[0.1, 0.9, 0.1], payload={"color": "green"}),
        ]
        res = self.client.upsert(self.collection_name, points)
        self.assertIsNotNone(res)
        self.assertEqual(res.get("status"), "ok")

    def test_batch_upsert(self):
        batch = Batch(
            ids=[1, 2, 3],
            vectors=[
                [0.9, 0.1, 0.1],
                [0.1, 0.9, 0.1],
                [0.1, 0.1, 0.9],
            ],
            payloads=[
                {"color": "red", "age" : 12},
                {"color": "green"},
                {"color": "blue"},
                {"tags": ["cat", "meme", "2023"],
                "metadata": {
                    "created_at": "2025-08-06T10:30:00Z",
                    "source": {
                        "platform": "twitter",
                        "id": "tweet_123456"
                    },
                    "confidence": 0.991}
                }
            ],
        )
        res = self.client.upsert(self.collection_name, batch)
        self.assertIsNotNone(res)
        self.assertEqual(res.get("status"), "ok")


if __name__ == "__main__":
    unittest.main()
