import { BrowserRouter, Routes, Route, Navigate, Outlet } from 'react-router-dom';
import { Layout } from '../components/layout/Layout';
import { DashboardLayout } from '../components/layout/DashboardLayout';
import { Home } from '../pages/Home';
import { Welcome } from '../pages/Welcome';
import { Dashboard } from '../pages/Dashboard';
import { Transactions } from '../pages/Transactions';
import { Settings } from '../pages/Settings';
import { VerifyEmail } from '../pages/VerifyEmail';
import { ResetPassword } from '../pages/ResetPassword';
import { ConfirmPasswordChange } from '../pages/ConfirmPasswordChange';
import { AuthProvider, useAuth } from '../context/AuthContext';

function ProtectedRoute({ children }: { children: React.ReactNode }) {
  const { isAuthenticated, isLoading } = useAuth();
  
  if (isLoading) {
    return <div>Loading...</div>;
  }
  
  return isAuthenticated ? <>{children}</> : <Navigate to="/login" />;
}

function LayoutWrapper() {
  return (
    <Layout>
      <Outlet />
    </Layout>
  );
}

function DashboardLayoutWrapper() {
  return (
    <ProtectedRoute>
      <DashboardLayout />
    </ProtectedRoute>
  );
}

export function AppRouter() {
  return (
    <AuthProvider>
      <BrowserRouter>
        <Routes>
          {/* Routes WITH the main layout (header, footer, etc.) */}
          <Route element={<LayoutWrapper />}>
            <Route path="/" element={<Home />} />
          </Route>

          {/* Routes WITHOUT the main layout (fullscreen components) */}
          <Route path="/login" element={<Welcome />} />
          <Route path="/verify-email" element={<VerifyEmail />} />
          <Route path="/reset-password" element={<ResetPassword />} />
          <Route path="/confirm-password-change" element={<ConfirmPasswordChange />} />
          
          {/* Protected Dashboard Routes */}
          <Route element={<DashboardLayoutWrapper />}>
            <Route path="/dashboard" element={<Dashboard />} />
            <Route path="/transactions" element={<Transactions />} />
            <Route path="/settings" element={<Settings />} />
          </Route>
        </Routes>
      </BrowserRouter>
    </AuthProvider>
  );
}
