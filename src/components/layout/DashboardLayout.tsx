import { useState } from 'react';
import { Outlet, useNavigate, useLocation } from 'react-router-dom';
import {
  Header,
  HeaderName,
  HeaderGlobalBar,
  Button,
  SideNav,
  SideNavItems,
  SideNavLink,
  HeaderMenuButton
} from '@carbon/react';
import { Dashboard, ChartLineData, Settings } from '@carbon/icons-react';
import { useAuth } from '../../context/AuthContext';
import './DashboardLayout.css';

export function DashboardLayout() {
  const [isSideNavExpanded, setIsSideNavExpanded] = useState(true);
  const navigate = useNavigate();
  const location = useLocation();

  const { logout } = useAuth();

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  return (
    <div className="dashboard-container">
      <Header aria-label="Bankara Dashboard">
        <HeaderMenuButton
          aria-label="Open menu"
          isCollapsible
          onClick={() => setIsSideNavExpanded(!isSideNavExpanded)}
          isActive={isSideNavExpanded}
        />
        <HeaderName prefix="Bankara" onClick={() => navigate('/dashboard')}>
          Wallet
        </HeaderName>
        <HeaderGlobalBar>
          <Button kind="danger--ghost" size="sm" onClick={handleLogout}>
            Log Out
          </Button>
        </HeaderGlobalBar>
        
        <SideNav
          aria-label="Side navigation"
          expanded={isSideNavExpanded}
          isPersistent={false}
          onOverlayClick={() => setIsSideNavExpanded(false)}
        >
          <SideNavItems>
            <SideNavLink
              renderIcon={Dashboard}
              isActive={location.pathname === '/dashboard'}
              onClick={() => navigate('/dashboard')}
            >
              Dashboard
            </SideNavLink>
            <SideNavLink
              renderIcon={ChartLineData}
              isActive={location.pathname === '/transactions'}
              onClick={() => navigate('/transactions')}
            >
              Transactions
            </SideNavLink>
            <SideNavLink
              renderIcon={Settings}
              isActive={location.pathname === '/settings'}
              onClick={() => navigate('/settings')}
            >
              Settings
            </SideNavLink>
          </SideNavItems>
        </SideNav>
      </Header>

      <main className={`dashboard-content ${isSideNavExpanded ? 'content-expanded' : ''}`}>
        <Outlet />
      </main>
    </div>
  );
}
