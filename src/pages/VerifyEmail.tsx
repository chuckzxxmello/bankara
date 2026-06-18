import { useEffect, useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { InlineNotification, Button } from '@carbon/react';

export function VerifyEmail() {
  const [searchParams] = useSearchParams();
  const [status, setStatus] = useState<'loading' | 'success' | 'error'>('loading');
  const [errorMsg, setErrorMsg] = useState('');
  const navigate = useNavigate();

  useEffect(() => {
    const token = searchParams.get('token');
    if (!token) {
      setStatus('error');
      setErrorMsg('No token provided in URL');
      return;
    }

    const verify = async () => {
      try {
        const res = await fetch('/api/auth/verify-email', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ token })
        });

        if (res.ok) {
          setStatus('success');
        } else {
          const text = await res.text();
          setStatus('error');
          setErrorMsg(text || 'Verification failed');
        }
      } catch (e) {
        setStatus('error');
        setErrorMsg('Network error');
      }
    };

    verify();
  }, [searchParams]);

  return (
    <div style={{ minHeight: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center', backgroundColor: '#f4f7fb' }}>
      <div style={{ backgroundColor: '#fff', padding: '3rem', borderRadius: '8px', maxWidth: '400px', textAlign: 'center' }}>
        <h1 style={{ marginBottom: '1.5rem', color: '#161616' }}>Bankara</h1>
        
        {status === 'loading' && <p>Verifying your email...</p>}
        
        {status === 'success' && (
          <div>
            <InlineNotification kind="success" title="Success!" subtitle="Your email has been verified." hideCloseButton style={{ marginBottom: '2rem' }} />
            <Button onClick={() => navigate('/')}>Return to Login</Button>
          </div>
        )}

        {status === 'error' && (
          <div>
            <InlineNotification kind="error" title="Verification Failed" subtitle={errorMsg} hideCloseButton style={{ marginBottom: '2rem' }} />
            <Button onClick={() => navigate('/')} kind="secondary">Return to Login</Button>
          </div>
        )}
      </div>
    </div>
  );
}
