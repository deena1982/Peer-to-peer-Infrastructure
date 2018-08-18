#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <regex>
#include <memory>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <sys/time.h>
#include "node.cpp"

using namespace boost::asio;
using namespace boost::posix_time;

void new_nodes_thread()
{
  ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
  while(true)
  {
    node_ptr new_(new node);
    acceptor.accept(new_->sock());
    //std::cout<<"new_nodes_thread locks mutex cs\n";
    std::lock_guard<std::recursive_mutex> lck (cs);
    nodes.push_back(new_);
  }
}

void serve_nodes_thread()
{
  while(true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //std::cout<<"serve_nodes_thread locks mutex cs\n";
    std::lock_guard<std::recursive_mutex> lck (cs);
    for(array::iterator a = nodes.begin(), z = nodes.end(); a != z; ++a)
    {
      (*a)->serve_node();
    }
  }
}

void monitor_nodes_thread()
{
  while(true)
  {
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//{
		//std::cout<<"monitor thread locks mutex cs\n";
		std::lock_guard<std::recursive_mutex> lck (cs);
		for(array::iterator a = nodes.begin(), z = nodes.end(); a != z; ++a)
		{
		  (*a)->monitor_node();
		}
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), boost::bind(&node::timed_out,_1)), nodes.end());
	//}
  }
}

int main(int argc, char* argv[])
{
	std::cout << "Starting Server....\n";
	std::vector<std::thread> threads;
	std::cout << "starting new_nodes_thread...\n";
	threads.push_back(std::thread(new_nodes_thread));
	std::cout << "starting serve_nodes_thread...\n";
	threads.push_back(std::thread(serve_nodes_thread));
	std::cout << "starting ping thread...\n";
	threads.push_back(std::thread(monitor_nodes_thread));
	for (auto& th : threads) th.join();
}
