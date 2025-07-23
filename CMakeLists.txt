# cmake_minimum_required(VERSION 3.14)
# project(vector_db LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Eigen (from system or pip/conda)
# find_package(Eigen3 REQUIRED)
# include_directories(${EIGEN3_INCLUDE_DIR})

# # Fetch pybind11
# include(FetchContent)
# FetchContent_Declare(
#   pybind11
#   GIT_REPOSITORY https://github.com/pybind/pybind11.git
# )

# FetchContent_MakeAvailable(pybind11)

# # Headers
# include_directories(include)

# # Build Python module
# pybind11_add_module(vector_db
#     bindings/pybind_wrapper.cpp
#     src/IndexFlat.cpp
# )

# target_link_libraries(vector_db PRIVATE Eigen3::Eigen)
