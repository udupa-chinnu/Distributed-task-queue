# 🚀 Distributed Task Queue System (C++ + Poco + YugabyteDB)

This project implements a **distributed task queue** using C++ with the **Poco libraries** and **YugabyteDB** as the backend database. It features:

- A **Task Queue Server** that accepts tasks and stores them in the database.
- Multiple **Worker Nodes** that poll the server for tasks, execute them, and update the status.
- A **YugabyteDB** instance that acts as the central coordination and task store.

---

## 📦 Architecture Overview


- Tasks are **added via HTTP** to the Task Queue Server.
- Server stores tasks in **YugabyteDB**.
- Workers poll server for available tasks and **update status** after execution.
- Workers are **scalable and run in parallel**.

---

## 🚀 How to Run

### 1️⃣ Start Everything

```bash
docker-compose up --build --scale worker=3
```
This will:

Build the task-queue-server and worker images.

Start:

YugabyteDB container (on port 5433 for SQL)

Task queue server (on port 9090)

3 worker containers (you can scale more!)

### Add Tasks
Use curl to add tasks to the queue via the Task Queue Server:

```bash
curl -X POST http://localhost:9090/add_task -d "task1"
curl -X POST http://localhost:9090/add_task -d "task2"
```

###  Server Endpoints

| Method | Endpoint          | Description                     |
| ------ | ----------------- | ------------------------------- |
| POST   | `/add_task`       | Add a task (raw text body)      |
| GET    | `/get/<workerId>` | Worker fetches one pending task |
| GET    | `/done/<taskId>`  | Mark task as completed          |
| GET    | `/fail/<taskId>`  | Mark task as failed             |

### Scaling Workers
To increase the number of worker nodes dynamically:
```bash
docker-compose up --scale worker=5 -d
```
This will create two more workers and connect them automatically.

### 🗃️ Database Details
YugabyteDB is used with PostgreSQL compatibility.

Tables are auto-created by the Task Queue Server:
```

tasks(id TEXT PRIMARY KEY, status TEXT, payload TEXT, assigned_to TEXT)

locks(id TEXT PRIMARY KEY, owner TEXT, acquired_at TIMESTAMPTZ)
```
### 🛑 Stop Everything
```docker-compose down```
