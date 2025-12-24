#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

// Структура джобы
struct Job {
    int id;
    string command;
    vector<int> dependencies;
    string mutex_name;
    bool completed = false;
    bool running = false;
    bool failed = false;
};

// Класс для управления мьютексами
class MutexManager {
private:
    map<string, mutex> mutexes;
    mutex manager_mutex;

public:
    void register_mutex(const string& name) {
        lock_guard<mutex> lock(manager_mutex);
        mutexes[name];
    }

    void lock(const string& name) {
        mutexes[name].lock();
    }

    void unlock(const string& name) {
        mutexes[name].unlock();
    }

    bool exists(const string& name) {
        lock_guard<mutex> lock(manager_mutex);
        return mutexes.find(name) != mutexes.end();
    }
};

// Класс планировщика DAG
class DAGScheduler {
private:
    map<int, Job> jobs;
    map<int, vector<int>> adjacency_list;
    map<int, vector<int>> reverse_adj_list;
    int max_parallel;
    atomic<int> running_count{0};
    atomic<bool> error_occurred{false};
    MutexManager mutex_manager;
    
    mutex scheduler_mutex;
    mutex jobs_state_mutex;  // Защита для изменения состояния джоб
    condition_variable cv;
    vector<thread> worker_threads;

    // Проверка на циклы (DFS)
    bool has_cycle() {
        map<int, int> color; // 0-белый, 1-серый, 2-черный
        for (auto& [id, job] : jobs) {
            color[id] = 0;
        }

        function<bool(int)> dfs = [&](int node) -> bool {
            color[node] = 1;
            for (int neighbor : adjacency_list[node]) {
                if (color[neighbor] == 1) return true;
                if (color[neighbor] == 0 && dfs(neighbor)) return true;
            }
            color[node] = 2;
            return false;
        };

        for (auto& [id, job] : jobs) {
            if (color[id] == 0 && dfs(id)) {
                return true;
            }
        }
        return false;
    }

