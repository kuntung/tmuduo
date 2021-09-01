#include <tmuduo/base/Logging.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/TcpServer.h>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <stdio.h>

using namespace tmuduo;
using namespace tmuduo::net;
const int kBufSize = 64*1024;
const char* g_file = NULL;

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
  LOG_INFO << "HighWaterMark " << len;
}


class FileTransferServer
{
public:
	typedef boost::shared_ptr<FILE> FilePtr;
	FileTransferServer(EventLoop* loop, InetAddress& listenAddr)
	  : loop_(loop),
		server_(loop_, listenAddr, "FileTransferServer")
{
	server_.setConnectionCallback(
		boost::bind(&FileTransferServer::onConnection, this, _1));
	server_.setWriteCompleteCallback(
		boost::bind(&FileTransferServer::onWriteComplete, this, _1));
}

void start()
{
	server_.start();
}
private:
		void onConnection(const TcpConnectionPtr& conn)
		{
		  LOG_INFO << "FileServer - " << conn->peerAddress().toIpPort() << " -> "
				   << conn->localAddress().toIpPort() << " is "
				   << (conn->connected() ? "UP" : "DOWN");
		  if (conn->connected())
		  {
			LOG_INFO << "FileServer - Sending file " << g_file
					 << " to " << conn->peerAddress().toIpPort();
			conn->setHighWaterMarkCallback(onHighWaterMark, kBufSize+1);

			FILE* fp = ::fopen(g_file, "rb");
			if (fp)
			{
			  FilePtr ctx(fp, ::fclose);
			  conn->setContext(ctx);
			  char buf[kBufSize];
			  size_t nread = ::fread(buf, 1, sizeof buf, fp);
			  conn->send(buf, nread);
			}
			else
			{
			  conn->shutdown();
			  LOG_INFO << "FileServer - no such file";
			}
		  }
		}

		void onWriteComplete(const TcpConnectionPtr& conn)
		{
		  const FilePtr& fp = boost::any_cast<const FilePtr&>(conn->getContext());
		  char buf[kBufSize];
		  size_t nread = ::fread(buf, 1, sizeof buf, get_pointer(fp));
		  if (nread > 0)
		  {
			conn->send(buf, nread);
		  }
		  else
		  {
			conn->shutdown();
			LOG_INFO << "FileServer - done";
		  }
		}
		
	EventLoop* loop_;
	TcpServer server_;
}; // end of FileTransferServer

int main(int argc, char* argv[])
{
	LOG_INFO << "pid = " << getpid();
	if (argc > 1)
	{
		g_file = argv[1];

		EventLoop loop;
		InetAddress listenAddr(2021);

		FileTransferServer server(&loop, listenAddr);
		server.start();
		loop.loop();
	}
	else
	{
		fprintf(stderr, "Usage： %s file_for_downloading\n", argv[0]);
	}
}
