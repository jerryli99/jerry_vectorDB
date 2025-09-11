// #include <iostream>
// #include <thread>
// #include <vector>
// #include <atomic>
// #include <chrono>
// #include <string>
// #include <sstream>
// #include <queue>
// #include <mutex>
// #include <condition_variable>
// #include <random>

// using namespace std;

// // Simple thread-safe queue for vector data
// template<typename T>
// class VectorConcurrentQueue {
// private:
//     queue<vector<T>> q;
//     mutable mutex mtx;
//     condition_variable cv;
//     atomic<size_t> total_elements{0};
//     atomic<size_t> total_vectors{0};
//     atomic<bool> stop_flag{false};
    
// public:
//     void enqueue(vector<T> item) {
//         lock_guard<mutex> lock(mtx);
//         if (stop_flag) return; // Don't enqueue if stopped
//         q.push(move(item));
//         total_elements += item.size();
//         total_vectors++;
//         cv.notify_one();
//     }
    
//     bool try_dequeue(vector<T>& item) {
//         lock_guard<mutex> lock(mtx);
//         if (q.empty()) return false;
//         item = move(q.front());
//         total_elements -= item.size();
//         total_vectors--;
//         q.pop();
//         return true;
//     }
    
//     size_t size() const {
//         lock_guard<mutex> lock(mtx);
//         return q.size();
//     }
    
//     size_t total_elements_count() const {
//         return total_elements.load();
//     }
    
//     size_t total_vectors_count() const {
//         return total_vectors.load();
//     }
    
//     bool wait_dequeue_bulk(vector<vector<T>>& items, size_t max_count, chrono::milliseconds timeout = chrono::milliseconds(100)) {
//         unique_lock<mutex> lock(mtx);
        
//         // Wait for data or timeout
//         if (cv.wait_for(lock, timeout, [this] { return !q.empty() || stop_flag; })) {
//             if (stop_flag) return false; // Stop requested
            
//             while (!q.empty() && items.size() < max_count) {
//                 items.push_back(move(q.front()));
//                 total_elements -= q.front().size();
//                 total_vectors--;
//                 q.pop();
//             }
//             return true;
//         }
//         return false;
//     }
    
//     void stop() {
//         lock_guard<mutex> lock(mtx);
//         stop_flag = true;
//         cv.notify_all(); // Wake up all waiting threads
//     }
    
//     bool is_stopped() const {
//         return stop_flag.load();
//     }
// };

// class VectorBatchProcessor {
// private:
//     VectorConcurrentQueue<float> queue;
//     vector<thread> producer_threads;
//     vector<thread> consumer_threads;
//     const size_t vector_size;
//     const size_t batch_size;
//     const int num_producers;
//     const int num_consumers;
//     atomic<size_t> total_produced{0};
//     atomic<size_t> total_consumed{0};
//     atomic<bool> stop_requested{false};

// public:
//     VectorBatchProcessor(size_t vec_size = 500, size_t batch_size = 100, 
//                         int num_producers = 4, int num_consumers = 2)
//         : vector_size(vec_size), batch_size(batch_size), 
//           num_producers(num_producers), num_consumers(num_consumers) {}

//     ~VectorBatchProcessor() {
//         stop();
//     }

//     void start() {
//         cout << "Starting Vector Processing System" << endl;
//         cout << "Vector size: " << vector_size << " floats" << endl;
//         cout << "Batch size: " << batch_size << " vectors" << endl;
//         cout << "Producers: " << num_producers << " (simulating HTTP clients)" << endl;
//         cout << "Consumers: " << num_consumers << " (worker threads)" << endl;
//         cout << "==============================================" << endl;

//         stop_requested = false;

//         // Start producers (simulating HTTP clients)
//         for (int i = 0; i < num_producers; ++i) {
//             producer_threads.emplace_back(&VectorBatchProcessor::http_client_simulator, this, i);
//         }

//         // Start consumers (worker threads)
//         for (int i = 0; i < num_consumers; ++i) {
//             consumer_threads.emplace_back(&VectorBatchProcessor::vector_processor, this, i);
//         }
//     }

//     void stop() {
//         if (stop_requested) return;
        
//         cout << "\nRequesting stop for all threads..." << endl;
//         stop_requested = true;
        
