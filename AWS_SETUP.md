# AWS IoT Core Setup Guide

## Prerequisites
1. AWS Account with IoT Core access
2. AWS CLI installed (optional but recommended)

## Step 1: Create AWS IoT Thing

1. Go to AWS IoT Core Console
2. Navigate to "Manage" > "Things"
3. Click "Create things" > "Create single thing"
4. Enter thing name: `PressureLogger`
5. Click "Next" and proceed with default settings

## Step 2: Generate Certificates

1. In the thing creation process, choose "Auto-generate a new certificate"
2. Download the following files:
   - `xxxxxxxxxx-certificate.pem.crt` (Device Certificate)
   - `xxxxxxxxxx-private.pem.key` (Private Key)
   - `xxxxxxxxxx-public.pem.key` (Public Key - not needed for this project)
3. Download the Amazon Root CA certificate from:
   - https://www.amazontrust.com/repository/AmazonRootCA1.pem

## Step 3: Update Certificate Files

1. Copy the template file:
   ```bash
   cp secure/aws_certificates.template.h secure/aws_certificates.h
   ```
2. Open `secure/aws_certificates.h`
3. Replace the placeholder certificates with your actual certificates:

### Device Certificate (.crt file)
Copy the contents of your `xxxxxxxxxx-certificate.pem.crt` file and paste it in the `device_cert` variable:
```c
const char* device_cert = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_DEVICE_CERTIFICATE_CONTENT_HERE
-----END CERTIFICATE-----
)EOF";
```

### Private Key (.key file)
Copy the contents of your `xxxxxxxxxx-private.pem.key` file and paste it in the `device_key` variable:
```c
const char* device_key = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
YOUR_PRIVATE_KEY_CONTENT_HERE
-----END RSA PRIVATE KEY-----
)EOF";
```

## Step 4: Update Configuration

1. Open `src/main.cpp`
2. Update the following variables:
   - `ssid`: Your WiFi network name
   - `password`: Your WiFi password
   - `aws_iot_endpoint`: Your AWS IoT endpoint (found in IoT Core Console > Settings)
   - `thing_name`: Should match your AWS IoT Thing name

Example:
```c
const char* ssid = "MyWiFiNetwork";
const char* password = "MyWiFiPassword";
const char* aws_iot_endpoint = "a1b2c3d4e5f6g7-ats.iot.us-east-1.amazonaws.com";
const char* thing_name = "PressureLogger";
```

## Step 5: Create IoT Policy

1. Go to AWS IoT Core Console > "Secure" > "Policies"
2. Click "Create policy"
3. Name: `PressureLoggerPolicy`
4. Add the following policy document:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect",
        "iot:Publish"
      ],
      "Resource": [
        "arn:aws:iot:YOUR_REGION:YOUR_ACCOUNT_ID:client/PressureLogger",
        "arn:aws:iot:YOUR_REGION:YOUR_ACCOUNT_ID:topic/pressure_logger/data"
      ]
    }
  ]
}
```

## Step 6: Attach Policy to Certificate

1. Go to "Secure" > "Certificates"
2. Select your certificate
3. Click "Actions" > "Attach policy"
4. Select `PressureLoggerPolicy`
5. Click "Actions" > "Attach thing"
6. Select `PressureLogger`

## Step 7: Test Connection

1. Upload the code to your M5Stack
2. Monitor the Serial output for connection status
3. Check AWS IoT Core Console > "Test" > "MQTT test client"
4. Subscribe to topic: `pressure_logger/data`
5. You should see JSON messages with sensor data

## Data Format

The device publishes data in the following JSON format:
```json
{
  "timestamp": 123456789,
  "device": "PressureLogger",
  "ch0": 5.234,
  "ch1": 3.567
}
```

## Troubleshooting

- **Certificate errors**: Ensure certificates are correctly copied with proper BEGIN/END markers
- **Connection timeouts**: Check your WiFi credentials and AWS endpoint
- **Permission denied**: Verify the IoT policy allows publish to your topic
- **SSL handshake failed**: Ensure you're using the correct Root CA certificate

## Security Notes

- The `secure/` folder is excluded from git via .gitignore to protect your credentials
- Keep your private key secure and never commit it to public repositories  
- Each developer should have their own certificates for development
- Consider using AWS IoT Device Management for production deployments
- Regularly rotate certificates for enhanced security
- If certificates are accidentally committed, immediately revoke them in AWS Console

## Repository Security

This project uses a secure folder structure:
- `secure/aws_certificates.h` - Contains actual certificates (gitignored)
- `secure/README.md` - Security documentation

The entire `secure/` folder is protected by .gitignore to prevent accidental commits of sensitive credentials.