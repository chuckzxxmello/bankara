import { useState, useEffect } from 'react';
import { Button, TextInput, PasswordInput, InlineNotification, Modal } from '@carbon/react';
import { QRCodeSVG } from 'qrcode.react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

export function Settings() {
  const [mfaEnabled, setMfaEnabled] = useState(false);
  const [mfaUri, setMfaUri] = useState('');
  const [mfaSecret, setMfaSecret] = useState('');
  const [mfaCode, setMfaCode] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [showDeleteModal, setShowDeleteModal] = useState(false);
  const [deleteAccountPassword, setDeleteAccountPassword] = useState('');
  
  const [showDisableMfaModal, setShowDisableMfaModal] = useState(false);
  const [disableMfaPassword, setDisableMfaPassword] = useState('');
  const navigate = useNavigate();
  const { email, email_verified, verifyEmail, logout } = useAuth();

  // Change Password state
  const [currentPassword, setCurrentPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmNewPassword, setConfirmNewPassword] = useState('');

  // Use email in keys to make them user-specific
  const nameKey = `bankara_profile_name_${email || 'default'}`;
  const addrKey = `bankara_profile_address_${email || 'default'}`;

  const [profileName, setProfileName] = useState('');
  const [profileAddress, setProfileAddress] = useState('');

  useEffect(() => {
    setProfileName(localStorage.getItem(nameKey) || '');
    setProfileAddress(localStorage.getItem(addrKey) || '');
  }, [nameKey, addrKey]);

  useEffect(() => {
    const fetchProfile = async () => {
      try {
        const res = await fetch('/api/auth/profile', {
          credentials: 'same-origin'
        });
        if (res.ok) {
          const data = await res.json();
          setMfaEnabled(data.mfa_enabled);
          if (data.email_verified && !email_verified) {
            verifyEmail();
          }
        }
      } catch (e) {
        console.error("Failed to fetch profile", e);
      }
    };
    fetchProfile();
  }, [email_verified, verifyEmail]);

  const handleResendVerification = async () => {
    setError('');
    setSuccess('');
    try {
      const res = await fetch('/api/auth/resend-verification', {
        method: 'POST',
        credentials: 'same-origin'
      });
      if (res.ok) {
        setSuccess('Verification link resent. Please check the server console.');
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
  };

  const handleSetupMfa = async () => {
    setError('');
    setSuccess('');
    try {
      const res = await fetch('/api/mfa/setup', {
        method: 'POST',
        credentials: 'same-origin'
      });
      if (res.ok) {
        const data = await res.json();
        setMfaUri(data.uri);
        setMfaSecret(data.secret);
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
  };

  const handleVerifyMfa = async () => {
    setError('');
    try {
      const res = await fetch('/api/mfa/verify', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code: mfaCode }),
        credentials: 'same-origin'
      });
      if (res.ok) {
        setSuccess('MFA has been successfully enabled!');
        setMfaEnabled(true);
        setMfaUri('');
        setMfaSecret('');
        setMfaCode('');
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
  };

  const handleDisableMfa = async () => {
    if (!disableMfaPassword) {
      setError('Password is required to disable MFA.');
      setShowDisableMfaModal(false);
      return;
    }
    setError('');
    setSuccess('');
    try {
      const res = await fetch('/api/mfa/disable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password: disableMfaPassword }),
        credentials: 'same-origin'
      });
      if (res.ok) {
        setSuccess('MFA has been successfully disabled.');
        setMfaEnabled(false);
        setDisableMfaPassword('');
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
    setShowDisableMfaModal(false);
  };

  const handleDeleteAccount = async () => {
    if (!deleteAccountPassword) {
      setError('Password is required to delete your account.');
      setShowDeleteModal(false);
      return;
    }
    try {
      const res = await fetch('/api/auth/delete-account', {
        method: 'DELETE',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password: deleteAccountPassword }),
        credentials: 'same-origin'
      });
      if (res.ok) {
        if (email) {
          localStorage.removeItem(`bankara_profile_name_${email}`);
          localStorage.removeItem(`bankara_profile_address_${email}`);
        }
        logout();
        navigate('/');
      } else if (res.status === 429) {
        // Locked out due to too many failed attempts
        logout();
        window.location.href = '/login';
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
    setShowDeleteModal(false);
  };

  const handleSaveProfile = () => {
    localStorage.setItem(nameKey, profileName);
    localStorage.setItem(addrKey, profileAddress);
    setSuccess('Profile updated successfully.');
  };

  const handleChangePassword = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    if (newPassword !== confirmNewPassword) {
      setError('New passwords do not match.');
      return;
    }

    if (newPassword.length < 8) {
      setError('New password must be at least 8 characters long.');
      return;
    }
    if (!/[A-Z]/.test(newPassword) || !/[^a-zA-Z0-9]/.test(newPassword)) {
      setError('New password must contain at least one uppercase letter and one special character.');
      return;
    }

    try {
      const res = await fetch('/api/auth/request-password-change', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ current_password: currentPassword, new_password: newPassword }),
        credentials: 'same-origin'
      });
      if (res.ok) {
        const data = await res.json();
        setSuccess(data.message || 'Please check the server console for the confirmation link.');
        setCurrentPassword('');
        setNewPassword('');
        setConfirmNewPassword('');
      } else {
        const text = await res.text();
        setError(text);
      }
    } catch (e) {
      setError('Network error');
    }
  };

  return (
    <div>
      <h2 style={{ marginBottom: '2rem', color: '#161616' }}>Account Settings</h2>
      
      {error && <InlineNotification kind="error" title="Error" subtitle={error} onClose={() => setError('')} />}
      {success && <InlineNotification kind="success" title="Success" subtitle={success} onClose={() => setSuccess('')} />}

      {!email_verified && (
        <div style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px', maxWidth: '600px', marginBottom: '2rem', borderLeft: '4px solid #f1c21b' }}>
          <h3>Email Verification</h3>
          <p style={{ marginTop: '1rem', color: '#525252' }}>
            Your email address is currently unverified. Some features may be restricted. 
            Please verify your email address to fully secure your account.
          </p>
          <Button kind="tertiary" onClick={handleResendVerification} style={{ marginTop: '1.5rem' }}>
            Resend Verification Email
          </Button>
        </div>
      )}

      <div style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px', maxWidth: '600px', marginBottom: '2rem' }}>
        <h3>Personal Information</h3>
        <p style={{ marginTop: '1rem', color: '#525252', marginBottom: '1.5rem' }}>
          Update your basic profile information.
        </p>
        
        <TextInput
          id="profile-name"
          labelText="Full Name"
          placeholder="Jane Doe"
          value={profileName}
          onChange={(e) => setProfileName(e.target.value)}
          style={{ marginBottom: '1rem' }}
        />
        <TextInput
          id="profile-address"
          labelText="Address"
          placeholder="123 Main St, City, Country"
          value={profileAddress}
          onChange={(e) => setProfileAddress(e.target.value)}
          style={{ marginBottom: '1.5rem' }}
        />
        <Button onClick={handleSaveProfile}>Save Profile</Button>
      </div>

      {/* Change Password Section */}
      <div style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px', maxWidth: '600px', marginBottom: '2rem' }}>
        <h3>Change Password</h3>
        <p style={{ marginTop: '1rem', color: '#525252', marginBottom: '1.5rem' }}>
          A confirmation link will be sent to the server console. Your password will only change after you click it.
        </p>
        <form onSubmit={handleChangePassword}>
          <PasswordInput
            id="current-password"
            labelText="Current Password"
            placeholder="Enter current password"
            value={currentPassword}
            onChange={(e) => setCurrentPassword(e.target.value)}
            required
            style={{ marginBottom: '1rem' }}
          />
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
            id="confirm-new-password"
            labelText="Confirm New Password"
            placeholder="Re-enter new password"
            value={confirmNewPassword}
            onChange={(e) => setConfirmNewPassword(e.target.value)}
            required
            style={{ marginBottom: '1.5rem' }}
          />
          <Button type="submit">Request Password Change</Button>
        </form>
      </div>

      <div style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px', maxWidth: '600px', marginBottom: '2rem' }}>
        <h3>Security Options</h3>
        <p style={{ marginTop: '1rem', color: '#525252' }}>
          Enable Multi-Factor Authentication (MFA) via Google Authenticator or Authy to secure your account.
        </p>
        
        {!mfaEnabled && !mfaUri && (
          <Button kind="secondary" onClick={handleSetupMfa} style={{ marginTop: '1.5rem' }}>
            Enable TOTP MFA
          </Button>
        )}

        {mfaEnabled && (
          <div style={{ marginTop: '1.5rem' }}>
            <InlineNotification kind="info" title="MFA Enabled" subtitle="Your account is protected by 2-step verification." hideCloseButton />
            <Button kind="danger" onClick={() => setShowDisableMfaModal(true)} style={{ marginTop: '1rem' }}>
              Disable TOTP MFA
            </Button>
          </div>
        )}

        {mfaUri && (
          <div style={{ marginTop: '2rem', padding: '1rem', backgroundColor: '#f4f7fb', borderRadius: '4px' }}>
            <h4>Scan this QR Code</h4>
            <p style={{ fontSize: '0.9rem', marginBottom: '1rem' }}>Open Google Authenticator and scan the code below:</p>
            <div style={{ display: 'flex', justifyContent: 'center', marginBottom: '1rem', backgroundColor: '#fff', padding: '1rem' }}>
              <QRCodeSVG value={mfaUri} size={200} />
            </div>
            <p style={{ fontSize: '0.9rem', fontFamily: 'monospace', textAlign: 'center', marginBottom: '1rem' }}>Secret: {mfaSecret}</p>
            
            <TextInput
              id="mfa-verify-code"
              labelText="Verification Code"
              placeholder="123456"
              value={mfaCode}
              onChange={(e) => setMfaCode(e.target.value)}
              style={{ marginBottom: '1rem' }}
            />
            <Button onClick={handleVerifyMfa}>Verify and Enable</Button>
            <Button kind="ghost" onClick={() => setMfaUri('')} style={{ marginLeft: '1rem' }}>Cancel</Button>
          </div>
        )}
      </div>

      <div style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px', maxWidth: '600px', borderLeft: '4px solid #da1e28' }}>
        <h3 style={{ color: '#da1e28' }}>Danger Zone</h3>
        <p style={{ marginTop: '1rem', color: '#525252' }}>
          Permanently delete your account and wipe all your transactions from the ledger. This action cannot be undone.
        </p>
        <Button kind="danger" onClick={() => setShowDeleteModal(true)} style={{ marginTop: '1.5rem' }}>
          Delete Account Permanently
        </Button>
      </div>

      <Modal
        open={showDeleteModal}
        danger
        modalHeading="Delete Account"
        primaryButtonText="Delete Permanently"
        secondaryButtonText="Cancel"
        onRequestSubmit={handleDeleteAccount}
        onRequestClose={() => setShowDeleteModal(false)}
      >
        <p style={{ marginBottom: '1rem' }}>
          Are you sure you want to delete your account? This action cannot be undone. All your data and balances will be lost.
          Please enter your password to confirm.
        </p>
        <PasswordInput
          id="delete-account-password"
          labelText="Password"
          placeholder="Enter your password"
          value={deleteAccountPassword}
          onChange={(e) => setDeleteAccountPassword(e.target.value)}
        />
      </Modal>

      <Modal
        open={showDisableMfaModal}
        danger
        modalHeading="Disable Multi-Factor Authentication"
        primaryButtonText="Disable MFA"
        secondaryButtonText="Cancel"
        onRequestSubmit={handleDisableMfa}
        onRequestClose={() => setShowDisableMfaModal(false)}
      >
        <p style={{ marginBottom: '1rem' }}>
          Disabling MFA will make your account less secure. Please enter your password to confirm.
        </p>
        <PasswordInput
          id="disable-mfa-password"
          labelText="Password"
          placeholder="Enter your password"
          value={disableMfaPassword}
          onChange={(e) => setDisableMfaPassword(e.target.value)}
        />
      </Modal>
    </div>
  );
}
