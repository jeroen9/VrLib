#include "ServerConnection.h"

#include <VrLib/Log.h>
#include <VrLib/json.h>
#include <Gl/glew.h>
#include <ctime>

namespace vrlib
{
	const unsigned char* renderer;
	ServerConnection::ServerConnection() : running(true), backgroundThread(&ServerConnection::thread, this)
	{
		s = 0;

		callbacks["session/start"] = [](const json::Value &) { };
		renderer = glGetString(GL_RENDERER);
		logger << (char*)renderer << Log::newline;
	}


	void ServerConnection::thread()
	{
		char hostname[1024];
		gethostname(hostname, 1024);
		char applicationName[MAX_PATH];
		::GetModuleFileName(0, applicationName, MAX_PATH);

		char username[1024];
		DWORD username_len = 1024;
		GetUserName(username, &username_len);


		std::time_t startTime = std::time(nullptr);


		while (running)
		{
			struct sockaddr_in addr;
			struct hostent* host;

			host = gethostbyname(apiHost.c_str());
			if (host == NULL)
			{
				logger << "Could not look up host " << apiHost << "', are you connected to the internet?";
				return;
			}
			addr.sin_family = host->h_addrtype;
			memcpy((char*)&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
			addr.sin_port = htons(apiPort);
			memset(addr.sin_zero, 0, 8);

			if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				logger << "Cannot create socket, try a reboot" << Log::newline;
				closesocket(s);
				Sleep(60000);
				continue;
			}

			int rc;
			int siz = sizeof(addr);
			rc = ::connect(s, (struct sockaddr*) &addr, siz);
			if (rc < 0)
			{
				logger << "Could not connect to api host: " << apiHost << Log::newline;
				closesocket(s);
				Sleep(1000);
				continue;
			}

			logger << "Connected to remote API" << Log::newline;

			json::Value packet;
			packet["id"] = "session/start";
			packet["data"]["host"] = hostname;
			packet["data"]["file"] = applicationName;
			packet["data"]["renderer"] = std::string((char*)renderer);
			packet["data"]["starttime"] = startTime;
			packet["data"]["user"] = username;
			send(packet);

			std::string buffer;
			char buf[1024];
			while (running && s != 0)
			{
				int rc = recv(s, buf, 1024, 0);
				if (rc < 0)
				{
					closesocket(s);
					s = 0;
					break;
				}
				buffer += std::string(buf, rc);
				while (buffer.size() > 4)
				{
					unsigned int len = *((unsigned int*)&buffer[0]);
					if (buffer.size() >= len + 4)
					{
						json::Value data = json::readJson(buffer.substr(4, len));
						buffer = buffer.substr(4 + len);

						if (!data.isMember("id"))
						{
							logger << "Invalid packet from server" << Log::newline;
							logger << data << Log::newline;
							closesocket(s);
							s = 0;
							break;
						}

						if (callbacks.find(data["id"]) != callbacks.end())
							callbacks[data["id"]](data);
						else if (singleCallbacks.find(data["id"]) != singleCallbacks.end())
						{
							singleCallbacks[data["id"]](data);
							singleCallbacks.erase(singleCallbacks.find(data["id"]));
						}
						else
						{
							logger << "Invalid packet from server" << Log::newline;
							logger << data << Log::newline;
							closesocket(s);
							s = 0;
							break;
						}
					}
					else
						break;
				}
			}
			logger << "Disconnected...." << Log::newline;
		}
	}

	void ServerConnection::send(const json::Value &value)
	{
		std::string data;
		data << value;
		unsigned int len = data.size();
		int rc = ::send(s, (char*)&len, 4, 0);
		if (rc < 0)
		{
			closesocket(s);
			s = 0;
			return;
		}
		::send(s, data.c_str(), data.size(), 0);
		if (rc < 0)
		{
			closesocket(s);
			s = 0;
			return;
		}
	}


	void ServerConnection::callBackOnce(const std::string &action, std::function<void(const json::Value &)> callback)
	{
		singleCallbacks[action] = callback;
	}

	json::Value ServerConnection::call(const std::string &action, const json::Value& data)
	{
		json::Value result;
		bool done = false;
		callBackOnce(action, [&done, &result](const vrlib::json::Value &data)
		{
			result = data["data"];
			done = true;
		});
		json::Value packet;
		packet["id"] = action;
		packet["data"] = data;
		send(packet);
		while (!done)
			Sleep(2);
		return result;
	}

	void ServerConnection::update(double frameTime)
	{
	}


	void ServerConnection::sendFps(float fps)
	{
		json::Value v;
		v["id"] = "session/report";
		v["data"]["fps"] = fps;
		send(v);
	}

}