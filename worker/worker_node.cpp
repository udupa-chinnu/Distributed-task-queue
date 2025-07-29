#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/UUIDGenerator.h>
#include <iostream>
#include <sstream>

using namespace Poco::Net;
using namespace Poco;
using namespace std;

string workerId;

void processTask(const string& task) {
    cout << "[" << workerId << "] Executing: " << task << endl;

    // Simulate work
    Thread::sleep(2000);

    if (task.find("fail") != string::npos) {
        HTTPClientSession session("task-queue-server", 9090);
        HTTPRequest request(HTTPRequest::HTTP_GET, "/fail/" + task, HTTPMessage::HTTP_1_1);
        session.sendRequest(request);
        cout << "[" << workerId << "] Task failed and marked as failed\n";
    } else {
        HTTPClientSession session("task-queue-server", 9090);
        HTTPRequest request(HTTPRequest::HTTP_GET, "/done/" + task, HTTPMessage::HTTP_1_1);
        session.sendRequest(request);
        cout << "[" << workerId << "] Task completed\n";
    }
}

int main() {
    // Generate a unique worker ID using UUID
    workerId = "worker-" + UUIDGenerator::defaultGenerator().createRandom().toString();
    cout << "[" << workerId << "] Worker started\n";

    while (true) {
        try {
            HTTPClientSession session("task-queue-server", 9090);
            HTTPRequest request(HTTPRequest::HTTP_GET, "/get/" + workerId, HTTPMessage::HTTP_1_1);
            session.sendRequest(request);
            HTTPResponse response;
            istream& rs = session.receiveResponse(response);
            string task;
            StreamCopier::copyToString(rs, task);

            if (task != "no task found") {
                processTask(task);
            } else {
                cout << "[" << workerId << "] No task found. Sleeping...\n";
                Thread::sleep(3000);
            }
        } catch (const Exception& ex) {
            cerr << "[" << workerId << "] Error: " << ex.displayText() << endl;
            Thread::sleep(3000);
        }
    }
}
