# Production Setup: Serving React Natively via Drogon

Your architecture has been upgraded! Previously, the React UI was hosted by a separate Node.js (Vite) server running on `localhost:3000`. 

Now, you have a true "Production Setup" where the Drogon C++ server itself serves your entire application.

## Changes Made

### 1. Drogon Configuration (`config.json`)
- **Maximum Concurrency Enabled:** Changed `"threads_num"` from `1` to `0`. Drogon will now automatically detect your CPU's hardware threads and distribute connections across all of them natively!
- **Document Root:** Added `"document_root": "./dist"`. This tells Drogon's web server where to look for static HTML/JS/CSS files.

### 2. Docker Networking (`docker-compose.yml`)
- Mounted the locally built `./dist` folder into the Docker container. This allows the internal C++ server to read the React files that were compiled on your Windows machine.

### 3. Build Script (`build-prod.bat`)
- Created a brand new script that orchestrates the entire deployment. It automatically compiles your React UI into a highly optimized static `dist` bundle, and then starts up the Drogon server.

## How to use it

You now have two ways to run your application:

**1. Development Mode (`run.bat`)**
Use this when you are actively writing React code. It spins up the `localhost:3000` Vite server so you get instant Hot-Reloading when you save a file.

**2. Production Mode (`build-prod.bat`)**
Use this when you want to run the app "for real". It compiles everything down and runs purely through the C++ server.

> [!TIP]
> After running `build-prod.bat`, open your browser to **http://localhost:5150**. You will see your beautiful React UI running natively without Node.js, served completely concurrently by your C++ backend!