    // Проверка связности (BFS)
    bool is_connected() {
        if (jobs.empty()) return true;
        
        set<int> visited;
        queue<int> q;
        int start = jobs.begin()->first;
        q.push(start);
        visited.insert(start);

        // Создаём неориентированный граф
        map<int, set<int>> undirected;
        for (auto& [id, job] : jobs) {
            for (int dep : job.dependencies) {
                undirected[id].insert(dep);
                undirected[dep].insert(id);
            }
        }

        while (!q.empty()) {
            int node = q.front();
            q.pop();
            for (int neighbor : undirected[node]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }

        return visited.size() == jobs.size();
    }

    // Выполнение джобы
    bool execute_job(Job& job) {
        cout << "[START] Job " << job.id << ": " << job.command << endl;
        
        int result = system(job.command.c_str());
        
        if (result != 0) {
            cerr << "[ERROR] Job " << job.id << " failed with code " << result << endl;
            return false;
        }
        
        cout << "[DONE] Job " << job.id << endl;
        return true;
    }

    // Проверка готовности джобы к выполнению (без lock - должен вызываться с захваченным jobs_state_mutex)
    bool is_ready_unlocked(const Job& job) {
        for (int dep_id : job.dependencies) {
            if (!jobs[dep_id].completed || jobs[dep_id].failed) {
                return false;
            }
        }
        return true;
    }
    
    // Проверка готовности джобы к выполнению (с lock)
    bool is_ready(const Job& job) {
        lock_guard<mutex> lock(jobs_state_mutex);
        return is_ready_unlocked(job);
    }

    // Запуск джобы в отдельном потоке
    void run_job(int job_id) {
        Job& job = jobs[job_id];
        
        {
            lock_guard<mutex> lock(jobs_state_mutex);
            job.running = true;
        }
        
        running_count++;

        bool success = false;
        
        // Захват мьютекса если указан
        if (!job.mutex_name.empty()) {
            mutex_manager.lock(job.mutex_name);
        }

        try {
            success = execute_job(job);
        } catch (...) {
            success = false;
        }

        // Освобождение мьютекса
        if (!job.mutex_name.empty()) {
            mutex_manager.unlock(job.mutex_name);
        }

        {
            lock_guard<mutex> lock(jobs_state_mutex);
            job.running = false;
            job.completed = true;
            
            if (!success) {
                job.failed = true;
                error_occurred = true;
                cerr << "[FATAL] Stopping DAG execution due to job " << job_id << " failure" << endl;
            }
        }

        running_count--;
        cv.notify_all();
    }

public:
    DAGScheduler(int max_par) : max_parallel(max_par) {}
    
    // Запрет копирования (содержит mutex, atomic)
    DAGScheduler(const DAGScheduler&) = delete;
    DAGScheduler& operator=(const DAGScheduler&) = delete;

    void add_job(const Job& job) {
        jobs[job.id] = job;
        for (int dep : job.dependencies) {
            adjacency_list[dep].push_back(job.id);
            reverse_adj_list[job.id].push_back(dep);
        }
    }

    void register_mutex(const string& name) {
        mutex_manager.register_mutex(name);
    }

    bool validate() {
        // Проверка на циклы
        if (has_cycle()) {
            cerr << "[VALIDATION ERROR] DAG contains cycles" << endl;
            return false;
        }

        // Проверка связности
        if (!is_connected()) {
            cerr << "[VALIDATION ERROR] DAG is not connected (multiple components)" << endl;
            return false;
        }

        // Проверка стартовых джоб
        vector<int> start_jobs;
        for (auto& [id, job] : jobs) {
            if (job.dependencies.empty()) {
                start_jobs.push_back(id);
            }
        }
        if (start_jobs.empty()) {
            cerr << "[VALIDATION ERROR] No start jobs found" << endl;
            return false;
        }

        // Проверка завершающих джоб
        vector<int> end_jobs;
        for (auto& [id, job] : jobs) {
            if (adjacency_list[id].empty()) {
                end_jobs.push_back(id);
            }
        }
        if (end_jobs.empty()) {
            cerr << "[VALIDATION ERROR] No end jobs found" << endl;
            return false;
        }

        // Проверка мьютексов
        for (auto& [id, job] : jobs) {
            if (!job.mutex_name.empty() && !mutex_manager.exists(job.mutex_name)) {
                cerr << "[VALIDATION ERROR] Mutex '" << job.mutex_name 
                     << "' in job " << id << " is not registered" << endl;
                return false;
            }
        }

        cout << "[VALIDATION OK] Start jobs: ";
        for (int id : start_jobs) cout << id << " ";
        cout << "\n[VALIDATION OK] End jobs: ";
        for (int id : end_jobs) cout << id << " ";
        cout << endl;

        return true;
    }

    void execute() {
        cout << "[SCHEDULER] Starting DAG execution (max_parallel=" << max_parallel << ")" << endl;

        while (true) {
            if (error_occurred) {
                // Ждём завершения всех запущенных джоб
                unique_lock<mutex> lock(scheduler_mutex);
                cv.wait(lock, [this] { return running_count == 0; });
                
                cerr << "[SCHEDULER] Execution stopped due to error" << endl;
                break;
            }

            // Ищем джобы готовые к запуску
            vector<int> ready_jobs;
            {
                lock_guard<mutex> lock(jobs_state_mutex);
                for (auto& [id, job] : jobs) {
                    if (!job.completed && !job.running && is_ready_unlocked(job)) {
                        ready_jobs.push_back(id);
                    }
                }
            }

            // Запускаем джобы с учётом лимита параллелизма
            for (int job_id : ready_jobs) {
                unique_lock<mutex> lock(scheduler_mutex);
                cv.wait(lock, [this] { 
                    return running_count < max_parallel || error_occurred; 
                });

                if (error_occurred) break;

                worker_threads.emplace_back([this, job_id] { run_job(job_id); });
            }

            // Проверка завершения всех джоб
            bool all_done = true;
            {
                lock_guard<mutex> lock(jobs_state_mutex);
                for (auto& [id, job] : jobs) {
                    if (!job.completed) {
                        all_done = false;
                        break;
                    }
                }
            }

            if (all_done) {
                cout << "[SCHEDULER] All jobs completed successfully" << endl;
                break;
            }

            if (ready_jobs.empty() && running_count == 0 && !all_done) {
                cerr << "[SCHEDULER ERROR] Deadlock detected - no jobs can run" << endl;
                break;
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }

        // Ждём завершения всех потоков
        for (auto& t : worker_threads) {
            if (t.joinable()) t.join();
        }
    }
};

// Загрузка конфига из JSON
unique_ptr<DAGScheduler> load_from_json(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Cannot open config file: " + filename);
    }

    json config;
    file >> config;

    int max_parallel = config["max_parallel"];
    auto scheduler = make_unique<DAGScheduler>(max_parallel);

    // Регистрация мьютексов
    if (config.contains("mutexes")) {
        for (const auto& mutex_name : config["mutexes"]) {
            scheduler->register_mutex(mutex_name);
        }
    }

    // Загрузка джоб
    for (const auto& j : config["jobs"]) {
        Job job;
        job.id = j["id"];
        job.command = j["command"];
        
        if (j.contains("dependencies")) {
            job.dependencies = j["dependencies"].get<vector<int>>();
        }
        
        if (j.contains("mutex") && !j["mutex"].is_null()) {
            job.mutex_name = j["mutex"];
        }

        scheduler->add_job(job);
    }

    return scheduler;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <config.json>" << endl;
        return 1;
    }

    try {
        auto scheduler = load_from_json(argv[1]);

        if (!scheduler->validate()) {
            return 1;
        }

        scheduler->execute();

    } catch (const exception& e) {
        cerr << "[FATAL ERROR] " << e.what() << endl;
        return 1;
    }

    return 0;
}
