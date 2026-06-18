import React, { createContext, useContext, useState, useEffect } from 'react';

interface AuthState {
  isAuthenticated: boolean;
  email: string | null;
  email_verified: boolean;
  isLoading: boolean;
}

interface AuthContextType extends AuthState {
  login: (email: string, email_verified?: boolean) => void;
  logout: () => void;
  verifyEmail: () => void;
  refreshProfile: () => Promise<void>;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export function AuthProvider({ children }: { children: React.ReactNode }) {
  const [authState, setAuthState] = useState<AuthState>({
    isAuthenticated: false,
    email: null,
    email_verified: true,
    isLoading: true,
  });

  const refreshProfile = async () => {
    try {
      const res = await fetch('/api/auth/profile', {
        credentials: 'same-origin'
      });
      if (res.ok) {
        const data = await res.json();
        setAuthState({
          isAuthenticated: true,
          email: data.email,
          email_verified: data.email_verified,
          isLoading: false,
        });
        return;
      }
    } catch (e) {
      console.error("Failed to fetch profile", e);
    }
    setAuthState(prev => ({ ...prev, isAuthenticated: false, isLoading: false }));
  };

  useEffect(() => {
    refreshProfile();
  }, []);

  const login = (email: string, email_verified: boolean = true) => {
    setAuthState({
      isAuthenticated: true,
      email,
      email_verified,
      isLoading: false,
    });
  };

  const logout = async () => {
    try {
      await fetch('/api/auth/logout', {
        method: 'POST',
        credentials: 'same-origin'
      });
    } catch (e) {
      // Ignore network errors during logout
    }
    setAuthState({
      isAuthenticated: false,
      email: null,
      email_verified: false,
      isLoading: false,
    });
  };

  const verifyEmail = () => {
    setAuthState(prev => ({ ...prev, email_verified: true }));
  };

  return (
    <AuthContext.Provider value={{ ...authState, login, logout, verifyEmail, refreshProfile }}>
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth() {
  const context = useContext(AuthContext);
  if (context === undefined) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
}
