#include "AllenThreadPool.hpp"
#include<iostream>
using namespace std;
using namespace std::chrono;

int main()
{
	AllenThreadPool a(4);

	auto job = []() {
		this_thread::sleep_for(1s);
	};

	int RunTimes = 8;

	auto StartTime = steady_clock::now();
	for (int i = 0; i < RunTimes; i++)
		job();
	auto cost = steady_clock::now() - StartTime;
	cout << "single thread finished cost " << duration_cast<milliseconds>(cost).count() << "ms" << endl;

	StartTime = steady_clock::now();
	a.AddJob(job, RunTimes);
	a.Wait();
	cost = steady_clock::now() - StartTime;
	cout << "thread pool finished cost " << duration_cast<milliseconds>(cost).count() << "ms" << endl;

	return 0;
}