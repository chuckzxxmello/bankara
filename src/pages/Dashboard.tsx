import { useState, useEffect } from 'react';
import { Tile, Button, Form, TextInput, InlineNotification, Stack } from '@carbon/react';
import { useAuth } from '../context/AuthContext';
import './Dashboard.css';

export function Dashboard() {
  const [balance, setBalance] = useState<string>('0.00');
  const [sendEmail, setSendEmail] = useState('');
  const [sendAmount, setSendAmount] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');

  const { email } = useAuth();

  const fetchLedger = async () => {
    try {
      const balRes = await fetch('/api/balance', {
        credentials: 'same-origin'
      });
      if (balRes.ok) {
        const balData = await balRes.json();
        setBalance(balData.balance);
      }
    } catch (e) {
      setError('Failed to fetch ledger data');
    }
  };

  useEffect(() => {
    fetchLedger();
  }, []);

  const handleSend = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    try {
      const res = await fetch('/api/transfer', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ email: sendEmail, amount: sendAmount }),
        credentials: 'same-origin'
      });
      
      if (res.ok) {
        const data = await res.json();
        setSuccess(`Successfully sent $${sendAmount} to ${sendEmail}!`);
        setSendEmail('');
        setSendAmount('');
        fetchLedger();
      } else {
        const text = await res.text();
        try {
          const data = JSON.parse(text);
          setError(data.error || data.body || 'Transfer failed.');
        } catch {
          setError(text || 'Transfer failed. Check balance or email.');
        }
      }
    } catch (e) {
      setError('Network error during transfer.');
    }
  };

  const requestDemoFunds = async () => {
    try {
      const res = await fetch('/api/demo-funds', {
        method: 'POST',
        credentials: 'same-origin'
      });
      if (res.ok) {
        setSuccess('Minted $100 Demo Funds!');
        fetchLedger();
      }
    } catch (e) {
      setError('Failed to request funds.');
    }
  };

  return (
    <div className="dashboard-page">
      <h2 className="welcome-header">Welcome, {email}</h2>
      <p className="overview-subtitle">Here's your account overview</p>
      
      <Tile style={{ marginBottom: '2rem', backgroundColor: '#0062ff', color: 'white' }}>
        <p style={{ fontSize: '1rem', opacity: 0.9 }}>Current Balance</p>
        <h1 style={{ fontSize: '4rem', fontWeight: 600, margin: '1rem 0' }}>${balance}</h1>
        <Button kind="secondary" onClick={requestDemoFunds} style={{ marginTop: '1rem' }}>
          Get $100 Demo Funds
        </Button>
      </Tile>

      <div className="dashboard-grid">
        <Tile>
          <h3 style={{ marginBottom: '1.5rem', fontWeight: 600 }}>Quick Send</h3>
          {error && <InlineNotification kind="error" title="Error" subtitle={error} onClose={() => setError('')} />}
          {success && <InlineNotification kind="success" title="Success" subtitle={success} onClose={() => setSuccess('')} />}
          <Form onSubmit={handleSend}>
            <Stack gap={5}>
              <TextInput
                id="send-email"
                labelText="Recipient Email"
                placeholder="friend@example.com"
                value={sendEmail}
                onChange={(e) => setSendEmail(e.target.value)}
                required
              />
              <TextInput
                id="send-amount"
                labelText="Amount ($)"
                placeholder="0.00"
                type="number"
                step="0.01"
                min="0.01"
                value={sendAmount}
                onChange={(e) => setSendAmount(e.target.value)}
                required
              />
              <Button type="submit">Send Money</Button>
            </Stack>
          </Form>
        </Tile>

        <Tile>
          <h3 style={{ marginBottom: '1.5rem', fontWeight: 600 }}>Recent Activity</h3>
          <p style={{ marginBottom: '1.5rem' }}>
            Check your recent transactions to see where your money is going.
            Head over to the Transactions tab for a detailed ledger view.
          </p>
          <Button kind="secondary" size="md" onClick={() => window.location.href='/transactions'}>
            View All Transactions
          </Button>
        </Tile>
      </div>
    </div>
  );
}
