<img src="https://readme-typing-svg.herokuapp.com?font=Anaheim&size=32&duration=3000&pause=1000&color=0081FB&width=1000&lines=Bankara;API+Endpoint+Tester" alt="Typing SVG" />

**Bankara** is a project designed to explore backend engineering, web security, databases, networking, and software architecture. 

The name comes from the Japanese concept of Bankara - a spirit associated with ruggedness, independence, and a willingness to take a different path from prevailing trends.

For this project, that idea translates into choosing to learn beneath the abstraction layer rather than relying entirely on managed platforms and generated solutions.

This is a small-scale project built to better understand how modern financial software operates at a lower level. It prioritizes learning, experimentation, and technical understanding over speed of deployment.

## Project Overview

Bankara functions as a prototype digital wallet with the following features:
- **Digital Wallet Dashboard**: A clean interface displaying current balances and transaction history.
- **P2P Fund Transfers**: A system for sending and receiving simulated funds between registered users.
- **Multi-Factor Authentication (MFA)**: Account protection using Time-Based One-Time Passwords (TOTP) via Google Authenticator.

## Technologies Used

- **Frontend**: React and Vite, styled with the IBM Carbon Design System for a clean, clinical, and functional interface.
- **Backend**: C++ using the Drogon framework. This was chosen to study performance-oriented web development, memory management, and concurrency.
- **Database**: PostgreSQL, used to explore data integrity, transaction isolation, and indexing strategies.
- **Infrastructure**: Docker and Docker Compose for consistent local development environments.

## Engineering Concepts Explored

Focused heavily on identifying and fixing common web vulnerabilities (OWASP Top 10) to improve the application's resilience:

- **Concurrency Control**: Implemented PostgreSQL Advisory Locks (`pg_advisory_xact_lock`) to prevent Time-of-Check to Time-of-Use (TOCTOU) race conditions that could lead to account overdrafts during simultaneous P2P transfers.
- **Proxy-Aware Rate Limiting**: Updated rate limiters to respect `X-Forwarded-For` headers, preventing self-inflicted Denial of Service (DoS) when deployed behind a reverse proxy.
- **Authentication Hygiene**: Enforced password re-authentication for sensitive account actions (e.g., disabling MFA).
- **Secrets Management**: Removed hardcoded secrets from the codebase and Docker configurations, migrating them to secure `.env` files.

## Setup and Installation

The backend relies on C++ and Drogon, which can take time to compile and configure manually. To simplify this, the application is containerized using Docker.

### Prerequisites
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) (Required to run the PostgreSQL database and the Drogon C++ server)
- [Node.js](https://nodejs.org/) (Version 18+)
- [pnpm](https://pnpm.io/) (Package manager)

### Running the Project (Windows)

The repository includes several batch files to make local setup straightforward. Before starting, ensure Docker Desktop is running.

**1. First-Time Setup**  
If this is your first time running the project, use the install script. It will install all Node.js dependencies and spin up the Docker containers (Database and Backend).
```cmd
install-run.bat
```
*Note: The first time you start the Docker containers, it may take several minutes for the Drogon backend to compile from source.*

**2. Standard Development Run**  
For everyday development, use the run script. It assumes your dependencies are already installed and simply starts the frontend and backend development servers.
```cmd
run.bat
```

**3. Production Build**  
To compile the React UI for production and serve it directly from the Drogon C++ server (acting as a single monolithic service), use the build script.
```cmd
build-prod.bat
```

## Future Small-Scale Additions

As a hobby learning project, future updates will focus on small, manageable features that provide opportunities to learn new patterns:
- Exporting transaction history to CSV format.
- Adding basic spending categorization (e.g., Food, Utilities, Entertainment).
- Fully containerizing the React frontend to run alongside the backend in a unified Docker network.
- Implementing Redis for faster session token validation.
