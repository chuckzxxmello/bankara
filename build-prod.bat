@echo off
echo ===============================================
echo Starting Bankara Wallet (Production Mode)
echo ===============================================
echo.

if not exist .env (
    echo Creating .env file from template...
    copy .env.example .env
)

echo [1/2] Compiling React UI for Production...
call pnpm run build

echo.
echo [2/2] Starting Drogon C++ Server on port 5150...
echo (Drogon is now serving both the React UI and the REST API concurrently!)
echo =========================================================================

call pnpm run start:backend

echo.
echo Production server is running! Visit: http://localhost:5150
pause
