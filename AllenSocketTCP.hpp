//#pragma once
#include <event.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <iostream>
#include <cstdint>
#include <future>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

class AllenSocketTCP
{
#ifdef WIN32
#define INIT_NETWORK	WSADATA wsaData;(void)WSAStartup(MAKEWORD(2, 2), &wsaData)
#define UNINIT_NETWORK	WSACleanup()
#define INIT_THREAD_PRE	evthread_use_windows_threads()
#define NEW_BASE(base)	event_config* evcfg = event_config_new();event_config_set_flag(evcfg,EVENT_BASE_FLAG_STARTUP_IOCP);base = event_base_new_with_config(evcfg);event_config_free(evcfg)
#else
#define INIT_NETWORK
#define UNINIT_NETWORK
#define INIT_THREAD_PRE	evthread_use_pthreads()
#define NEW_BASE(base)	base = event_base_new()
#endif
#define INIT_THREAD(base) INIT_THREAD_PRE;evthread_make_base_notifiable(base)

public:
	enum Mode {
		Mode_None,
		Mode_Serv,
		Mode_Clnt
	};

	// callback's type for server
	enum CallbackServer {
		CS_Received,
		CS_Connected,
		CS_Event
	};
	// callback's type for client
	enum CallbackClient {
		CC_Received
	};

	// events 
	enum SocketEvent {
		SE_None,
		SE_Timeout,
		SE_Error,
		SE_EOF,
		SE_Connected
	};

	// events while
	enum SocketEventWhile {
		SEW_None,
		SEW_Writing,
		SEW_Reading
	};

protected:
	event_base*     m_evBase;
	// for server
	evconnlistener*	m_evListener;
	// for client
	bufferevent*	m_evBuffEvt;

	using CallbackTypeServerConnected	= std::function<void(const char*, unsigned short)>;
	using CallbackTypeServerReceived	= std::function<void(std::vector<char>*)>;
	using CallbackTypeServerEvent		= std::function<void(SocketEvent, SocketEventWhile)>;
	using CallbackTypeClientReceived	= std::function<void(std::vector<char>*)>;

	using CallbackFuncType = std::function<void(void*)>;
	typedef struct struCBServer {
		std::list<CallbackTypeServerConnected> Connected;
		std::list<CallbackTypeServerReceived> Received;
		std::list<CallbackTypeServerEvent> Event;
	}CallbackTypeServer;
	
	typedef struct struCBClient {
		std::list<CallbackTypeClientReceived> Received;
	}CallbackTypeClient;

#define CallbackGetServerConnected(arg) &((CallbackTypeServer*)arg)->Connected
#define CallbackGetServerReceived(arg) &((CallbackTypeServer*)arg)->Received
#define CallbackGetServerEvent(arg) &((CallbackTypeServer*)arg)->Event

#define CallbackGetClientReceived(arg) &((CallbackTypeClient*)arg)->Received


	std::unordered_map<evutil_socket_t, bufferevent*> m_servClientList;

	// callbacks
	CallbackTypeServer m_cbServer;
	CallbackTypeClient m_cbClient;

	// Args
	typedef struct struServerArgs {
		event_base*					base;
		CallbackTypeServer*			callback;
		decltype(m_servClientList)*	clients;
	}ServerArgs;
	typedef struct struClientArgs {
		event_base*					base;
		CallbackTypeClient*			callback;
	}ClientArgs;
	#define ArgsGetEvBase(args,stru) ((stru*)args)->base
	#define ArgsGetCbList(args,stru) ((stru*)args)->callback

#define ServerArgsGetClients(args) (((ServerArgs*)args)->clients)
#define ServerArgsGetEvBase(args) ArgsGetEvBase(args,ServerArgs)
#define ServerArgsGetCbList(args) ArgsGetCbList(args,ServerArgs)
#define ClientArgsGetEvBase(args) ArgsGetEvBase(args,ClientArgs)
#define ClientArgsGetCbList(args) ArgsGetCbList(args,ClientArgs)
	ServerArgs			m_servArgs;
	ClientArgs			m_clntArgs;



	// class work mode
	Mode					m_mode;
	std::thread				m_thrdServDispatch;
	std::thread				m_thrdClntDispatch;

