/*
extensions for stl
*/

#include<string>
#include<vector>
#include<memory>
#include<utility>
#include<codecvt>
#include<locale>
#include<functional>
#include<fstream>
#include<future>
#include<deque>
#include<list>

namespace eSTL {

	template<class T>
	class vector :public std::vector<T>
	{
	public:
		size_t sizeInBytes() const
		{
			return (this->size() * sizeof(T));
		}
	};

	class ofstream :public std::ofstream
	{
	public:
		template<typename T>
		size_t write(vector<T>& containner)
		{
			return this->write(containner.data(), containner.sizeInBytes());
		}

		template<typename T>
		size_t write(std::list<T>& containner)
		{
			size_t s = 0;
			while (auto &v:containner)
				s += this->write((char*)&v, sizeof(T));
			return s;
		}
	};

	const std::string to_string(const std::wstring& str)
	{
		std::locale sys_loc("");
		mbstate_t out_cvt_state;

		const wchar_t* src_wstr = str.c_str();
		const size_t MAX_UNICODE_BYTES = 4;
		const size_t BUFFER_SIZE =
			str.size() * MAX_UNICODE_BYTES + 1;

		char* extern_buffer = new char[BUFFER_SIZE];
		memset(extern_buffer, 0, BUFFER_SIZE);

		const wchar_t* intern_from = src_wstr;
		const wchar_t* intern_from_end = intern_from + str.size();
		const wchar_t* intern_from_next = 0;
		char* extern_to = extern_buffer;
		char* extern_to_end = extern_to + BUFFER_SIZE;
		char* extern_to_next = 0;

		typedef std::codecvt<wchar_t, char, mbstate_t> CodecvtFacet;

		CodecvtFacet::result cvt_rst =
			std::use_facet<CodecvtFacet>(sys_loc).out(
				out_cvt_state,
				intern_from, intern_from_end, intern_from_next,
				extern_to, extern_to_end, extern_to_next);
		if (cvt_rst != CodecvtFacet::ok) {
			switch (cvt_rst) {
			case CodecvtFacet::partial:
				std::cerr << "partial";
				break;
			case CodecvtFacet::error:
				std::cerr << "error";
				break;
			case CodecvtFacet::noconv:
				std::cerr << "noconv";
				break;
			default:
				std::cerr << "unknown";
			}
			std::cerr << ", please check out_cvt_state."
				<< std::endl;
		}
		std::string result = extern_buffer;

		delete[]extern_buffer;

		return result;
	}

	const std::wstring to_wstring(const std::string& str)
	{
		std::locale sys_loc("");
		mbstate_t in_cvt_state;

		const char* src_str = str.c_str();
		const size_t BUFFER_SIZE = str.size() + 1;

		wchar_t* intern_buffer = new wchar_t[BUFFER_SIZE];
		wmemset(intern_buffer, 0, BUFFER_SIZE);

		const char* extern_from = src_str;
		const char* extern_from_end = extern_from + str.size();
		const char* extern_from_next = 0;
		wchar_t* intern_to = intern_buffer;
		wchar_t* intern_to_end = intern_to + BUFFER_SIZE;
		wchar_t* intern_to_next = 0;

		typedef std::codecvt<wchar_t, char, mbstate_t> CodecvtFacet;

		CodecvtFacet::result cvt_rst =
			std::use_facet<CodecvtFacet>(sys_loc).in(
				in_cvt_state,
				extern_from, extern_from_end, extern_from_next,
				intern_to, intern_to_end, intern_to_next);
		if (cvt_rst != CodecvtFacet::ok) {
			switch (cvt_rst) {
			case CodecvtFacet::partial:
				std::cerr << "partial";
				break;
			case CodecvtFacet::error:
				std::cerr << "error";
				break;
			case CodecvtFacet::noconv:
				std::cerr << "noconv";
				break;
			default:
				std::cerr << "unknown";
			}
			std::cerr << ", please check in_cvt_state."
				<< std::endl;
		}
		std::wstring result = intern_buffer;

		delete[]intern_buffer;

		return result;
	}

}
