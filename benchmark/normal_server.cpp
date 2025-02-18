#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

std::string make_response()
{
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";
    return response;
}

void handle_request(tcp::socket& socket)
{
    boost::system::error_code error;
    char data[1024];
    size_t length = socket.read_some(boost::asio::buffer(data), error);

    if (!error) {
        std::string request(data, length);
        std::cout << "Received request:\n" << request << std::endl;

        std::string response = make_response();
        boost::asio::write(socket, boost::asio::buffer(response), error);
    }
}

int main()
{
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        std::cout << "Server is running on port 8080..." << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            handle_request(socket);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}