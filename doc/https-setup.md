# HTTPS / SSL Setup

This guide explains how to enable HTTPS support in cpp_cli_srv.

---

## Prerequisites

HTTPS requires OpenSSL development libraries.

| Distribution | Install Command |
|--------------|----------------|
| **Ubuntu/Debian** | `sudo apt update && sudo apt install libssl-dev` |
| **CentOS/RHEL** | `sudo yum install openssl-devel` |
| **Fedora** | `sudo dnf install openssl-devel` |
| **Arch Linux** | `sudo pacman -S openssl` |

Verify:
```bash
pkg-config --modversion openssl
# Expected: 1.1.1 or 3.0.x
```

Then build (CMake auto-detects OpenSSL):
```bash
rm -rf build && ./build.sh
# Should print: -- OpenSSL found - HTTPS support enabled
```

> **Windows**: HTTPS is not currently supported on the Windows build.

---

## SSL Certificate Setup

### Option 1: Use Existing Certificates (Production)

```bash
mkdir -p ssl
cp your_certificate.crt ssl/server.crt
cp your_private_key.key ssl/server.key
chmod 600 ssl/server.key
chmod 644 ssl/server.crt
```

### Option 2: Self-Signed Certificate (Development)

```bash
mkdir -p ssl

# Basic self-signed (localhost)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=State/L=City/O=Org/CN=localhost"

chmod 600 ssl/server.key
```

**For multiple domains (SAN):**
```bash
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
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -config ssl/san.cnf \
  -extensions v3_req
```

### Option 3: Let's Encrypt (Free Trusted Certificates)

```bash
sudo apt install certbot
sudo certbot certonly --standalone -d yourdomain.com

# Copy to your ssl directory
sudo cp /etc/letsencrypt/live/yourdomain.com/fullchain.pem ssl/server.crt
sudo cp /etc/letsencrypt/live/yourdomain.com/privkey.pem ssl/server.key
sudo chown $USER:$USER ssl/server.*
```

---

## Starting the Server with HTTPS

```bash
# HTTP only (default)
./build/cpp_srv --port 8080

# HTTPS only
./build/cpp_srv --port_https 8443 --ssl ./ssl

# Both HTTP and HTTPS
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl

# Full options
./build/cpp_srv \
  --port 8080 \
  --port_https 8443 \
  --ssl ./ssl \
  --threads 8 \
  --log server.log \
  --token mytoken123
```

On startup with HTTPS enabled:
```
=== cpp_srv started ===
  IPv4 (HTTP)  : http://0.0.0.0:8080
  IPv6 (HTTP)  : http://[::]:8080
  IPv4 (HTTPS) : https://0.0.0.0:8443
  IPv6 (HTTPS) : https://[::]:8443
  SSL     : ./ssl
```

---

## Testing HTTPS

```bash
# Self-signed (ignore cert verification)
curl -k https://localhost:8443/get/status

# With cert verification
curl --cacert ssl/server.crt https://localhost:8443/get/status

# POST request
curl -k -X POST https://localhost:8443/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"Hello HTTPS"}}'

# Comprehensive HTTPS test script
./test_https.sh
```

**Browser**: Navigate to `https://localhost:8443`. Accept the security warning (expected for self-signed certs).

---

## Monitoring

```bash
# Check certificate details
openssl x509 -in ssl/server.crt -text -noout

# Check expiration date
openssl x509 -in ssl/server.crt -noout -enddate

# Test TLS connection
openssl s_client -connect localhost:8443 -showcerts

# Check HTTPS connections
sudo netstat -tulpn | grep :8443
```

---

## Production Deployment

**Systemd service with HTTPS:**
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

**Security best practices:**
1. Use minimum 2048-bit RSA keys (4096-bit recommended)
2. `chmod 600 server.key` — restrict private key access
3. Let's Encrypt certs expire every 90 days — automate renewal
4. Use ports > 1024 (no root required)
5. Run as non-root user
6. Configure firewall to only expose necessary ports

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "OpenSSL not found" | `sudo apt install libssl-dev`, then `rm -rf build && ./build.sh` |
| "Cannot bind HTTPS" | Check port in use: `sudo netstat -tulpn \| grep :8443`; use port > 1024 |
| "SSL certificate not found" | Use absolute path: `--ssl /full/path/to/ssl` |
| Browser "connection not private" | Expected for self-signed certs. Click "Advanced → Proceed" in dev, use CA cert in prod |
