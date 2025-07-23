#include <iostream>
#include <typeinfo>
#include <Eigen/Dense>

int main() {
    Eigen::VectorXf vec(3);
    vec << 1.0f, 2.0f, 3.0f;
    std::cout << "Type of vec: " << typeid(vec).name() << std::endl;
    // Check the type of data() pointer
    auto ptr = vec.data();
    std::cout << "Type of vec.data(): " << typeid(ptr).name() << std::endl;

    // If you want to dereference it
    std::cout << "Type of *vec.data(): " << typeid(*ptr).name() << std::endl;

    return 0;
}

