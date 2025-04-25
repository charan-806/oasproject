#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <limits>
#include <numeric> // For accumulate

using namespace std;
using namespace std::chrono;

class Task {
public:
    int id;
    int priority;       // Higher value = higher priority
    int burst_time;     // in milliseconds
    int deadline;       // in milliseconds (relative to start time)
    bool is_completed;
    system_clock::time_point arrival_time;

    Task(int id, int priority, int burst_time, int deadline)
        : id(id), priority(priority), burst_time(burst_time),
          deadline(deadline), is_completed(false), arrival_time(system_clock::now()) {}

    // For priority queue ordering based on deadline (EDF)
    bool operator<(const Task& other) const {
        return deadline > other.deadline; // Min-heap based on deadline
    }
};

class PowerModel {
public:
    static double calculate_power(int frequency, double utilization) {
        const double P_static = 0.2;
        const double C = 1e-8;
        double voltage = 0.5 + (frequency / 2000.0) * 0.5;
        double P_dynamic = C * voltage * voltage * frequency * utilization;
        return P_static + P_dynamic;
    }
};

class EnergyEfficientScheduler {
private:
    vector<Task> tasks;
    int current_frequency; // MHz
    double total_energy;
    system_clock::time_point start_time;
    vector<double> energy_data;
    vector<double> time_points;

public:
    EnergyEfficientScheduler()
        : current_frequency(1000), total_energy(0.0), start_time(system_clock::now()) {}

    void add_task(const Task& task) {
        tasks.push_back(task);
    }

    void adjust_frequency(int new_freq) {
        if (new_freq >= 500 && new_freq <= 2000) {
            current_frequency = new_freq;
            cout << "Adjusted CPU frequency to " << current_frequency << " MHz" << endl;
        }
    }

    void run_edf_with_dvfs() {
        start_time = system_clock::now();
        priority_queue<Task> ready_queue;

        // Copy tasks to the priority queue
        for (const auto& task : tasks) {
            ready_queue.push(task);
        }

        while (!ready_queue.empty()) {
            Task current_task = ready_queue.top();
            ready_queue.pop();

            double utilization = calculate_system_utilization();
            int optimal_freq = calculate_optimal_frequency(current_task, utilization);
            adjust_frequency(optimal_freq);

            cout << "Executing Task " << current_task.id
                 << " (Priority: " << current_task.priority
                 << ", Burst: " << current_task.burst_time << "ms"
                 << ", Deadline: " << current_task.deadline << "ms)"
                 << " at " << current_frequency << " MHz" << endl;

            double time_ratio = min(1.0, static_cast<double>(current_task.burst_time) / current_task.deadline);
            double task_power = PowerModel::calculate_power(current_frequency, time_ratio);

            // Simulate execution time (convert ms to seconds for energy calculation)
            double execution_time = current_task.burst_time / 1000.0;
            double task_energy = task_power * execution_time;
            total_energy += task_energy;

            // Record energy and time for plotting
            energy_data.push_back(total_energy);
            auto now = system_clock::now();
            duration<double> elapsed = now - start_time;
            time_points.push_back(elapsed.count());

            // Simulate execution (we'll just sleep for a short time for demonstration)
            this_thread::sleep_for(milliseconds(100));

            cout << "Completed Task " << current_task.id
                 << ". Energy used: " << fixed << setprecision(6) << task_energy << " J" << endl;
        }

        cout << "\nTotal energy consumed: " << fixed << setprecision(6) << total_energy << " J" << endl;
        plot_energy_consumption();
    }

    double calculate_system_utilization() {
        double total_utilization = 0.0;
        for (const auto& task : tasks) {
            if (!task.is_completed) {
                total_utilization += static_cast<double>(task.burst_time) / task.deadline;
            }
        }
        return min(1.0, total_utilization);
    }

    int calculate_optimal_frequency(const Task& task, double utilization) {
        double time_ratio = static_cast<double>(task.burst_time) / task.deadline;
        if (time_ratio < 0.3) {
            return 800;
        } else if (time_ratio < 0.7) {
            return 1200;
        } else {
            return 1800;
        }
    }

    void plot_energy_consumption() {
        // Simple ASCII plot since we don't have matplotlib in C++
        cout << "\nEnergy Consumption Over Time:\n";
        cout << "Time (s)\tEnergy (J)\n";
        cout << "----------------------------\n";

        for (size_t i = 0; i < time_points.size(); ++i) {
            cout << fixed << setprecision(2) << time_points[i] << "\t\t"
                 << fixed << setprecision(6) << energy_data[i] << endl;
        }

        // Find max energy for scaling
        double max_energy = *max_element(energy_data.begin(), energy_data.end());
        double max_time = time_points.empty() ? 0.0 : time_points.back();

        // Simple ASCII bar chart
        cout << "\nSimple ASCII Chart:\n";
        const int width = 50;

        for (size_t i = 0; i < time_points.size(); ++i) {
            int bar_length = static_cast<int>((energy_data[i] / max_energy) * width);
            cout << fixed << setprecision(2) << time_points[i] << "s |";
            for (int j = 0; j < bar_length; ++j) {
                cout << "#";
            }
            cout << " " << fixed << setprecision(6) << energy_data[i] << " J\n";
        }
    }
};

int main() {
    EnergyEfficientScheduler scheduler;
    int num_tasks;

    cout << "Energy-Efficient CPU Scheduler Simulation\n";
    cout << "Enter number of tasks: ";
    while (!(cin >> num_tasks) || num_tasks <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Please enter a positive integer: ";
    }

    for (int i = 0; i < num_tasks; ++i) {
        int task_id = i + 1;
        int priority, burst, deadline;

        cout << "\nTask " << task_id << " parameters:\n";
        cout << "  Enter priority (1-10): ";
        while (!(cin >> priority) || priority < 1 || priority > 10) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter an integer between 1 and 10: ";
        }

        cout << "  Enter burst time (ms): ";
        while (!(cin >> burst) || burst <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a positive integer: ";
        }

        cout << "  Enter deadline (ms): ";
        while (!(cin >> deadline) || deadline <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a positive integer: ";
        }

        scheduler.add_task(Task(task_id, priority, burst, deadline));
    }

    cout << "\nStarting simulation...\n";
    scheduler.run_edf_with_dvfs();

    return 0;
}
