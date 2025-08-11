# jerry_vectorDB

Hello,

This is my first time writing a DB, to be specific, a vector database that AI agents or future robots can possibily use some kind of DB like this. Just by thinking about this future is already exciting. I am still a C++ noob, but I feel like I should give it a try even though it could be hard, pointless, or feeling void. But even void has a pointer (void*), so I am heading to some direction.  


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

