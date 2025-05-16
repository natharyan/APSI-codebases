# JWT Tokens

When to change the `isValid` key or delete the token:

### 1. **Token Expiry or Refresh**:

- **Scenario**: When a token has expired or is close to expiration, the user requests a new one (typically via a refresh token). You can either:
  - Mark the old token as invalid (`isValid: false`).
  - **Or** delete the expired token from the database to minimize storage.
- **Action**: Set `isValid` to `false` or delete the expired token.
- **Why**: Expired tokens should no longer be valid for security reasons, and deleting them reduces storage.

### 2. **Logout**:

- **Scenario**: When a user logs out of the system, the associated access token or refresh token should be invalidated so it can’t be used again.
- **Action**: Set `isValid` to `false` (or delete the token if you want to minimize storage).
- **Why**: Logging out should invalidate all active tokens for security and session management purposes.

### 3. **Session Hijacking or Compromise**:

- **Scenario**: If there is a report or detection of session hijacking (such as multiple suspicious login attempts), all tokens associated with the user or session might need to be invalidated for security reasons.
- **Action**: Set `isValid` to `false` for all tokens or delete them from the database.
- **Why**: Compromised tokens should not be used, so invalidating or removing them helps secure the user’s account.

### 4. **Device Logout or Session Revocation**:

- **Scenario**: If a user logs out from a specific device (e.g., mobile, laptop), you may want to invalidate only the token associated with that session or device while keeping others active.
- **Action**: Set `isValid` to `false` for that particular token (or delete it from the database).
- **Why**: Invalidating the token ensures that a logged-out session can’t be reactivated, maintaining security for the other active devices.

### 5. **Token Rotation (Replacing Refresh Tokens)**:

- **Scenario**: When a new refresh token is issued, the old one can be invalidated (or deleted) to prevent the reuse of refresh tokens. This prevents replay attacks, where someone tries to use an old refresh token to gain access.
- **Action**: Invalidate (`isValid: false`) or delete the old refresh token after issuing the new one.
- **Why**: Token rotation ensures only the most recent token is valid, preventing reuse of old tokens.

### 6. **Manual Revocation by Admin or User**:

- **Scenario**: An admin or the user may choose to manually revoke a token for any reason, such as when a user suspects unauthorized access or wants to end a session early.
- **Action**: Set `isValid` to `false` for the token (or delete it if preferred).
- **Why**: Revoking access tokens immediately prevents further access from that session.

### 7. **User Account Deletion**:

- **Scenario**: If a user deletes their account, all associated tokens should be invalidated (or deleted) to ensure no further access to the system.
- **Action**: Delete all tokens related to the user from the database.
- **Why**: Account deletion should revoke all access to any system resources.

---

### Summary of When to Update `isValid` or Delete Tokens:

- **Update `isValid` to false**:

  - Token expiration.
  - User logout (for specific session or device).
  - Session hijacking or account compromise.
  - Token refresh (rotation).
  - Manual token revocation.
- **Delete tokens from the database** (to minimize storage):

  - Token expiration (instead of invalidation).
  - User logout (if you don’t need to keep the invalidated token).
  - Issuing a new refresh token (delete the old one).
  - User account deletion (delete all tokens).

In most scenarios where tokens are no longer needed (expired, user logged out, etc.), deleting them from the database can be a good practice to minimize storage, especially if there’s no need to retain them for audit or logging purposes. However, keeping invalidated tokens may be useful for a period (e.g., for logging or forensic purposes).