//         // Stop the queue first to wake up all waiting threads
//         queue.stop();
        
//         // Wait for producers
//         for (auto& thread : producer_threads) {
//             if (thread.joinable()) thread.join();
//         }
        
//         // Give some time to process remaining data
//         this_thread::sleep_for(chrono::seconds(1));
        
//         // Wait for consumers
//         for (auto& thread : consumer_threads) {
//             if (thread.joinable()) thread.join();
//         }
        
//         print_stats();
//     }

//     void print_stats() const {
//         cout << "\n=== FINAL STATISTICS ===" << endl;
//         cout << "Total vectors produced: " << total_produced.load() << endl;
//         cout << "Total vectors consumed: " << total_consumed.load() << endl;
//         cout << "Vectors in queue: " << queue.total_vectors_count() << endl;
//         cout << "Elements in queue: " << queue.total_elements_count() << endl;
//         cout << "Queue size: " << queue.size() << endl;
//     }

// private:
//     vector<float> generate_random_vector(size_t size) {
//         static random_device rd;
//         static mt19937 gen(rd());
//         static uniform_real_distribution<float> dist(0.0f, 1.0f);
        
//         vector<float> vec;
//         vec.reserve(size);
//         for (size_t i = 0; i < size; ++i) {
//             vec.push_back(dist(gen));
//         }
//         return vec;
//     }

//     void http_client_simulator(int client_id) {
//         int request_count = 0;
//         random_device rd;
//         mt19937 gen(rd());
//         uniform_int_distribution<int> delay_dist(5, 20); // ms delay between requests
        
//         cout << "HTTP Client " << client_id << " started" << endl;
        
//         while (!stop_requested && !queue.is_stopped()) {
//             // BACKPRESSURE: Slow down if queue is too big
//             if (queue.size() > 20000) {
//                 this_thread::sleep_for(chrono::milliseconds(50));
//                 continue;
//             }
//             // Simulate HTTP request processing time
//             this_thread::sleep_for(chrono::milliseconds(delay_dist(gen)));
            
//             if (stop_requested || queue.is_stopped()) break;
            
//             // Generate vector data (simulating received HTTP data)
//             auto vector_data = generate_random_vector(vector_size);
//             queue.enqueue(move(vector_data));
//             total_produced++;
//             request_count++;
            
//             // Log every 10 requests
//             if (request_count % 10 == 0) {
//                 cout << "Client " << client_id << " sent request #" << request_count 
//                      << " (queue: " << queue.size() << " vectors)" << endl;
//             }
//         }
//         cout << "HTTP Client " << client_id << " stopped (requests: " << request_count << ")" << endl;
//     }

//     void vector_processor(int worker_id) {
//         vector<vector<float>> batch;
//         batch.reserve(batch_size);
//         size_t total_processed = 0;
        
//         cout << "Worker " << worker_id << " started" << endl;
        
//         while (!stop_requested || queue.total_vectors_count() > 0) {
//             batch.clear();
            
//             if (queue.wait_dequeue_bulk(batch, batch_size, chrono::milliseconds(1))) {
//                 process_vector_batch(worker_id, batch);
//                 total_processed += batch.size();
//                 total_consumed += batch.size();
//             }
            
//             // Early exit if stop requested and queue is empty or stopped
//             if ((stop_requested || queue.is_stopped()) && queue.total_vectors_count() == 0) {
//                 break;
//             }
//         }
//         cout << "Worker " << worker_id << " stopped (processed: " << total_processed << " vectors)" << endl;
//     }

//     void process_vector_batch(int worker_id, const vector<vector<float>>& batch) {
//         size_t total_elements = 0;
//         for (const auto& vec : batch) {
//             total_elements += vec.size();
//         }
        
//         cout << "\n[Worker " << worker_id << "] Processing batch of " << batch.size() 
//              << " vectors (" << total_elements << " elements)" << endl;
        
//         // Show sample data from first vector
//         if (!batch.empty()) {
//             const auto& sample_vec = batch[0];
//             cout << "  Sample vector [0]: [";
//             for (size_t i = 0; i < min(size_t(5), sample_vec.size()); ++i) {
//                 cout << sample_vec[i];
//                 if (i < min(size_t(4), sample_vec.size() - 1)) cout << ", ";
//             }
//             if (sample_vec.size() > 5) cout << ", ...";
//             cout << "]" << endl;
//         }
        
