#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/PostgreSQL/Connector.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Statement.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <iostream>
#include <sstream>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace Poco::Data;
using namespace Poco::Data::Keywords;
using namespace Poco::JSON;
using namespace std;

Session createSession() {
    return Session("PostgreSQL", "host=yugabyte port=5433 user=postgres password=postgres dbname=postgres");
}

// === Handler for /add_task ===
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

// === Handler for /get/<worker_id> ===
class GetTaskHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        string uri = request.getURI();
        string workerId = uri.substr(uri.find_last_of('/') + 1);

        Session session = createSession();
        Statement select(session), update(session);

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

        } catch (const Exception& ex) {
            session.rollback();
            response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            response.send() << "Error: " << ex.displayText();
        }
    }
};

// === Handler for /done/<task_id> ===
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

// === Handler for /fail/<task_id> ===
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
        if (request.getURI().find("/add_task") == 0)
            return new AddTaskHandler;
        else if (request.getURI().find("/get/") == 0)
            return new GetTaskHandler;
        else if (request.getURI().find("/done/") == 0)
            return new DoneTaskHandler;
        else if (request.getURI().find("/fail/") == 0)
            return new FailTaskHandler;
        else
            return nullptr;
    }
};

// === Main App ===
int main(int argc, char** argv) {
    PostgreSQL::Connector::registerConnector();
    try {
        ServerSocket socket(9090);
        HTTPServer server(new TaskRequestFactory, socket, new HTTPServerParams);
        server.start();
        std::cout << "Task Queue Server running on port 9090...\n";
        std::cout << "Waiting for termination...\n";
        waitForTerminationRequest();
        server.stop();
        std::cout << "Server stopped.\n";
    } catch (const Exception& e) {
        std::cerr << "Exception: " << e.displayText() << std::endl;
        return 1;
    }

    return 0;
}
