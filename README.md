# jerry_vectorDB
<!-- 
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

probably use gRPC: so i write a c++ server, and then have clients in Go, Python, Rust, Java etc... 

For configuration, maybe use TOML files just include the toml.hpp header..?

-->
