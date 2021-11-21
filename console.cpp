#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;


boost::asio::io_service io_context;

void output_shell(string session,string content){
  //cerr << content << endl;
  boost::replace_all(content, "\n", "&NewLine;");
  boost::replace_all(content, "'", "\\'");
  printf("<script>document.getElementById('%s').innerHTML += '%s';</script>",session.c_str(),content.c_str());
  fflush(stdout);
}

void output_command(string session,string content){
  //cerr << content << endl;
  boost::replace_all(content, "\n", "&NewLine;");
  boost::replace_all(content, "'", "\\'");
  printf("<script>document.getElementById('%s').innerHTML += '<b>%s</b>';</script>",session.c_str(),content.c_str());
  fflush(stdout);
}

class session
: public std::enable_shared_from_this<session>
{
	public:
		session(string host, string port,string f)
			: client_socket(io_context)
		{
      memset(data_,0,max_length);
      string filename = "./test_case/"+f;
      file_input.open(filename,ios::in);
      tcp::resolver resolve(io_context);
      tcp::resolver::query query(host, port);
      boost::asio::ip::tcp::resolver::iterator iter = resolve.resolve(query);
      client_socket.connect(iter->endpoint());
      start();
		}
		void start()
		{
			do_read();
		}

	private:
		void do_read()
		{
			//auto self(shared_from_this());
      client_socket.async_read_some(boost::asio::buffer(data_, max_length),
          [this](boost::system::error_code ec, std::size_t length){
          if (!ec){
            output_shell("s0",string(data_));
            char *ret;
            ret = strstr(data_, "%");
            memset(data_,0,max_length);
            if(ret){
              //cerr << "go write" << endl;
              do_write();
            }
            else{
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
      output_command("s0",input);
      if(file_input.eof()) return;
      boost::asio::async_write(client_socket, boost::asio::buffer(input.c_str(), input.length()),
        [this](boost::system::error_code ec, std::size_t /*length*/){
        if (!ec){
          //cerr << "go read" << endl;
          do_read();
        }
      });
		}

		tcp::socket client_socket;
		enum { max_length = 1024 };
		char data_[max_length];
    ifstream file_input;
};


void print_header(){
  cout << "Content-type: text/html\r\n\r\n";
  cout << "<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\" />\
    <title>NP Project 3 Sample Console</title>\
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
        <tr>\
          <th scope=\"col\">nplinux1.cs.nctu.edu.tw:1234</th>\
          <th scope=\"col\">nplinux2.cs.nctu.edu.tw:5678</th>\
        </tr>\
      </thead>\
      <tbody>\
        <tr>\
          <td><pre id=\"s0\" class=\"mb-0\"></pre></td>\
          <td><pre id=\"s1\" class=\"mb-0\"></pre></td>\
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
    print_header();
    string host = string("nplinux7.cs.nctu.edu.tw");
    string port = string("7777");
    string filename = string("t1.txt");
    session s(host, port,filename);
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}