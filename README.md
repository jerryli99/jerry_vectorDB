# jerry_vectorDB

Hello,

This is my first time writing a DB, to be specific, a vector database that AI agents or future robots can possibily use some kind of DB like this. Just by thinking about this future is already exciting. I know some C++ and I felt like I should give it a try even though it could be hard, pointless, or feeling void. But even void has a pointer (void*), so I am heading to some direction.  

The current DB only supports Dense Vectors, so to be continued with Sparse Vectors...

## VectorDB architechture
I know it is not perfect, but hopefully I can improve it -.-

![Architecture Diagram](jerry_vectordb_design.drawio.png)

## API Schema

### upsert an array of this:
```
(single vector per point)
{
  "id": "22s3",
  "vector": [0.1, 0.2, 0.3, 0.4],
  "payload": { "label": "cat" }
}
```

### or upsert an array of this:
```
(multi vector per point)
{
  "id": "img_1",
  "vector": {
    "image": [0.1, 0.2, 0.3],
    "text": [0.3, 0.6, 0.7],
    ...
  },
  "payload": { "type": "image+text" }
}
```

### For Querying (in python client of course)
Note: This is for the "default" named vector in the DB. 
```
client.query_points(
    collection_name="{collection_name}",
    query_vectors=[[0.2, 0.1, 0.9, 0.7]], # <--- Single Dense vector
    top_k=3 #<---default is 5, this is optional
)

or 

client.query_points(
    collection_name="{collection_name}",
    query_pointids=["43cf51e2-8777-4f52-bc74-c2cbde0c8b04"], # <--- single point id
)

or with batch query

client.query_points(
  collection_name="{collection_name}",
  query_vectors=[vector1, vector2, ...], (note -->one or more, error if none)  
  using="default",
  top_k=10,
)

//this one i think will just return the vector itself
client.query_points(
  collection_name="{collection_name}",
  query_pointids=["id1", "id2", ...], --> one or more, error if none
  using="default",
)

And the result will look something like this:
{
  "result": [
    { "id": "abcd", "score": 0.81 },
    { "id": "herwewf", "score": 0.75 },
    { "id": "qwer34wff-we", "score": 0.73 }
  ],
  "status": "ok",
  "time": 0.001
}

or this

{
  "result": [
    [
        { "id": "sdafr", "score": 0.81 },
        { "id": "qrt3f-ewf", "score": 0.75 },
        { "id": "qwe-qwer3-4-q", "score": 0.73 }
    ],
    [
        { "id": "dfgh-e-h", "score": 0.92 },
        { "id": "5-tf-wer-t4", "score": 0.89 },
        { "id": "ert-5-tw-erg-w", "score": 0.75 }
    ]
  ],
  "status": "ok",
  "time": 0.001
}
```

### Query with filters 

The using specifier here can make the user specify which named vector.
```

```
## TODO: (as of 10/30/2025, to be updated)
Need to write tests for each component. <br>
Need to improve performance, possibly add GPU stuff. <br>
Add user interface such as Qt desktop.<br>
Might need to redesign the payload store and filtermatrix. <br>
Could try to deploy to cloud.<br>

<hr>

Below is just some libs i used for this project.

```
//source: https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl-download.html?operatingsystem=linux&linux-install=apt

sudo apt update

sudo apt install -y gpg-agent wget

wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null

echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" | sudo tee /etc/apt/sources.list.d/oneAPI.list

sudo apt update

sudo apt install intel-oneapi-mkl

sudo apt install intel-oneapi-mkl-devel

----------------------------------------------
sudo apt install libeigen3-dev

sudo apt-get install nlohmann-json3-dev

sudo apt install librocksdb-dev

sudo apt-get install libblas-dev liblapack-dev

sudo apt-get install swig

sudo apt-get install uuid-dev

git clone https://github.com/facebookresearch/faiss.git
cd faiss
cmake -B build -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF .
make -C build -j faiss
sudo make -C build install

//installing grpc is just not working for me because protobuf versions mismatch, even though 
//I followed every instruction and watching old youtube videos or trying chatgpt stuff, spending
//3 days and a lot of cmake build hours, still not working, so i have to move one to other alternatives.

//----Yeah, ignore protobuf now---------
//install protobuf, git clone lastest, 
//cd in project, mkdir build && cd build
//then cmake -Dprotobuf_BUILD_TESTS=OFF .. because there is some weird test error in their cmakefile
//then do "make"
//then either sudo make install or make install
//-------------------------------------


The library for C++ standards https://github.com/abseil/abseil-cpp/tree/master is quite interesting to study the fundamentals in C++17

i have not tried to use the boost lib yet

https://capnproto.org/install.html for installing Cap'n Proto, better than protobuf


For configuration, uhm
```

