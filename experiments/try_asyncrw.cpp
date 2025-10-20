#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <future>
#include <chrono>

struct ObjA {
    int value;
    ObjA(int v) : value(v) {}
};

// 模拟 SmartArray
class SmartArray {
public:
    SmartArray(size_t capacity) : m_capacity(capacity) {}

    void append(ObjA obj) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.push_back(obj);
        if (m_buffer.size() >= m_capacity) {
            auto batch = std::move(m_buffer);
            m_buffer.clear();
            asyncWrite(batch);
        }
    }

    void flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_buffer.empty()) {
            auto batch = std::move(m_buffer);
            m_buffer.clear();
            asyncWrite(batch);
        }
    }

    std::vector<int> readAllAsync() {
        std::vector<std::future<std::vector<int>>> futures;
        {
            std::lock_guard<std::mutex> lock(m_file_mutex);
            for (auto& fname : m_file_paths) {
                // async read
                futures.push_back(std::async(std::launch::async, [fname]() {
                    std::ifstream fin(fname);
                    std::vector<int> v;
                    int x;
                    while (fin >> x) v.push_back(x);
                    return v;
                }));
            }
        }

        //merge results
        std::vector<int> result;
        for (auto& f : futures) {
            auto batch = f.get();
            result.insert(result.end(), batch.begin(), batch.end());
        }
        return result;
    }

private:
    void asyncWrite(std::vector<ObjA> batch) {
        std::thread([this, batch=std::move(batch)]() mutable {
            static int file_id = 0;
            std::string fname = "batch_" + std::to_string(file_id++) + ".txt";
            {
                std::lock_guard<std::mutex> lock(m_file_mutex);
                m_file_paths.push_back(fname);
            }
            std::ofstream fout(fname);
            for (auto& obj : batch) {
                fout << obj.value << "\n";
            }
            std::cout << "[Async write] Wrote " << batch.size() << " objects to " << fname << "\n";
        }).detach();
    }

    size_t m_capacity;
    std::vector<ObjA> m_buffer;
    std::mutex m_mutex;

    std::vector<std::string> m_file_paths;
    std::mutex m_file_mutex;
};

int main() {
    const int N = 10;
    SmartArray smart(N);

    //generate 30 objects
    for (int i = 0; i < 3*N; ++i) {
        smart.append(ObjA(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    smart.flush();//flush the remainings

    std::this_thread::sleep_for(std::chrono::seconds(1)); //wait async to finish

    auto all_data = smart.readAllAsync();

    std::cout << "Read back " << all_data.size() << " integers:\n";
    for (auto v : all_data) std::cout << v << " ";
    std::cout << "\n";

    return 0;
}