	void init()
	{
		m_evBase = nullptr;
		m_evBuffEvt = nullptr;
		m_evListener = nullptr;
		m_mode = Mode::Mode_None;
	}

public:
	AllenSocketTCP() { init(); }
	virtual ~AllenSocketTCP()
	{
		if (m_evListener)	evconnlistener_free(m_evListener);
		if (m_evBuffEvt)	bufferevent_free(m_evBuffEvt);
		if (m_evBase)
		{
			event_base_loopbreak(m_evBase);
			if (m_mode == Mode::Mode_Serv)
				m_thrdServDispatch.detach();
			if (m_mode == Mode::Mode_Clnt)
				m_thrdClntDispatch.detach();
			event_base_free(m_evBase);
		}
		UNINIT_NETWORK;
	}

protected:
#define INFO(msg,ret) { std::cout << msg << std::endl; return ret; }
	int serverStartServer(uint16_t port)
	{
		if (m_mode != Mode::Mode_None) return -1;
		INIT_NETWORK;
		sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		NEW_BASE(m_evBase);
		if (!m_evBase)
			INFO("event_base create faild",0);
		INIT_THREAD(m_evBase);
		m_servArgs.base = m_evBase;
		m_servArgs.callback = &m_cbServer;
		m_servArgs.clients = &m_servClientList;
		m_evListener = evconnlistener_new_bind(m_evBase, [](
			evconnlistener* listener,
			evutil_socket_t fd,
			sockaddr* sock,
			int socklen,
			void* arg) {

			auto base = ServerArgsGetEvBase(arg);
			if (fd < 0) INFO("accept",);

			bufferevent* evBuffer = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
			bufferevent_setcb(evBuffer, [](bufferevent* in_bev, void* args) {
				std::vector<char> buff;
				const size_t CacheSize = 256;
				char* cache = new char[CacheSize];
				size_t n(0);

				while (n = bufferevent_read(in_bev, cache, CacheSize), n > 0)
					buff.insert(buff.end(), cache, cache + n);
				delete cache;

				auto cb = ServerArgsGetCbList(args);
				auto result = CallbackGetServerReceived(cb);
				if (!result->empty())
					for (auto i : *result)
						i(&buff);
			}, [](bufferevent* bev, void* arg) {
				//std::cout << "send to client" << std::endl;
			}, [](bufferevent* in_bev, short in_event, void* args) {
				SocketEvent			evt			= SocketEvent::SE_None;
				SocketEventWhile	evtWhile	= SocketEventWhile::SEW_None;

				if (in_event & BEV_EVENT_TIMEOUT)			evt = SocketEvent::SE_Timeout;
				else if (in_event & BEV_EVENT_EOF)			evt = SocketEvent::SE_EOF;
				else if (in_event & BEV_EVENT_ERROR)		evt = SocketEvent::SE_Error;
				else if (in_event & BEV_EVENT_CONNECTED)	evt = SocketEvent::SE_Connected;

				if (in_event & BEV_EVENT_READING)			evtWhile = SocketEventWhile::SEW_Reading;
				else if (in_event & BEV_EVENT_WRITING)		evtWhile = SocketEventWhile::SEW_Writing;
				
				if (evt == SocketEvent::SE_EOF || evt == SocketEvent::SE_Error)
					ServerArgsGetClients(args)->erase(bufferevent_getfd(in_bev));

				auto cb = ServerArgsGetCbList(args);
				auto result = CallbackGetServerEvent(cb);
				if (!result->empty())
					for (auto i : *result)
						i(evt, evtWhile);
			}, arg);
			bufferevent_enable(evBuffer, EV_READ | EV_WRITE | EV_PERSIST);
			
			auto cb = ServerArgsGetCbList(arg);
			auto result = CallbackGetServerConnected(cb);
			if (!result->empty())
			{
				char str[INET_ADDRSTRLEN];
				auto p1 = evutil_inet_ntop(AF_INET, &(((sockaddr_in*)sock)->sin_addr), str, INET_ADDRSTRLEN);
				auto p2 = ((sockaddr_in*)sock)->sin_port;
				for (auto i : *result)
					i(p1, p2);
				ServerArgsGetClients(arg)->insert(std::make_pair(fd, evBuffer));
				std::cout << "new connection created and now is connected " << ServerArgsGetClients(arg)->size() << " clients" << std::endl;
			}
		}, (void*)&m_servArgs,
			LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,
			10, (const sockaddr*)&sin, sizeof(sin));

		evconnlistener_set_error_cb(m_evListener, [](evconnlistener* listener, void* args) {
			struct event_base *base = evconnlistener_get_base(listener);
			int err = EVUTIL_SOCKET_ERROR();

			printf("Got an error %d (%s) on the listener. "
				"Shutting down.\n", err, evutil_socket_error_to_string(err));
		});
		m_thrdServDispatch = std::thread([](event_base* base) {event_base_dispatch(base);}, m_evBase);
		m_mode = Mode::Mode_Serv;
		return 0;
	}

