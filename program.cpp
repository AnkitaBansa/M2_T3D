#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

mutex mutex1;
condition_variable producer_cv, consumer_cv;

int num_producers = 1; 
int num_consumers = 2;
int hour_indicator = 48; 

int producer_count = 0; 
int consumer_count = 0; 
int total_rows = 0;

vector<int> index_vector;
vector<int> traffic_light;
vector<int> car_counts;
vector<string> timestamps;

struct TrafficSignal {
    int row_index;
    string time_stamp;
    int light_id;
    int car_count;
};

TrafficSignal traffic_signals[4] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0}};
queue<TrafficSignal> traffic_queue;

bool sort_method(TrafficSignal first, TrafficSignal second) {
    return first.car_count > second.car_count;
}

void producer_function() {
    string file_name = "timestamp.txt"; 
    
    while (true) {
        ifstream input_file(file_name);
        if (!input_file.is_open()) {
            cout << "Could not open file, please try again." << endl;
            return;
        }

        string line;
        getline(input_file, line);
        
        while (getline(input_file, line)) {
            istringstream ss(line);
            string token;

            getline(ss, token, ','); 
            int row_index = stoi(token);
            getline(ss, token, ','); 
            string time_stamp = token;
            getline(ss, token, ','); 
            int light_id = stoi(token);
            getline(ss, token, '\n'); 
            int car_count = stoi(token);

            if (row_index > producer_count) {
                unique_lock<mutex> lock(mutex1);
                traffic_queue.push({row_index, time_stamp, light_id, car_count});
                producer_cv.notify_all(); 
                producer_count = row_index;
                lock.unlock();
            }
        }
        input_file.close();
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Simulate real-time data acquisition
    }
}

void consumer_function() {
    int hour_counter = 0; 
    while (true) {
        unique_lock<mutex> lock(mutex1); 

        if (!traffic_queue.empty()) {
            TrafficSignal signal = traffic_queue.front();
            traffic_queue.pop(); 
            
            if (consumer_count % hour_indicator == 0) {
                for (int i = 0; i < 4; ++i) {
                    traffic_signals[i].car_count = 0;
                }
                cout << "Hour " << ++hour_counter << " started." << endl;
            }

            for (int i = 0; i < 4; ++i) {
                if (signal.light_id == traffic_signals[i].light_id) {
                    traffic_signals[i].car_count = signal.car_count; 
                    break;
                }
            }

            consumer_count++;
        } else {
            consumer_cv.wait(lock, []{ return !traffic_queue.empty(); }); 
        }

        if (consumer_count % hour_indicator == 0 && consumer_count > 0) { 
            sort(traffic_signals, traffic_signals + 4, sort_method); 
            auto now = chrono::system_clock::now(); // Changed to system_clock
            auto now_c = chrono::system_clock::to_time_t(now); // Changed to system_clock
            cout << "Time: " << put_time(localtime(&now_c), "%T") << endl; // Print current time
            cout << "Traffic signals arranged on the basis of urgency | Time: " << timestamps[consumer_count-1] << endl; 
            cout << "------Traffic Light-------\t\t-----Number of Cars-----" << endl;
            for (int i = 0; i < 4; ++i) {
                cout << "\t" << traffic_signals[i].light_id << "\t\t\t\t\t" << traffic_signals[i].car_count << endl;
            }
        }
        
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
    }
}

void get_traffic_data() {
    string file_name = "timestamp.txt"; 
    ifstream input_file(file_name);

    if (input_file.is_open()) {
        string line;
        getline(input_file, line);

        while (getline(input_file, line)) {
            istringstream ss(line);
            string token;

            getline(ss, token, ','); 
            index_vector.push_back(stoi(token));
            getline(ss, token, ','); 
            timestamps.push_back(token); 
            getline(ss, token, ','); 
            traffic_light.push_back(stoi(token));
            getline(ss, token, '\n'); 
            car_counts.push_back(stoi(token));

            total_rows++;
        }
        input_file.close();
    } else {
        cout << "Could not open file, please try again." << endl;
    }
}

int main() {
    auto start_time = chrono::steady_clock::now(); // Record the start time

    get_traffic_data();
    
    thread producer_thread(producer_function);
    thread consumers[num_consumers]; 

    for (int i = 0; i < num_consumers; ++i) {
        consumers[i] = thread(consumer_function);
    }

    producer_thread.join();
    for (int i = 0; i < num_consumers; ++i) {
        consumers[i].join();
    }

    auto end_time = chrono::steady_clock::now(); // Record the end time
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time); // Calculate the duration in milliseconds

    cout << "Execution time: " << duration.count() << " milliseconds" << endl; // Print the execution time

    return 0;
}
