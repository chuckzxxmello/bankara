@echo off
echo ===============================================
echo Bankara Wallet - Setup and Run Script (Docker)
echo ===============================================
echo.

echo [1/3] Checking environment variables...
if not exist .env (
    echo Creating .env file from template...
    copy .env.example .env
    echo Please update the .env file with your own secrets if necessary.
) else (
    echo .env file already exists, skipping.
)
echo.

echo [2/3] Installing Node.js dependencies...
call pnpm install

echo.
echo [3/3] Starting Bankara Wallet Frontend and Backend Container...
echo ==============================================================
call pnpm run dev:all

pause
