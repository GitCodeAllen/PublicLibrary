#pragma once
#include<string>
#include<vector>
#include<memory>
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;

#define VARS_TYPE vector<string,string>

class StringVarReplace
{
public:
	StringVarReplace(){}
	StringVarReplace(const char* prefix, const char* suffix)
	{
		m_prefix = prefix;
		m_suffix = suffix;
	}
	~StringVarReplace(){}

	void AddVar(const char* varName, const char* varValue)
	{
		if(!m_vars)
			m_vars = make_shared<VARS_TYPE>();
		m_vars->emplace_back(varName,varValue);
	}

private:
	string m_prefix, m_suffix;
	shared_ptr<VARS_TYPE> m_vars;
};