# Spyware Project Documentation

## Project Overview
This project consists of three main components:

1. **Agent (Malware)**: A C-based program that collects information from the infected machine (e.g., hostname, OS version, IP addresses, logs, screenshots) and sends it to the server.
2. **Backend (Flask API)**: A Python-based API that receives data from the agent and stores it in a database. It also provides endpoints for the frontend to fetch and display data.
3. **Frontend (React + Vite + ShadCN UI)**: A modern web interface to visualize the data collected by the agent and interact with the backend API.

## Project Structure
```
Spyware/
├── agent/
│   ├── agent.c
│   ├── core/        # Core functionalities (e.g., data collection, screenshot capture)
│   ├── network/     # Network communication logic
│   └── utils/       # Utility functions
├── server/
│   ├── backend/
│   │   ├── app.py   # Main entry point for the Flask API
│   │   ├── routes/  # Contains API route definitions
│   │   ├── models/  # Database models (if needed)
│   │   ├── services/ # Utility functions for the backend
│   │   └── requirements.txt # Python dependencies
│   ├── frontend/
│       ├── src/
│       │   ├── components/ # Reusable React components
│       │   ├── pages/      # Main pages of the application
│       │   ├── services/   # API service calls
│       │   ├── styles/     # Global styles
│       │   └── App.jsx     # Main React component
│       ├── public/         # Static assets
│       ├── package.json    # Frontend dependencies
│       └── vite.config.js  # Vite configuration
```

## Features

### Agent (Malware)
- Collects:
  - Hostname
  - OS version
  - Private and public IP addresses
  - Log files
  - Screenshots
- Sends data to the backend API.

### Backend (Flask API)
- **Endpoints**:
  - `/api/report` (POST): Receives data from the agent.
  - `/api/machines` (GET): Returns a list of infected machines.
- Modular structure with Blueprints for routes.
- SQLite database for storing machine data.

### Frontend (React + Vite + ShadCN UI)
- Displays a table of infected machines with the following columns:
  - Hostname
  - OS version
  - Private IP
  - Public IP
- Provides action buttons for each machine (e.g., fetch logs, take screenshot).

## Development Guidelines
- **Backend**:
  - Use Blueprints for route organization.
  - Separate logic into `routes`, `models`, and `services`.
- **Frontend**:
  - Use React components for modularity.
  - Follow best practices for state management and API calls.
- **Agent**:
  - Separate functionalities into different files and compile them into a single executable.

## Setup Instructions

### Backend
1. Navigate to the `server/backend` directory.
2. Create a virtual environment:
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```
3. Run the Flask server:
   ```bash
   python app.py
   ```

### Frontend
1. Navigate to the `server/frontend` directory.
2. Install dependencies:
   ```bash
   npm install
   ```
3. Start the development server:
   ```bash
   npm run dev
   ```

### Agent
1. Navigate to the `agent` directory.
2. Compile the C code:
   ```bash
   gcc -o agent agent.c -lcurl
   ```
3. Run the executable:
   ```bash
   ./agent
   ```

## Future Work
- Add authentication to the API.
- Enhance the frontend with more features (e.g., filtering, sorting).
- Improve the agent's capabilities (e.g., keylogging, file exfiltration).

---
This file serves as a reference for the project structure, features, and development guidelines.