//         // Perform some computation on the vectors
//         float batch_sum = 0.0f;
//         size_t batch_count = 0;
        
//         for (const auto& vec : batch) {
//             for (float value : vec) {
//                 batch_sum += value;
//                 batch_count++;
//             }
//         }
        
//         cout << "  Batch statistics: sum=" << batch_sum 
//              << ", avg=" << (batch_count > 0 ? batch_sum / batch_count : 0.0f)
//              << ", vectors=" << batch.size() << endl;
        
//         // Simulate processing time (e.g., ML inference, database insert, etc.)
//         this_thread::sleep_for(chrono::milliseconds(100 + batch.size() * 2));
//     }
// };

// int main() {
//     cout << "Vector Processing System Demo" << endl;
//     cout << "Simulating concurrent HTTP vector data ingestion" << endl;
//     cout << "==============================================" << endl;

//     // Configuration - adjust based on your hardware
//     VectorBatchProcessor processor(
//         500,    // vector size (500 floats)
//         5000,     // batch size (50 vectors per batch)
//         2,      // producers (HTTP clients)
//         2       // consumers (worker threads)
//     );
    
//     processor.start();
    
//     // Run for 10 seconds
//     this_thread::sleep_for(chrono::seconds(10));
    
//     processor.stop();
    
//     cout << "\nDemo completed successfully!" << endl;
    
//     return 0;
// }



#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std;

// Double Buffer class for efficient batch collection
template<typename T>
class DoubleBuffer {
private:
    vector<vector<T>> front_buffer;
    vector<vector<T>> back_buffer;
    mutable mutex buffer_mutex;
    atomic<size_t> front_buffer_size{0};
    atomic<size_t> total_vectors{0};
    const size_t max_batch_size;
    const chrono::milliseconds max_wait_time;

public:
    DoubleBuffer(size_t batch_size = 1000, chrono::milliseconds wait_time = chrono::milliseconds(100))
        : max_batch_size(batch_size), max_wait_time(wait_time) {
        front_buffer.reserve(batch_size);
        back_buffer.reserve(batch_size);
    }

    // Add vector to front buffer (thread-safe)
    void add_vector(vector<T> vector) {
        lock_guard<mutex> lock(buffer_mutex);
        front_buffer.push_back(move(vector));
        front_buffer_size = front_buffer.size();
        total_vectors++;
    }

    // Swap buffers and return batch if ready (thread-safe)
    bool try_get_batch(vector<vector<T>>& output_batch) {
        lock_guard<mutex> lock(buffer_mutex);
        
        if (front_buffer.empty()) {
            return false;
        }

        // Check if we should swap based on size or time
        if (front_buffer.size() >= max_batch_size) {
            swap(front_buffer, back_buffer);
            front_buffer.clear();
            front_buffer_size = 0;
            output_batch = move(back_buffer);
            back_buffer.clear();
            return true;
        }
        
        return false;
    }

    // Force get batch even if not full (for timeout)
    bool force_get_batch(vector<vector<T>>& output_batch) {
        lock_guard<mutex> lock(buffer_mutex);
        
        if (front_buffer.empty()) {
            return false;
        }

        swap(front_buffer, back_buffer);
        front_buffer.clear();
        front_buffer_size = 0;
        output_batch = move(back_buffer);
        back_buffer.clear();
        return true;
    }

    size_t current_size() const {
        return front_buffer_size.load();
    }

    size_t total_vectors_count() const {
        return total_vectors.load();
    }

    bool is_empty() const {
        return front_buffer_size.load() == 0;
    }
};

// Simple thread-safe queue for passing batches between threads
template<typename T>
class BatchConcurrentQueue {
private:
    queue<vector<vector<T>>> q;
    mutable mutex mtx;
    condition_variable cv;
    atomic<bool> stop_flag{false};
    
public:
    void enqueue_batch(vector<vector<T>> batch) {
        lock_guard<mutex> lock(mtx);
        if (stop_flag) return;
        q.push(move(batch));
        cv.notify_one();
    }
    
