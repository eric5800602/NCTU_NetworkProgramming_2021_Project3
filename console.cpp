#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;


struct client_info{
  string host;
  string port;
  string file;
};

boost::asio::io_service io_context[5];
client_info client[5];


void output_shell(int index,string content){
  //cerr << content << endl;
  boost::replace_all(content, "\n\r", " ");
  boost::replace_all(content, "\n", "&NewLine;");
  boost::replace_all(content, "\r", " ");
  boost::replace_all(content, "'", "\\'");
  boost::replace_all(content, "<", "&lt;");
  boost::replace_all(content, ">", "&gt;");
  printf("<script>document.getElementById('s%d').innerHTML += '%s';</script>",index,content.c_str());
  fflush(stdout);
}

void output_command(int index,string content){
  //cerr << content << endl;
  boost::replace_all(content, "\n\r", " ");
  boost::replace_all(content, "\n", "&NewLine;");
  boost::replace_all(content, "\r", " ");
  boost::replace_all(content, "'", "\\'");
  boost::replace_all(content, "<", "&lt;");
  boost::replace_all(content, ">", "&gt;");
  printf("<script>document.getElementById('s%d').innerHTML += '<b>%s</b>';</script>",index,content.c_str());
  fflush(stdout);
}

void add_row(int index,string host,string port){
  printf("<script>var table = document.getElementById('table_tr'); table.innerHTML += '<th>%s:%s</th>';</script>",host.c_str(),port.c_str());
  printf("<script>var table = document.getElementById('session'); table.innerHTML += '<td><pre id=\\'s%d\\' class=\\'mb-0\\'></pre></td>&NewLine;' </script>",index);
  fflush(stdout);
}

class session
: public std::enable_shared_from_this<session>
{
	public:
		session(int index,string host, string port,string f)
			: client_socket(io_context[index])
		{
      memset(data_,0,max_length);
      id = index;
      valid = true;
      string filename = "./test_case/"+f;
      file_input.open(filename,ios::in);
      tcp::resolver resolve(io_context[index]);
      tcp::resolver::query query(host, port);
      boost::asio::ip::tcp::resolver::iterator iter = resolve.resolve(query);
      client_socket.connect(iter->endpoint());
      add_row(index,host,port);
		}
		void start()
		{
			do_read();
		}
    bool valid;

	private:
		void do_read()
		{
			//auto self(shared_from_this());
      client_socket.async_read_some(boost::asio::buffer(data_, max_length),
          [this](boost::system::error_code ec, std::size_t length){
          if (!ec){
            output_shell(id,string(data_));
            if(file_input.eof()) valid = false;
            char *ret;
            ret = strstr(data_, "%");
            memset(data_,0,max_length);
            if(ret){
              do_write();
            }else{
              do_read();
            }
          }
          });
		}

		void do_write()
		{
			//auto self(shared_from_this());
      string input;
      getline(file_input,input);
      input = input + "\n";
      cerr << input << endl;
      output_command(id,input);
      if(file_input.eof()) return;
      boost::asio::async_write(client_socket, boost::asio::buffer(input.c_str(), input.length()),
        [this](boost::system::error_code ec, std::size_t /*length*/){
          if (!ec){
            return;
        }});
    }

		tcp::socket client_socket;
		enum { max_length = 1024 };
		char data_[max_length];
    ifstream file_input;
    int id;
};


void print_header(){
  cout << "Content-type: text/html\r\n\r\n";
  cout << "<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\" />\
    <title>NP Project 3 Console</title>\
    <link\
      rel=\"stylesheet\"\
      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
      crossorigin=\"anonymous\"\
    />\
    <link\
      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
      rel=\"stylesheet\"\
    />\
    <link\
      rel=\"icon\"\
      type=\"image/png\"\
      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
    />\
    <style>\
      * {\
        font-family: 'Source Code Pro', monospace;\
        font-size: 1rem !important;\
      }\
      body {\
        background-color: #212529;\
      }\
      pre {\
        color: #cccccc;\
      }\
      b {\
        color: #01b468;\
      }\
    </style>\
  </head>\
  <body>\
    <table class=\"table table-dark table-bordered\">\
      <thead>\
        <tr id=\"table_tr\">\
        </tr>\
      </thead>\
      <tbody>\
        <tr id=\"session\">\
        </tr>\
      </tbody>\
    </table>\
  </body>\
</html>";
}

int main(int argc, char* argv[]){
  try
	{
		if (argc != 4)
		{
			std::cerr << "Usage: console.cgi <host> <port> <file>\n";
			//return 1;
		}
    // string host = string(argv[1]);
    // string port = string(argv[2]);
    // string filename = string(argv[3]);
    // example: h0=nplinux3.cs.nctu.edu.tw&p0=9898&f0=t2.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
    print_header();
    string QUERY_STRING = getenv("QUERY_STRING");
    size_t pos = 0;
    std::string token;
    int index = 0;
    int i = 0;
    while ((pos = QUERY_STRING.find("&")) != std::string::npos) {
      size_t pos2 = 0;
      if((pos2 = QUERY_STRING.find("=")+1) != std::string::npos){
        token = QUERY_STRING.substr(pos2, pos-3);
        if(token.length() == 0)break;
        switch(index){
          case 0:
            client[i].host = token;
            index++;
            break;
          case 1:
            client[i].port = token;
            index++;
            break;
          case 2:
            client[i].file = token;
            index = 0;
            i++;
            break;
          default:
            std::cerr << "Exception: switch error " << "\n";
            break;
        }
        QUERY_STRING.erase(0, pos + string("&").length());
      }
    }
    size_t pos2 = 0;
    if((pos2 = QUERY_STRING.find("=")+1) != std::string::npos){
      token = QUERY_STRING.substr(pos2, QUERY_STRING.length());
      client[i].file = token;
    }
    ///cerr << getenv("QUERY_STRING") << endl;
    for(int j = 0;j < i;j++){
      cerr << "host: "<<client[j].host << endl;
      cerr << "port: "<<client[j].port << endl;
      cerr << "file: "<<client[j].file << endl;
      session s(j,client[j].host, client[j].port,client[j].file);
      io_context[j].run();
    }
    if(client[0].host.length()!=0){
      session s0(0,client[0].host, client[0].port,client[0].file);
      io_context[0].run();
    }
    if(client[1].host.length()!=0){
      session s1(1,client[1].host, client[1].port,client[1].file);
      io_context[1].run();
    }
    if(client[2].host.length()!=0){
      session s2(2,client[2].host, client[2].port,client[2].file);
      io_context[2].run();
    }
    if(client[3].host.length()!=0){
      session s3(3,client[3].host, client[3].port,client[3].file);
      io_context[3].run();
    }
    if(client[4].host.length()!=0){
      session s4(4,client[4].host, client[4].port,client[4].file);
      io_context[4].run();
    }
    /*
    while(true){
      if(!(s0.valid || s1.valid || s2.valid || s3.valid || s4.valid)) break;
      if(s0.valid) s0.start();
      if(s1.valid) s1.start();
      if(s2.valid) s2.start();
      if(s3.valid) s3.start();
      if(s4.valid) s4.start();
    }
    */
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}