	int serverSend(const char* data, size_t dataLen)
	{
		if (m_mode != Mode::Mode_Serv) return -1;
		for(auto i:m_servClientList)
			bufferevent_write(i.second, data, dataLen);
		return 0;
	}

	int clientConnect(const char* ip, uint16_t port)
	{
		if (m_mode != Mode::Mode_None) return -1;
		INIT_NETWORK;
	
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (evutil_inet_pton(AF_INET, ip, &addr.sin_addr) < 0)
			INFO("inet_pton",0);
		NEW_BASE(m_evBase);
		if (m_evBase == NULL)
			INFO("new base error",0);
		INIT_THREAD(m_evBase);
		m_evBuffEvt = bufferevent_socket_new(m_evBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
		if (m_evBuffEvt == NULL)
			INFO("new bufferevent error",0);
		
		int ret = bufferevent_socket_connect(m_evBuffEvt, (sockaddr*)&addr, sizeof(addr));
		if (ret != 0)
			INFO("connect error",0);

		m_clntArgs.base = m_evBase;
		m_clntArgs.callback = &m_cbClient;

		bufferevent_setcb(m_evBuffEvt,
			[](bufferevent* bev, void* args) {

			std::vector<char> buff;
			const size_t CacheSize = 256;
			char* cache = new char[CacheSize];
			size_t n(0);

			while (n = bufferevent_read(bev, cache, CacheSize), n > 0)
				buff.insert(buff.end(), cache, cache + n);
			delete cache;

			auto cb = ClientArgsGetCbList(args);
			auto result = CallbackGetClientReceived(cb);
			if (result->empty()) return;
			for (auto i : *result)
				i(&buff);
		},
			[](bufferevent* bev, void* arg) {
			//std::cout << "send to server" << std::endl;
		},
			[](bufferevent* bev, short events, void* ctx) {
			if (events & BEV_EVENT_CONNECTED)
			{
				std::cout << "Connected" << std::endl;
			}
			else if (events & BEV_EVENT_ERROR)
			{
				std::cout << "Error" << std::endl;
			}
		}, &m_clntArgs);
		bufferevent_enable(m_evBuffEvt, EV_READ | EV_WRITE | EV_PERSIST);
		m_thrdClntDispatch = std::thread([](event_base* base) {event_base_dispatch(base);}, m_evBase);
		m_mode = Mode::Mode_Clnt;
		return 0;
	}
#undef INFO

	int clientSend(const char* data, size_t dataLen)
	{
		if (m_mode != Mode::Mode_Clnt) return -1;
		return bufferevent_write(m_evBuffEvt, data, dataLen);
	}

public:

		void wait()
		{
			if (m_mode == Mode::Mode_Clnt)
				m_thrdClntDispatch.join();
			if (m_mode == Mode::Mode_Serv)
				m_thrdServDispatch.join();
		}

		bool isWorked() { return m_mode != Mode::Mode_None; }

		auto GetNetworkMethod() { return event_base_get_method(m_evBase); }


#undef INIT_NETWORK
};

class AllenSocketTCPServer :public AllenSocketTCP
{
public:
	AllenSocketTCPServer() { }
	AllenSocketTCPServer(uint16_t port) { if (port) serverStartServer(port); }
	int StartServer(uint16_t port) { return serverStartServer(port); }
	int Send(const char* data, size_t dataLen) { return serverSend(data, dataLen); }

	void AddCallbackConnected(CallbackTypeServerConnected cb, bool isAppend = true) {
		if (!isAppend) m_cbServer.Connected.clear(); m_cbServer.Connected.push_back(cb);
	}

	void AddCallbackReceived(CallbackTypeServerReceived cb, bool isAppend = true) {
		if (!isAppend) m_cbServer.Received.clear(); m_cbServer.Received.push_back(cb);
	}

	void AddCallbackEvent(CallbackTypeServerEvent cb, bool isAppend = true) {
		if (!isAppend) m_cbServer.Event.clear(); m_cbServer.Event.push_back(cb);
	}
};

class AllenSocketTCPClient :public AllenSocketTCP
{
public:
	AllenSocketTCPClient() { }
	AllenSocketTCPClient(const char* ip, uint16_t port) { clientConnect(ip, port); }
	int Connect(const char* ip, uint16_t port) { return clientConnect(ip, port); }
	int Send(const char* data, size_t dataLen) { return clientSend(data, dataLen); }

	void AddCallbackReceived(CallbackTypeClientReceived cb, bool isAppend = true) {
		if (!isAppend) m_cbClient.Received.clear(); m_cbClient.Received.push_back(cb);
	}
};