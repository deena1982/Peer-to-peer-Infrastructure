#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

using namespace boost::asio;
io_service service;

ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
ip::tcp::socket sock(service);
int already_read_;
enum { max_msg = 1024 };
char buff_[max_msg];
bool connected_ = false;
std::string peerNode;

void hello_server()
{
  std::string msg("hello\n");
  sock.write_some(buffer(msg));
  std::cout<<"Sending "<< msg << " message to seederServer\n";
}

size_t read_complete(const boost::system::error_code & err, size_t bytes)
{
  if (err) return 0;
  already_read_ = bytes;
  bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
  return found ? 0 : 1;
}

void process_msg()
{
  std::string msg(buff_, already_read_);
  std::cout<<"Rx msg from seeder = ["<<msg<<"]\n";
}

void tokeniseAndSelect(bool select=false)
{
	/*randomly choose the node to connect to from the list of
	  nodes sent by the seeder server*/
	std::string msg(buff_, already_read_);
	std::cout<<"Rx msg from seeder = ["<<msg<<"]\n";
	boost::char_separator<char> sep(",");
	boost::tokenizer<boost::char_separator<char>> tokens (msg,sep);

	/* DEUBUG prints
	std::cout << "List of nodes in the n/w" << "\n";
	for (const auto& t: tokens)
		std::cout<<t<<"\n";*/

	int s = std::distance(tokens.begin(), tokens.end());
	std::cout<<"No. of Nodes in the network = "<<s<<"\n";
	int randNum = rand()%s+1;
	std::cout<<"random choose node num = "<<randNum<<"\n";
	if (randNum!=1)
	{
		auto nex = std::next(tokens.begin(), randNum-1);
		peerNode=*nex; std::size_t pos = peerNode.find(".Peer->");
		peerNode=peerNode.replace(pos,std::string::npos,"");
		std::cout<<"node choosen is = "<<peerNode<<"\n";
	}
	else
	{
		auto nex = tokens.begin();
		peerNode=*nex;std::size_t pos = peerNode.find(".Peer->");
		peerNode=peerNode.replace(pos,std::string::npos,"");
		std::cout<<"node chosen is = "<<peerNode<<"\n";
	}
}

void read_response(bool selectNode=false)
{
  already_read_ = 0;
  read(sock, buffer(buff_), boost::bind(read_complete, _1, _2));
  if (selectNode)
  {
	  tokeniseAndSelect(true);
  }
  else
	  process_msg();
}

void request_all_nodes(bool select=false)
{
  std::string msg ("list_all\n");
  std::cout<<"Sending "<< msg << " message to seederServer\n";
  sock.write_some(buffer(msg));
  if (select)
	  read_response(true);
  else
	  read_response();
}

void request_nodes(std::string msg)
{
	std::cout<<"Sending "<< msg << " message to seederServer\n";
	sock.write_some(buffer(msg));
	read_response();
}

void run_client()
{
  std::cout<<"Client Connecting..."<<"\n";;
  try
  {
    sock.connect(ep);
    hello_server();
    read_response();
    connected_ = true;
  }
  catch(boost::system::system_error & err)
  {
    std::cout<<"client terminated: "<<err.what()<<"\n";;
  }

  request_all_nodes(true);
  int option;
  std::string msg_to_be_sent;
  while(true)
  {
    std::cout<<"Enter number:\n0 - All Nodes\n1 - Alive for 1 hour\n"<<
                "2 - Alive for 2 hours\n3 - Alive for one day\n4 - Alive for 5 min\n"<<
				"5 - Alive for 10 min\n"<<"\n";;
    std::cout<<"> ";
    std::cin>>option;
    switch(option)
    {
      case 0:
        request_all_nodes();
        break;
      case 1:
        msg_to_be_sent = "list_1_hour\n";
        request_nodes(msg_to_be_sent);
        break;
      case 2:
        msg_to_be_sent = "list_2_hour\n";
        request_nodes(msg_to_be_sent);
        break;
      case 3:
        msg_to_be_sent = "list_1_day\n";
        request_nodes(msg_to_be_sent);
        break;
      case 4:
        msg_to_be_sent = "list_5_min\n";
        request_nodes(msg_to_be_sent);
        break;
      case 5:
        msg_to_be_sent = "list_10_min\n";
        request_nodes(msg_to_be_sent);
        break;
      default:
        break;
    }
  }
}

void ping()
{
  while(true)
  {
	//send ping for every 1 seconds
	//client must send its peer address also in ping
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if (connected_)
    {
    	std::string msg ("ping_peer::");
    	msg=msg+peerNode+std::string(" \n");
    	//std::string msg ("ping\n");
    	sock.write_some(buffer(msg));
    }
  }
}

int main(int argc, char* argv[])
{
  std::vector<std::thread> threads;
  std::cout << "starting client thread...\n";
  threads.push_back(std::thread(run_client));
  std::cout << "starting ping thread...\n";
  threads.push_back(std::thread(ping));

  for (auto& th : threads) th.join();
}