    bool try_dequeue_batch(vector<vector<T>>& batch) {
        lock_guard<mutex> lock(mtx);
        if (q.empty()) return false;
        batch = move(q.front());
        q.pop();
        return true;
    }
    
    bool wait_dequeue_batch(vector<vector<T>>& batch, chrono::milliseconds timeout = chrono::milliseconds(100)) {
        unique_lock<mutex> lock(mtx);
        
        if (cv.wait_for(lock, timeout, [this] { return !q.empty() || stop_flag; })) {
            if (stop_flag) return false;
            batch = move(q.front());
            q.pop();
            return true;
        }
        return false;
    }
    
    size_t size() const {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
    
    void stop() {
        lock_guard<mutex> lock(mtx);
        stop_flag = true;
        cv.notify_all();
    }
    
    bool is_stopped() const {
        return stop_flag.load();
    }
};

class VectorBatchProcessor {
private:
    DoubleBuffer<float> double_buffer;
    BatchConcurrentQueue<float> batch_queue;
    vector<thread> producer_threads;
    vector<thread> consumer_threads;
    vector<thread> batcher_threads;
    const size_t vector_size;
    const size_t batch_size;
    const int num_producers;
    const int num_consumers;
    atomic<size_t> total_produced{0};
    atomic<size_t> total_consumed{0};
    atomic<bool> stop_requested{false};

public:
    VectorBatchProcessor(size_t vec_size = 500, size_t batch_size = 100, 
                        int num_producers = 4, int num_consumers = 2)
        : vector_size(vec_size), batch_size(batch_size), 
          num_producers(num_producers), num_consumers(num_consumers),
          double_buffer(batch_size, chrono::milliseconds(50)) {}

    ~VectorBatchProcessor() {
        stop();
    }

    void start() {
        cout << "Starting Vector Processing System with Double Buffering" << endl;
        cout << "Vector size: " << vector_size << " floats" << endl;
        cout << "Batch size: " << batch_size << " vectors" << endl;
        cout << "Producers: " << num_producers << " (HTTP clients)" << endl;
        cout << "Batchers: 1 (buffer manager)" << endl;
        cout << "Consumers: " << num_consumers << " (worker threads)" << endl;
        cout << "==============================================" << endl;

        stop_requested = false;

        // Start producers (HTTP clients)
        for (int i = 0; i < num_producers; ++i) {
            producer_threads.emplace_back(&VectorBatchProcessor::http_client_simulator, this, i);
        }

        // Start batcher thread (manages double buffer)
        batcher_threads.emplace_back(&VectorBatchProcessor::batcher_manager, this);

        // Start consumers (worker threads)
        for (int i = 0; i < num_consumers; ++i) {
            consumer_threads.emplace_back(&VectorBatchProcessor::batch_processor, this, i);
        }
    }

    void stop() {
        if (stop_requested) return;
        
        cout << "\nRequesting stop for all threads..." << endl;
        stop_requested = true;
        
        // Stop the batch queue first
        batch_queue.stop();
        
        // Wait for producers
        for (auto& thread : producer_threads) {
            if (thread.joinable()) thread.join();
        }
        
        // Wait for batcher
        for (auto& thread : batcher_threads) {
            if (thread.joinable()) thread.join();
        }
        
        // Process any remaining data
        this_thread::sleep_for(chrono::seconds(1));
        
        // Wait for consumers
        for (auto& thread : consumer_threads) {
            if (thread.joinable()) thread.join();
        }
        
        print_stats();
    }

    void print_stats() const {
        cout << "\n=== FINAL STATISTICS ===" << endl;
        cout << "Total vectors produced: " << total_produced.load() << endl;
        cout << "Total vectors consumed: " << total_consumed.load() << endl;
        cout << "Vectors in buffer: " << double_buffer.current_size() << endl;
        cout << "Batches in queue: " << batch_queue.size() << endl;
    }

private:
    vector<float> generate_random_vector(size_t size) {
        static random_device rd;
        static mt19937 gen(rd());
        static uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        vector<float> vec;
        vec.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            vec.push_back(dist(gen));
        }
        return vec;
    }

