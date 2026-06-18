import { useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { Form, PasswordInput, Button, InlineNotification } from '@carbon/react';

export function ResetPassword() {
  const [searchParams] = useSearchParams();
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState(false);
  const navigate = useNavigate();

  const token = searchParams.get('token');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    if (!token) {
      setError('No reset token provided in URL');
      return;
    }

    if (newPassword !== confirmPassword) {
      setError('Passwords do not match.');
      return;
    }

    if (newPassword.length < 8) {
      setError('Password must be at least 8 characters long.');
      return;
    }
    if (!/[A-Z]/.test(newPassword) || !/[^a-zA-Z0-9]/.test(newPassword)) {
      setError('Password must contain at least one uppercase letter and one special character.');
      return;
    }

    try {
      const res = await fetch('/api/auth/reset-password', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ token, new_password: newPassword })
      });

      if (res.ok) {
        setSuccess(true);
      } else {
        const text = await res.text();
        setError(text || 'Failed to reset password');
      }
    } catch (err) {
      setError('Network error');
    }
  };

  if (!token) {
    return (
      <div style={{ minHeight: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center', backgroundColor: '#f4f7fb' }}>
        <div style={{ backgroundColor: '#fff', padding: '3rem', borderRadius: '8px', maxWidth: '400px', textAlign: 'center' }}>
          <h1 style={{ marginBottom: '1.5rem', color: '#161616' }}>Bankara</h1>
          <InlineNotification kind="error" title="Error" subtitle="No reset token provided in URL." hideCloseButton style={{ marginBottom: '2rem' }} />
          <Button onClick={() => navigate('/login')}>Return to Login</Button>
        </div>
      </div>
    );
  }

  return (
    <div style={{ minHeight: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center', backgroundColor: '#f4f7fb' }}>
      <div style={{ backgroundColor: '#fff', padding: '3rem', borderRadius: '8px', maxWidth: '400px', width: '100%' }}>
        <h1 style={{ marginBottom: '0.5rem', color: '#161616', textAlign: 'center' }}>Bankara</h1>
        <p style={{ textAlign: 'center', color: '#525252', marginBottom: '2rem' }}>Set your new password</p>

        {error && <InlineNotification kind="error" title="Error" subtitle={error} onClose={() => setError('')} style={{ marginBottom: '1rem' }} />}

        {success ? (
          <div>
            <InlineNotification kind="success" title="Success!" subtitle="Your password has been reset." hideCloseButton style={{ marginBottom: '2rem' }} />
            <Button onClick={() => navigate('/login')} style={{ width: '100%' }}>Return to Login</Button>
          </div>
        ) : (
          <Form onSubmit={handleSubmit}>
            <PasswordInput
              id="new-password"
              labelText="New Password"
              placeholder="Enter new password"
              value={newPassword}
              onChange={(e) => setNewPassword(e.target.value)}
              required
              helperText="Must be at least 8 chars, 1 uppercase, 1 special char"
              style={{ marginBottom: '1rem' }}
            />
            <PasswordInput
              id="confirm-password"
              labelText="Confirm New Password"
              placeholder="Re-enter new password"
              value={confirmPassword}
              onChange={(e) => setConfirmPassword(e.target.value)}
              required
              style={{ marginBottom: '1.5rem' }}
            />
            <Button type="submit" style={{ width: '100%' }}>Reset Password</Button>
          </Form>
        )}
      </div>
    </div>
  );
}
