@echo off
echo ===============================================
echo Starting Bankara Wallet Frontend and Backend...
echo ===============================================

if not exist .env (
    echo Creating .env file from template...
    copy .env.example .env
)

call pnpm run dev:all

pause
