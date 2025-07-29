#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/Option.h>
#include <Poco/Thread.h>
#include <Poco/Mutex.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <signal.h>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace Poco::JSON;
using namespace std;

enum TaskStatus { PENDING, IN_PROGRESS, COMPLETED };

struct Task {
    string id;
    string data;
    TaskStatus status;
};

vector<Task> tasks;
Poco::Mutex taskMutex;

class TaskRequestHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        ostream& out = response.send();
        response.setContentType("application/json");

        if (request.getURI() == "/add_task" && request.getMethod() == "POST") {
            handleAddTask(request, out);
        } else if (request.getURI() == "/get_task") {
            handleGetTask(out);
        } else if (request.getURI().find("/complete_task?id=") != string::npos) {
            handleCompleteTask(request, out);
        } else {
            response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
            out << "{\"error\":\"Endpoint not found\"}";
        }
    }

private:
    void handleAddTask(HTTPServerRequest& request, ostream& out) {
        string body;
        request.stream() >> body;

        Poco::Mutex::ScopedLock lock(taskMutex);
        string id = "task" + to_string(tasks.size() + 1);
        tasks.push_back({id, body, PENDING});
        out << "{\"status\":\"Task added\", \"id\":\"" << id << "\"}";
        cout << "Task added: " << id << "\n";
    }

    void handleGetTask(ostream& out) {
        Poco::Mutex::ScopedLock lock(taskMutex);
        for (auto& task : tasks) {
            if (task.status == PENDING) {
                task.status = IN_PROGRESS;
                out << "{\"id\":\"" << task.id << "\", \"data\":\"" << task.data << "\"}";
                cout << "Assigned task: " << task.id << "\n";
                return;
            }
        }
        out << "{\"status\":\"No tasks available\"}";
    }

    void handleCompleteTask(HTTPServerRequest& request, ostream& out) {
        string id = request.getURI().substr(request.getURI().find("=") + 1);
        Poco::Mutex::ScopedLock lock(taskMutex);
        for (auto& task : tasks) {
            if (task.id == id && task.status == IN_PROGRESS) {
                task.status = COMPLETED;
                out << "{\"status\":\"Task marked as completed\"}";
                cout << "Task completed: " << task.id << "\n";
                return;
            }
        }
        out << "{\"error\":\"Task not found or not in progress\"}";
    }
};

class TaskRequestFactory : public HTTPRequestHandlerFactory {
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest&) override {
        return new TaskRequestHandler;
    }
};

bool stopServer = false;

void handleSignal(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Stopping server..." << std::endl;
    stopServer = true;
}

int main(int argc, char** argv) {
    signal(SIGINT, handleSignal);

    try {
        ServerSocket socket(9090);
        HTTPServer server(new TaskRequestFactory, socket, new HTTPServerParams);
        server.start();
        cout << "Server started on port 9090\n";

        while (!stopServer) {
            Poco::Thread::sleep(1000);
        }

        server.stop();
        cout << "Server stopped.\n";
    } catch (const Poco::Exception& ex) {
        cerr << "Poco Exception: " << ex.displayText() << endl;
        return 1;
    } catch (const std::exception& e) {
        cerr << "Std Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
