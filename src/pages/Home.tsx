import { useNavigate } from 'react-router-dom';
import { Button } from '@carbon/react';

export function Home() {
  const navigate = useNavigate();

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', height: '80vh', textAlign: 'center', fontFamily: '"IBM Plex Sans", sans-serif' }}>
      <h1 style={{ fontSize: '3rem', color: '#0062ff', marginBottom: '1rem' }}>Bankara API Endpoint Tester</h1>
      <p style={{ fontSize: '1.2rem', color: '#525252', maxWidth: '600px', marginBottom: '2rem' }}>
        This is a basic frontend testing interface. It connects directly to the high-performance C++ Drogon REST API backend to test the digital ledger, authentication flows, and P2P transfers.
      </p>
      <Button size="lg" onClick={() => navigate('/login')}>
        Login
      </Button>
    </div>
  );
}
