#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/PostgreSQL/Connector.h>
#include <Poco/Data/Statement.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/Exception.h>
#include <thread>     
#include <chrono>
#include <iostream>
#include <sstream>

using namespace Poco::Net;
using namespace Poco::Data;
using namespace Poco::Data::Keywords;
using namespace Poco::JSON;
using namespace std;

// === PostgreSQL session helper ===
Session createSession() {
    return Session("PostgreSQL", "host=yugabyte port=5433 user=postgres password=postgres dbname=postgres");
}

// === /add_task ===
class AddTaskHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        Session session = createSession();
        string taskData;
        request.stream() >> taskData;

        Statement stmt(session);
        stmt << "INSERT INTO tasks (data, status) VALUES ($1, 'pending')", use(taskData);
        stmt.execute();

        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType("application/json");
        Object obj;
        obj.set("status", "Task added");
        obj.set("data", taskData);
        obj.stringify(response.send());
    }
};

// === /get/<worker_id> ===
class GetTaskHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        string uri = request.getURI();
        string workerId = uri.substr(uri.find_last_of('/') + 1);

        Session session = createSession();
        Statement select(session);

        string taskId, taskData;

        try {
            session.begin();

            select << R"(
                UPDATE tasks SET status='in_progress', assigned_to=$1
                WHERE id = (
                    SELECT id FROM tasks WHERE status='pending'
                    ORDER BY id LIMIT 1 FOR UPDATE SKIP LOCKED
                )
                RETURNING id, data
            )",
            use(workerId), into(taskId), into(taskData);

            if (select.execute() == 0) {
                session.rollback();
                response.setStatus(HTTPResponse::HTTP_OK);
                response.setContentType("text/plain");
                response.send() << "no task found";
                return;
            }

            session.commit();

            response.setStatus(HTTPResponse::HTTP_OK);
            response.setContentType("text/plain");
            response.send() << taskData;

        } catch (const Poco::Exception& ex) {
            session.rollback();
            response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            response.send() << "Error: " << ex.displayText();
        }
    }
};

// === /done/<task_id> ===
class DoneTaskHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        string uri = request.getURI();
        string taskId = uri.substr(uri.find_last_of('/') + 1);

        Session session = createSession();
        Statement stmt(session);
        stmt << "UPDATE tasks SET status='done' WHERE id=$1", use(taskId);
        stmt.execute();

        response.setStatus(HTTPResponse::HTTP_OK);
        response.send() << "Task " << taskId << " marked as done.";
    }
};

// === /fail/<task_id> ===
class FailTaskHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        string uri = request.getURI();
        string taskId = uri.substr(uri.find_last_of('/') + 1);

        Session session = createSession();
        Statement stmt(session);
        stmt << "UPDATE tasks SET status='failed' WHERE id=$1", use(taskId);
        stmt.execute();

        response.setStatus(HTTPResponse::HTTP_OK);
        response.send() << "Task " << taskId << " marked as failed.";
    }
};

// === Factory ===
class TaskRequestFactory : public HTTPRequestHandlerFactory {
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override {
        const std::string& uri = request.getURI();

        if (uri.find("/add_task") == 0) return new AddTaskHandler;
        if (uri.find("/get/") == 0) return new GetTaskHandler;
        if (uri.find("/done/") == 0) return new DoneTaskHandler;
        if (uri.find("/fail/") == 0) return new FailTaskHandler;

        return nullptr;
    }
};

// === Main ===
int main(int argc, char** argv) {
    Poco::Data::PostgreSQL::Connector::registerConnector();
    try {
        ServerSocket socket(9090);
        HTTPServer server(new TaskRequestFactory, socket, new HTTPServerParams);
        server.start();
        std::cout << "Task Queue Server running on port 9090...\n";
        while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
        server.stop();
    } catch (const Poco::Exception& e) {
        std::cerr << "Exception: " << e.displayText() << std::endl;
        return 1;
    }

    return 0;
}
