#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/JSON/Object.h>
#include <iostream>

int main() {
    Poco::JSON::Object task;
    task.set("type", "process_data");
    task.set("params", "{\"file\": \"data.txt\"}");

    Poco::Net::HTTPClientSession session("localhost", 8000);
    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, "/add_task");
    request.setContentType("application/json");
    request.setContentLength(task.toString().length());

    session.sendRequest(request) << task.toString();
    Poco::Net::HTTPResponse response;
    session.receiveResponse(response);

    std::cout << "Task submitted! Status: " << response.getStatus() << std::endl;
    return 0;
}