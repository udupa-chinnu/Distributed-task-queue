#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/Thread.h>
#include <Poco/ThreadTarget.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/UUIDGenerator.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace Poco::Net;
using namespace Poco::JSON;
using namespace Poco;
using namespace std;

string workerId;

void reportTaskStatus(const string& endpoint, const string& taskId) {
    try {
        HTTPClientSession session("task-queue-server", 9090);
        string url = "/" + endpoint + "?id=" + taskId;
        HTTPRequest request(HTTPRequest::HTTP_GET, url, HTTPMessage::HTTP_1_1);
        session.sendRequest(request);
        HTTPResponse response;
        session.receiveResponse(response);
        cout << "[" << workerId << "] Reported " << endpoint << " for task " << taskId << endl;
    } catch (const Exception& ex) {
        cerr << "[" << workerId << "] Failed to report " << endpoint << " for task " << taskId
             << ": " << ex.displayText() << endl;
    }
}

void processTask(const string& id, const string& data) {
    cout << "[" << workerId << "] Executing task " << id << ": " << data << endl;

    // Simulate work
    Poco::Thread::sleep(1000);

    if (data.find("fail") != string::npos) {
        cerr << "[" << workerId << "] Task failed during processing.\n";
        reportTaskStatus("fail_task", id);
    } else {
        reportTaskStatus("complete_task", id);
    }
}

int main() {
    // Set unique worker ID using thread ID
    workerId = "worker-" + UUIDGenerator::defaultGenerator().createRandom().toString();
    cout << "[" << workerId << "] Worker started\n";

    while (true) {
        try {
            HTTPClientSession session("task-queue-server", 9090);
            HTTPRequest request(HTTPRequest::HTTP_GET, "/get_task", HTTPMessage::HTTP_1_1);
            session.sendRequest(request);

            HTTPResponse response;
            istream& rs = session.receiveResponse(response);

            string responseBody;
            StreamCopier::copyToString(rs, responseBody);

            if (responseBody.find("No tasks available") != string::npos) {
                cout << "[" << workerId << "] No task found. Sleeping...\n";
                Poco::Thread::sleep(2000);
                continue;
            }

            // Parse JSON
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(responseBody);
            Poco::JSON::Object::Ptr obj = result.extract<Poco::JSON::Object::Ptr>();


            string taskId = obj->getValue<string>("id");
            string taskData = obj->getValue<string>("data");

            processTask(taskId, taskData);

        } catch (const Exception& ex) {
            cerr << "[" << workerId << "] Error fetching task: " << ex.displayText() << endl;
            Poco::Thread::sleep(3000); // Sleep before retry
        }
    }

    return 0;
}
