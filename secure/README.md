# Secure Credentials Folder

This folder contains sensitive AWS IoT Core certificates and credentials that should **NEVER** be committed to version control.

## Files in this folder:
- `aws_certificates.h` - Contains AWS IoT certificates and keys

## Important Security Notes:

⚠️ **WARNING**: This folder is excluded from git via .gitignore to protect your sensitive credentials.

### Files that belong here:
- AWS IoT device certificates (.crt)
- AWS private keys (.key)
- Any other sensitive configuration files

### Setup Instructions:
1. Copy your AWS IoT certificates to `aws_certificates.h`
2. Update the certificate content in the file
3. **Never** remove this folder from .gitignore
4. **Never** commit this folder to any public repository

### If you accidentally commit certificates:
1. Immediately revoke the certificates in AWS IoT Console
2. Generate new certificates
3. Remove the committed files from git history
4. Update your device with new certificates

## For Team Development:
- Each developer should have their own certificates
- Share setup instructions, not the actual certificates
- Use environment-specific certificates (dev/staging/prod)
- Consider using AWS IAM roles and temporary credentials for development

## Backup:
- Keep certificates backed up securely outside of git
- Consider using password managers or secure vaults
- Document certificate expiration dates