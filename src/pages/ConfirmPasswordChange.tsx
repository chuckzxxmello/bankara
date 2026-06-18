import { useEffect, useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { InlineNotification, Button } from '@carbon/react';
import { useAuth } from '../context/AuthContext';

export function ConfirmPasswordChange() {
  const [searchParams] = useSearchParams();
  const [status, setStatus] = useState<'loading' | 'success' | 'error'>('loading');
  const [errorMsg, setErrorMsg] = useState('');
  const navigate = useNavigate();
  const { isAuthenticated } = useAuth();

  useEffect(() => {
    const token = searchParams.get('token');
    if (!token) {
      setStatus('error');
      setErrorMsg('No token provided in URL');
      return;
    }

    const confirm = async () => {
      try {
        const res = await fetch('/api/auth/confirm-password-change', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ token })
        });

        if (res.ok) {
          setStatus('success');
        } else {
          const text = await res.text();
          setStatus('error');
          setErrorMsg(text || 'Confirmation failed');
        }
      } catch (e) {
        setStatus('error');
        setErrorMsg('Network error');
      }
    };

    confirm();
  }, [searchParams]);

  return (
    <div style={{ minHeight: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center', backgroundColor: '#f4f7fb' }}>
      <div style={{ backgroundColor: '#fff', padding: '3rem', borderRadius: '8px', maxWidth: '400px', textAlign: 'center' }}>
        <h1 style={{ marginBottom: '1.5rem', color: '#161616' }}>Bankara</h1>
        
        {status === 'loading' && <p>Confirming password change...</p>}
        
        {status === 'success' && (
          <div>
            <InlineNotification kind="success" title="Success!" subtitle="Your password has been changed." hideCloseButton style={{ marginBottom: '2rem' }} />
            <Button onClick={() => navigate(isAuthenticated ? '/settings' : '/login')}>
              {isAuthenticated ? 'Return to Settings' : 'Return to Login'}
            </Button>
          </div>
        )}

        {status === 'error' && (
          <div>
            <InlineNotification kind="error" title="Failed" subtitle={errorMsg} hideCloseButton style={{ marginBottom: '2rem' }} />
            <Button onClick={() => navigate(isAuthenticated ? '/settings' : '/login')} kind="secondary">
              {isAuthenticated ? 'Return to Settings' : 'Return to Login'}
            </Button>
          </div>
        )}
      </div>
    </div>
  );
}
