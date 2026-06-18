import { Link, useNavigate } from 'react-router-dom';
import {
  Header as CarbonHeader,
  HeaderName,
  HeaderGlobalBar,
  Button,
} from '@carbon/react';
import { useAuth } from '../../context/AuthContext';
import './Header.css';

export function Header() {
  const navigate = useNavigate();
  const { isAuthenticated } = useAuth();

  return (
    <CarbonHeader aria-label="Bankara Fintech Solutions">
      <HeaderName prefix="Bankara" element={Link as any} to="/">
        API Endpoint Tester
      </HeaderName>
      <HeaderGlobalBar>
        {isAuthenticated ? (
          <Button
            kind="primary"
            size="sm"
            onClick={() => navigate('/dashboard')}
          >
            Dashboard
          </Button>
        ) : (
          <Button
            kind="primary"
            size="sm"
            onClick={() => navigate('/login')}
          >
            Login
          </Button>
        )}
      </HeaderGlobalBar>
    </CarbonHeader>
  );
}
