# HTTPS Configuration Guide

This guide explains how to enable HTTPS support in cpp_cli_srv.

---

## Prerequisites

### 1. Install OpenSSL Development Libraries

The HTTPS feature requires OpenSSL development libraries to be installed:

| Distribution | Command |
|--------------|---------|
| **Ubuntu/Debian** | `sudo apt update && sudo apt install libssl-dev` |
| **CentOS/RHEL** | `sudo yum install openssl-devel` |
| **Fedora** | `sudo dnf install openssl-devel` |
| **Arch Linux** | `sudo pacman -S openssl` |

### 2. Verify OpenSSL Installation

```bash
# Check if OpenSSL development files are installed
pkg-config --modversion openssl

# Expected output: version number like 1.1.1 or 3.0.x
```

### 3. Rebuild with OpenSSL Support

If you previously built without OpenSSL, clean and rebuild:

```bash
rm -rf build
./build.sh
```

You should see:
```
-- OpenSSL found - HTTPS support enabled
```

---

## SSL Certificate Setup

### Option 1: Use Existing Certificates

If you already have SSL certificates (e.g., from Let's Encrypt or a CA):

1. Place your certificate files in a directory:
   ```bash
   mkdir -p ssl
   cp your_certificate.crt ssl/server.crt
   cp your_private_key.key ssl/server.key
   ```

2. Set proper permissions:
   ```bash
   chmod 600 ssl/server.key
   chmod 644 ssl/server.crt
   ```

3. Start the server:
   ```bash
   ./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
   ```

### Option 2: Generate Self-Signed Certificate (Development)

For development and testing, you can create a self-signed certificate:

```bash
# Create SSL directory
mkdir -p ssl

# Generate self-signed certificate (valid for 365 days)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"

# Set permissions
chmod 600 ssl/server.key
chmod 644 ssl/server.crt
```

**Parameters explained:**
- `-x509`: Generate a self-signed certificate
- `-newkey rsa:4096`: Create a new 4096-bit RSA key
- `-nodes`: Don't encrypt the private key (no passphrase)
- `-days 365`: Certificate valid for 1 year
- `-subj`: Certificate subject (customize as needed)

**For custom domain:**
```bash
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=California/L=San Francisco/O=MyCompany/CN=example.com"
```

**For multiple domains (SAN):**
```bash
# Create config file
cat > ssl/san.cnf << 'EOF'
[req]
default_bits = 4096
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[dn]
C=US
ST=California
L=San Francisco
O=MyCompany
CN=localhost

[v3_req]
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = example.com
DNS.3 = *.example.com
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

# Generate certificate with SAN
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -config ssl/san.cnf \
  -extensions v3_req
```

---

## Starting the Server

### HTTP Only (Default)
```bash
./build/cpp_srv --port 8080
```

### HTTPS Only
```bash
./build/cpp_srv --port_https 8443 --ssl ./ssl
```

### Both HTTP and HTTPS
```bash
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
```

### With All Options
```bash
./build/cpp_srv \
  --port 8080 \
  --port_https 8443 \
  --ssl ./ssl \
  --threads 8 \
  --log server.log \
  --token mytoken123
```

---

## Testing HTTPS

### Using curl

```bash
# Self-signed certificate (ignore verification)
curl -k https://localhost:8443/get/status

# With certificate verification
curl --cacert ssl/server.crt https://localhost:8443/get/status

# POST request
curl -k -X POST https://localhost:8443/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"Hello HTTPS"}}'
```

### Using Web Browser

1. Navigate to `https://localhost:8443`
2. Accept the security warning (for self-signed certificates)
3. Use the web interface normally

**Note**: Browsers will show a warning for self-signed certificates. This is expected and safe for development.

### Using the Test Script

```bash
./test_https.sh
```

This runs a comprehensive test suite including:
- HTTP endpoint connectivity
- HTTPS endpoint connectivity
- HTTPS POST requests
- Concurrent HTTP + HTTPS
- IPv6 HTTPS support

---

## Common Issues

### Issue: "OpenSSL not found"

**Solution**: Install OpenSSL development libraries:
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# After installing, rebuild
rm -rf build
./build.sh
```

### Issue: "Cannot bind HTTPS"

**Causes**:
1. Port already in use
2. Permission denied (ports < 1024 require root)

**Solutions**:
```bash
# Check if port is in use
sudo netstat -tulpn | grep :8443

# Use port > 1024 (no root needed)
./build/cpp_srv --port_https 8443 --ssl ./ssl

# Or run as root for port 443
sudo ./build/cpp_srv --port_https 443 --ssl ./ssl
```

### Issue: "SSL certificate not found"

**Cause**: Certificate files missing or wrong path

**Solution**:
```bash
# Verify files exist
ls -l ssl/
# Should show: server.crt and server.key

# Use absolute path
./build/cpp_srv --port_https 8443 --ssl /full/path/to/ssl
```

### Issue: Browser shows "Your connection is not private"

**Cause**: Using self-signed certificate

**Solution**: This is expected for self-signed certificates.
- **Development**: Click "Advanced" → "Proceed to localhost"
- **Production**: Use certificates from a trusted CA (Let's Encrypt, etc.)

---

## Production Deployment

### Using Let's Encrypt (Free Certificates)

```bash
# Install certbot
sudo apt install certbot

# Get certificate (HTTP-01 challenge)
sudo certbot certonly --standalone -d yourdomain.com

# Certificates will be in:
# /etc/letsencrypt/live/yourdomain.com/fullchain.pem (server.crt)
# /etc/letsencrypt/live/yourdomain.com/privkey.pem (server.key)

# Copy to your ssl directory
sudo cp /etc/letsencrypt/live/yourdomain.com/fullchain.pem ssl/server.crt
sudo cp /etc/letsencrypt/live/yourdomain.com/privkey.pem ssl/server.key
sudo chown $USER:$USER ssl/server.*

# Start server
./build/cpp_srv --port 80 --port_https 443 --ssl ./ssl
```

### Security Best Practices

1. **Use strong keys**: Minimum 2048-bit RSA (4096-bit recommended)
2. **Restrict key permissions**: `chmod 600 server.key`
3. **Regular certificate renewal**: Let's Encrypt certs expire every 90 days
4. **Use modern TLS**: cpp-httplib uses OpenSSL defaults (TLS 1.2+)
5. **Disable insecure ciphers**: Handled automatically by OpenSSL
6. **Run as non-root**: Use ports > 1024 or systemd socket activation
7. **Firewall configuration**: Only expose necessary ports

### Systemd Service Example

```ini
[Unit]
Description=cpp_cli_srv with HTTPS
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/path/to/cpp_cli_srv
ExecStart=/path/to/cpp_cli_srv/build/cpp_srv \
  --port 8080 \
  --port_https 8443 \
  --ssl /path/to/ssl \
  --log /var/log/cpp_srv.log \
  --token ${TOKEN}
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

---

## Monitoring

### Check HTTPS is enabled:

```bash
# Server should show HTTPS lines
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
# Output includes:
#   IPv4 (HTTPS) : https://0.0.0.0:8443
#   SSL     : ./ssl
```

### Test certificate validity:

```bash
# Check certificate details
openssl x509 -in ssl/server.crt -text -noout

# Check certificate expiration
openssl x509 -in ssl/server.crt -noout -enddate

# Test TLS connection
openssl s_client -connect localhost:8443 -showcerts
```

### Monitor connections:

```bash
# Check HTTPS connections
sudo netstat -tulpn | grep :8443

# Monitor server logs (if --log enabled)
tail -f server.log | grep HTTPS
```

---

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review server logs (if `--log` enabled)
3. Run `./test_https.sh` to verify HTTPS functionality
4. Ensure OpenSSL is installed: `pkg-config --modversion openssl`