    void http_client_simulator(int client_id) {
        int request_count = 0;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> delay_dist(5, 20);
        
        cout << "HTTP Client " << client_id << " started" << endl;
        
        while (!stop_requested) {
            // BACKPRESSURE: Slow down if buffer is getting full
            if (double_buffer.current_size() > batch_size * 2) {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }
            
            this_thread::sleep_for(chrono::milliseconds(delay_dist(gen)));
            
            if (stop_requested) break;
            
            // Generate and add vector to double buffer
            auto vector_data = generate_random_vector(vector_size);
            double_buffer.add_vector(move(vector_data));
            total_produced++;
            request_count++;
            
            if (request_count % 50 == 0) {
                cout << "Client " << client_id << " sent #" << request_count 
                     << " (buffer: " << double_buffer.current_size() << "/" << batch_size << ")" << endl;
            }
        }
        cout << "HTTP Client " << client_id << " stopped (requests: " << request_count << ")" << endl;
    }

    void batcher_manager() {
        cout << "Batcher Manager started" << endl;
        auto last_batch_time = chrono::steady_clock::now();
        
        while (!stop_requested || !double_buffer.is_empty()) {
            vector<vector<float>> batch;
            
            // Try to get a batch based on size
            if (double_buffer.try_get_batch(batch)) {
                batch_queue.enqueue_batch(move(batch));
                last_batch_time = chrono::steady_clock::now();
            }
            else {
                // Check if we should force a batch due to timeout
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_batch_time);
                
                if (elapsed > chrono::milliseconds(100) && !double_buffer.is_empty()) {
                    if (double_buffer.force_get_batch(batch)) {
                        batch_queue.enqueue_batch(move(batch));
                        last_batch_time = now;
                    }
                }
            }
            
            this_thread::sleep_for(chrono::milliseconds(5));
            
            if (stop_requested && double_buffer.is_empty()) {
                break;
            }
        }
        cout << "Batcher Manager stopped" << endl;
    }

    void batch_processor(int worker_id) {
        vector<vector<float>> batch;
        size_t total_processed = 0;
        
        cout << "Worker " << worker_id << " started" << endl;
        
        while (!stop_requested || batch_queue.size() > 0) {
            if (batch_queue.wait_dequeue_batch(batch, chrono::milliseconds(10))) {
                process_batch(worker_id, batch);
                total_processed += batch.size();
                total_consumed += batch.size();
                batch.clear();
            }
            
            if (stop_requested && batch_queue.size() == 0) {
                break;
            }
        }
        cout << "Worker " << worker_id << " stopped (processed: " << total_processed << " vectors)" << endl;
    }

    void process_batch(int worker_id, const vector<vector<float>>& batch) {
        size_t total_elements = 0;
        for (const auto& vec : batch) {
            total_elements += vec.size();
        }
        
        cout << "\n[Worker " << worker_id << "] Processing batch of " << batch.size() 
             << " vectors (" << total_elements << " elements)" << endl;
        
        // Show sample data
        if (!batch.empty()) {
            const auto& sample_vec = batch[0];
            cout << "  Sample: [";
            for (size_t i = 0; i < min(size_t(3), sample_vec.size()); ++i) {
                cout << sample_vec[i];
                if (i < min(size_t(2), sample_vec.size() - 1)) cout << ", ";
            }
            if (sample_vec.size() > 3) cout << ", ...";
            cout << "]" << endl;
        }
        
        // Perform computation
        float batch_sum = 0.0f;
        size_t batch_count = 0;
        
        for (const auto& vec : batch) {
            for (float value : vec) {
                batch_sum += value;
                batch_count++;
            }
        }
        
        cout << "  Stats: sum=" << batch_sum 
             << ", avg=" << (batch_count > 0 ? batch_sum / batch_count : 0.0f)
             << ", vectors=" << batch.size() << endl;
        
        // Simulate processing time
        this_thread::sleep_for(chrono::milliseconds(100 + batch.size() * 1));
    }
};

int main() {
    cout << "Vector Processing System with Double Buffering Demo" << endl;
    cout << "==============================================" << endl;

    VectorBatchProcessor processor(
        500,    // vector size
        1000,   // batch size (LARGER batches now!)
        1,      // producers
        4       // consumers
    );
    
    processor.start();
    
    // Run for 15 seconds to see batching in action
    this_thread::sleep_for(chrono::seconds(15));
    
    processor.stop();
    
    cout << "\nDemo completed successfully!" << endl;
    
    return 0;
}