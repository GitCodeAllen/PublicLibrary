#pragma once
#include<chrono>
#include<list>
#include<vector>
#include<string>
#include<map>
#include<thread>
#include<mutex>
#include<fstream>
#include<iomanip>
#include<debugapi.h>

using namespace std;
using namespace chrono;

void ThreadProc(void* param);

class CPerformanceTrace{
public:
	CPerformanceTrace(const char* PerformanceTracerName = "")
	{
		m_Info["TracerName"] = PerformanceTracerName;
		AddCount();
		//m_WriteThread = thread(ThreadProc,this);
	}
	~CPerformanceTrace()
	{
	}

	enum TimeUnit{
		TU_Nano,
		TU_Micro,
		TU_Milli,
		TU_Sec,
		TU_Min,
		TU_Hour
	};

	enum ErrorCode{
		PTEC_TimeEmpty = -100,
		PTEC_UnhandledTimeUnit
	};

	void AddCount(){ m_CountedTime.emplace_back(steady_clock::now());}
	void Reset(){ m_CountedTime.clear();}
	void ResetWithCount(){ Reset(); m_CountedTime.emplace_back(steady_clock::now());}
	void WriteThread();
	
	__int64 GetTime(TimeUnit unit = TimeUnit::TU_Milli);

	template<typename T>
	__int64 GetTimeCount(T duration, TimeUnit unit = TimeUnit::TU_Milli);
	void DisplayTime(string str, TimeUnit uint = TimeUnit::TU_Milli);

private:
	list<decltype(steady_clock::now())> m_CountedTime;
	map<string,string> m_Info;
	list<string> m_write;
	mutex m_mutex;
	thread m_WriteThread;
};

__int64 CPerformanceTrace::GetTime(TimeUnit unit)
{
	if(m_CountedTime.size() == 0)
		return ErrorCode::PTEC_TimeEmpty;
	auto diff = steady_clock::now() - m_CountedTime.back();
	switch(unit)
	{
	case TimeUnit::TU_Nano:		return duration_cast<nanoseconds>(diff).count();
	case TimeUnit::TU_Micro:	return duration_cast<microseconds>(diff).count();
	case TimeUnit::TU_Milli:	return duration_cast<milliseconds>(diff).count();
	case TimeUnit::TU_Sec:		return duration_cast<seconds>(diff).count();
	case TimeUnit::TU_Min:		return duration_cast<minutes>(diff).count();
	case TimeUnit::TU_Hour:		return duration_cast<hours>(diff).count();
	default:					return ErrorCode::PTEC_UnhandledTimeUnit;
	}
}

template<typename T>
__int64 CPerformanceTrace::GetTimeCount(T duration, TimeUnit unit)
{
	switch(unit)
	{
	case TimeUnit::TU_Nano:		return duration_cast<nanoseconds>(duration).count();
	case TimeUnit::TU_Micro:	return duration_cast<microseconds>(duration).count();
	case TimeUnit::TU_Milli:	return duration_cast<milliseconds>(duration).count();
	case TimeUnit::TU_Sec:		return duration_cast<seconds>(duration).count();
	case TimeUnit::TU_Min:		return duration_cast<minutes>(duration).count();
	case TimeUnit::TU_Hour:		return duration_cast<hours>(duration).count();
	default:					return ErrorCode::PTEC_UnhandledTimeUnit;
	}
}

void CPerformanceTrace::DisplayTime(string str, TimeUnit unit)
{
	if(m_CountedTime.size() == 0)
		return;
	AddCount();
	auto *first = &m_CountedTime.front();
	stringstream ss;
	for(auto& i : m_CountedTime)
	{
		if(first == &i)	continue;
		auto diff = i - *first;
		auto result = GetTimeCount(diff,unit);
		if(!ss.str().empty()) ss << ",";
		if(result < 0)
			ss << "0";
		else
			ss << setw(3) << result;
		first = &i;
	}
	m_Info["time"] = ss.str();
	m_Info["total"] = to_string(GetTimeCount(m_CountedTime.back() - m_CountedTime.front()));

	/*ofstream out((string("D:\\") + m_Info["TracerName"] + ".txt"),ios::app);
	out << m_Info["total"] << endl;
	out.close();*/

	string prefix("{"), suffix("}");
	while(1)
	{
		string::size_type pos = string::size_type(0);
		auto prefix_pos = str.find_first_of(prefix, pos);
		if(prefix_pos == string::npos) break;
		auto suffix_pos = str.find_first_of(suffix, pos);
		if(suffix_pos == string::npos) break;
		string keyword(str,prefix_pos + prefix.size(),suffix_pos - prefix_pos - suffix.size());
		auto result = m_Info.find(keyword);
		if(result != m_Info.end())
			str.replace(prefix_pos,suffix_pos - prefix_pos + suffix.size(), m_Info[keyword]);
		pos = suffix_pos + suffix.size();
	}
	OutputDebugStringA((str+"\n").c_str());
}

void CPerformanceTrace::WriteThread()
{
	ofstream out((string("D:\\") + m_Info["TracerName"] + ".txt"));
	while(1)
	{
		m_mutex.lock();
		while(!m_write.empty())
		{
			out << m_write.front() << endl;
			m_write.pop_front();
		}
		m_mutex.unlock();
		Sleep(30);
	}
}

void ThreadProc(void* param)
{
	((CPerformanceTrace*)param)->WriteThread();
}