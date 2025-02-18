#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;

std::string request =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n\r\n";

void send_request(tcp::socket& socket)
{
    boost::asio::write(socket, boost::asio::buffer(request));
}

std::string receive_response(tcp::socket& socket)
{
    boost::system::error_code error;
    char data[1024];
    size_t length = socket.read_some(boost::asio::buffer(data), error);

    if (!error) {
        return std::string(data, length);
    } else {
        return "";
    }
}

int main()
{
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "8080");

        int request_count = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (true) {
            tcp::socket socket(io_context);
            boost::asio::connect(socket, endpoints);

            auto request_start = std::chrono::steady_clock::now();
            send_request(socket);
            std::string response = receive_response(socket);
            auto request_end = std::chrono::steady_clock::now();

            std::chrono::duration<double> request_duration = request_end - request_start;
            std::cout << "Request duration: " << request_duration.count() << " seconds" << std::endl;

            request_count++;
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_time = current_time - start_time;
            double requests_per_second = request_count / elapsed_time.count();
            std::cout << "Requests per second: " << requests_per_second << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Adjust delay as needed
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}