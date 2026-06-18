import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { Form, TextInput, PasswordInput, Button, InlineNotification } from '@carbon/react';
import { useAuth } from '../context/AuthContext';
import './Welcome.css';

export function Welcome() {
  const [isLogin, setIsLogin] = useState(true);
  const [isMfaStep, setIsMfaStep] = useState(false);
  const [isForgotPassword, setIsForgotPassword] = useState(false);
  const [mfaToken, setMfaToken] = useState('');
  const [mfaCode, setMfaCode] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [lockoutTimeRemaining, setLockoutTimeRemaining] = useState<number>(0);
  const navigate = useNavigate();
  const { login } = useAuth();

  useEffect(() => {
    let timer: any;
    if (lockoutTimeRemaining > 0) {
      timer = setInterval(() => {
        setLockoutTimeRemaining((prev) => prev - 1);
      }, 1000);
    } else {
      setLockoutTimeRemaining(0);
    }
    return () => clearInterval(timer);
  }, [lockoutTimeRemaining]);

  const handleMfaSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    try {
      const response = await fetch('/api/auth/mfa-login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mfa_token: mfaToken, code: mfaCode }),
        credentials: 'same-origin'
      });

      if (response.ok) {
        const data = await response.json();
        login(data.email, data.email_verified !== false);
        navigate('/dashboard');
      } else {
        const text = await response.text();
        setError(text || 'Invalid MFA code');
      }
    } catch (err) {
      setError('Network error');
    }
  };

  const handleForgotPassword = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    if (!email) {
      setError('Please enter your email address.');
      return;
    }

    try {
      const response = await fetch('/api/auth/forgot-password', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email })
      });

      if (response.ok) {
        setSuccess('If an account with that email exists, a password reset link has been printed to the server console.');
      } else {
        const text = await response.text();
        setError(text || 'Failed to send reset email');
      }
    } catch (err) {
      setError('Network error');
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    if (!isLogin) {
      if (password.length < 8) {
        setError('Password must be at least 8 characters long.');
        return;
      }
      if (!/[A-Z]/.test(password) || !/[^a-zA-Z0-9]/.test(password)) {
        setError('Password must contain at least one uppercase letter and one special character.');
        return;
      }
    }

    const endpoint = isLogin ? '/api/auth/login' : '/api/auth/register';

    try {
      const response = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password }),
        credentials: 'same-origin'
      });

      if (response.ok) {
        const data = await response.json();
        if (data.status === 'mfa_required') {
          setMfaToken(data.mfa_token);
          setIsMfaStep(true);
        } else if (data.status === 'success' && data.email) {
          login(data.email, data.email_verified !== false);
          navigate('/dashboard');
        } else if (data.user_id) {
          // Registration successful
          setIsLogin(true);
          setEmail('');
          setPassword('');
          alert('Registration successful! Please check the server console for your verification link before logging in.');
        }
      } else if (response.status === 429) {
        const data = await response.json();
        setLockoutTimeRemaining(data.cooldown_remaining);
        setError(data.message || 'Too many failed attempts.');
      } else {
        const text = await response.text();
        try {
          const data = JSON.parse(text);
          setError(data.error || 'Authentication failed. Please try again.');
        } catch {
          setError(text || 'Authentication failed. Please try again.');
        }
      }
    } catch (err) {
      setError('Network error. Is the backend running?');
    }
  };

  // MFA Step
  if (isMfaStep) {
    return (
      <div className="welcome-container">
        <div className="welcome-card">
          <h1>Bankara</h1>
          <p className="welcome-subtitle">Enter your authenticator code</p>

          {error && <InlineNotification kind="error" title="Error" subtitle={error} onClose={() => setError('')} />}

          <Form onSubmit={handleMfaSubmit}>
            <TextInput
              id="mfa-code-input"
              labelText="6-digit Code"
              placeholder="123456"
              value={mfaCode}
              onChange={(e) => setMfaCode(e.target.value)}
              required
              className="form-input"
            />
            <Button type="submit" className="submit-button">
              Verify
            </Button>
            <Button kind="ghost" onClick={() => setIsMfaStep(false)} className="submit-button" style={{ marginTop: '1rem' }}>
              Cancel
            </Button>
          </Form>
        </div>
      </div>
    );
  }

  // Forgot Password Step
  if (isForgotPassword) {
    return (
      <div className="welcome-container">
        <div className="welcome-card">
          <Button
            kind="ghost"
            size="sm"
            onClick={() => setIsForgotPassword(false)}
            style={{ marginBottom: '1rem', paddingLeft: 0, paddingRight: 0 }}
          >
            ← back to Login
          </Button>
          <h1>Bankara</h1>
          <p className="welcome-subtitle">Reset your password</p>

          {error && <InlineNotification kind="error" title="Error" subtitle={error} onClose={() => setError('')} />}
          {success && <InlineNotification kind="success" title="Success" subtitle={success} onClose={() => setSuccess('')} />}

          <Form onSubmit={handleForgotPassword}>
            <TextInput
              id="forgot-email-input"
              labelText="Email Address"
              placeholder="name@example.com"
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              required
              className="form-input"
            />
            <Button type="submit" className="submit-button">
              Send Reset Link
            </Button>
          </Form>
        </div>
      </div>
    );
  }

  // Main Login/Register
  return (
    <div className="welcome-container">
      <div className="welcome-card">
        <Button
          kind="ghost"
          size="sm"
          onClick={() => navigate('/')}
          style={{ marginBottom: '1rem', paddingLeft: 0, paddingRight: 0 }}
        >
          ← go back to Home
        </Button>
        <img src="/bankara_logo.png" alt="Bankara Logo" style={{ height: '120px', display: 'block', margin: '0 auto 1rem auto' }} />
        <p className="welcome-subtitle">
          {isLogin ? 'Log in' : 'Create new account'}
        </p>

        {error && (
          <InlineNotification
            kind="error"
            title="Error"
            subtitle={error}
            onClose={() => setError('')}
          />
        )}

        <Form onSubmit={handleSubmit}>
          <TextInput
            id="email-input"
            labelText="Email Address"
            placeholder="name@example.com"
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            required
            className="form-input"
          />
          <PasswordInput
            id="password-input"
            labelText="Password"
            placeholder="Enter your password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
            helperText={!isLogin ? "Must be at least 8 chars, 1 uppercase, 1 special char" : undefined}
            className="form-input"
          />
          <Button 
            type="submit" 
            className="submit-button"
            disabled={lockoutTimeRemaining > 0}
          >
            {lockoutTimeRemaining > 0 
              ? `Locked. Try again in ${Math.floor(lockoutTimeRemaining / 60)}:${(lockoutTimeRemaining % 60).toString().padStart(2, '0')}` 
              : isLogin ? 'Log In' : 'Create Account'}
          </Button>
        </Form>

        {isLogin && (
          <div style={{ textAlign: 'center', marginTop: '0.5rem' }}>
            <Button kind="ghost" size="sm" onClick={() => setIsForgotPassword(true)}>
              Forgot Password?
            </Button>
          </div>
        )}

        <div className="toggle-mode">
          <Button
            kind="ghost"
            onClick={() => setIsLogin(!isLogin)}
          >
            {isLogin ? 'Need an account? Register here' : 'Already have an account? Log in'}
          </Button>
        </div>
      </div>
    </div>
  );
}
