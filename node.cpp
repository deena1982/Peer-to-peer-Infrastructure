using namespace boost::asio;
using namespace boost::posix_time;

struct node;
io_service service;
typedef boost::shared_ptr<node> node_ptr;
typedef std::vector<node_ptr> array;
array nodes;
time_duration ONE_HOUR = hours(1);
time_duration ONE_DAY = hours(24);
time_duration FIVE_MIN = minutes(1);
time_duration TEN_MIN = minutes(10);


std::recursive_mutex cs;


struct node : boost::enable_shared_from_this<node>
{
private:
	ip::tcp::socket sock_;
	ptime join_time_;
	ptime last_ping_;
	enum { max_msg = 1024 };
	int already_read_;
	char buff_[max_msg];
	std::string peer="none";
public:
	node() : sock_(service), already_read_(0)
	{
	}

	void serve_node()
	{
		try
		{
		  read_query();
		  process_query();
		}
		catch(std::exception & err)
		{
		  std::cout<<err.what();
		  stop();
		}
		catch (...)
		{
			std::cout<<"Unknown Error!!!!";
			stop();
		}
	}

	void monitor_node()
	{
		if (timed_out())
		{
		  stop();
		}
	}

	ip::tcp::socket & sock()
	{
		return sock_;
	}

	std::string get_ip()
	{
		return (sock_.remote_endpoint().address().to_string() + ":" + std::to_string(sock_.remote_endpoint().port()));
	}

	std::string get_peer()
	{
		if (peer != "none")
			return peer;
		else
			return std::string("");
	}

	bool timed_out()
	{
		ptime now = microsec_clock::local_time();
		long long ms = (now - last_ping_).total_milliseconds();
		//timeout returns true if no heartbeat for 3 sec
		return (ms > 3000);
	}

	long long get_alive_since()
	{
		time_duration life = last_ping_ - join_time_;
		return life.total_milliseconds();
	}

	void stop()
	{
		boost::system::error_code err;
		sock_.close(err);
		std::cout<<err.message()<<"\n";
	}

private:
	void read_query()
	{
		if (sock_.available())
		{
		  already_read_ += sock_.read_some(buffer(buff_ + already_read_, max_msg - already_read_));
		}
	}

	void process_query()
	{
		bool found_endofline = std::find(buff_, buff_ + already_read_, '\n') < buff_ + already_read_;
		if (!found_endofline)
		{
		  return;
		}
		last_ping_ = microsec_clock::local_time();
		size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
		std::string msg(buff_, pos);
		std::copy(buff_ + already_read_, buff_ + max_msg, buff_);
		already_read_ -= pos + 1;

		std::cout<<"Msg rx from client ["<<msg<<"]\n";
		if (msg.find("ping_peer::") == 0)
		{
			std::size_t pos2 = msg.find("::");
			std::string ipPort= msg.substr(pos2+2);
			//std::cout<<"Peer is "<<ipPort<<"\n";
			peer=ipPort;
		//if (msg.find("ping") == 0)
			on_ping();
		}
		else if (msg.find("hello") == 0)
			on_join();
		else if (msg.find("list_all") == 0)
			on_list_all_nodes();
		else if (msg.find("list_1_hour") == 0)
			on_list_nodes_alive_for(ONE_HOUR.total_milliseconds());
		else if (msg.find("list_2_hour") == 0)
			on_list_nodes_alive_for(2*ONE_HOUR.total_milliseconds());
		else if (msg.find("list_1_day") == 0)
			on_list_nodes_alive_for(ONE_DAY.total_milliseconds());
		else if (msg.find("list_5_min") == 0)
			on_list_nodes_alive_for(FIVE_MIN.total_milliseconds());
		else if (msg.find("list_10_min") == 0)
			on_list_nodes_alive_for(TEN_MIN.total_milliseconds());
		else
			std::cerr << "invalid msg " << msg << "\n";;
	}

	void on_ping()
	{
		last_ping_ = microsec_clock::local_time();
	}

	void on_join()
	{
		join_time_ = microsec_clock::local_time();
		last_ping_ = microsec_clock::local_time();
		std::cout<<"Node: "<<get_ip()<<" joined"<<"\n";
		std::cout<<"Sending hello to node\n";
		write("hello from seeder\n");
	}

	void on_list_all_nodes()
	{
		std::string write_msg;
		{
		  std::lock_guard<std::recursive_mutex> lck (cs);
		  for(array::const_iterator a = nodes.begin(), z = nodes.end(); a != z; ++a)
		  {
			write_msg += (*a)->get_ip() + ".Peer->" +(*a)->get_peer()+ ",";
		  }
		}
		write_msg.pop_back();
		write_msg += "\n";
		std::cout<<"Sending list of all nodes as requested\n";
		write(write_msg);
	}

	void on_list_nodes_alive_for(long long query_time)
	{
		std::string write_msg="Nodes alive: ";
		{
		  std::lock_guard<std::recursive_mutex> lck (cs);
		  for(array::const_iterator a = nodes.begin(), z = nodes.end(); a != z; ++a)
		  {
			if ((*a)->get_alive_since() > query_time)
			{
			  write_msg += (*a)->get_ip() + ", ";
			}
		  }
		}
		write_msg.pop_back();
		write_msg += "\n";
		std::cout<<"Sending list of all nodes live after a timePeriod\n";
		write(write_msg);
	}

	void write(const std::string & msg)
	{
		sock_.write_some(buffer(msg));
	}
};
