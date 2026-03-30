# Permission Denied Error - Quick Fix

## Problem

When copying the cpp_cli_srv project to another machine, you may encounter:

```
make[2]: stat: /root/jd/t/t0/cc1/Claw/cpp_cli_srv/cli/main.cpp: Permission denied
make[2]: stat: /root/jd/t/t0/cc1/Claw/cpp_cli_srv/server/main.cpp: Permission denied
```

This happens when files are copied without preserving permissions.

## Quick Solution

### On the new machine:

```bash
# 1. Go to project directory
cd /path/to/cpp_cli_srv

# 2. Run the fix script
./fix_permissions.sh

# 3. Rebuild
./build.sh
```

Done! The build should now work.

## Manual Fix (if fix_permissions.sh is missing or not executable)

```bash
# Make all directories accessible
find . -type d -exec chmod 755 {} \;

# Make all files readable
find . -type f -exec chmod 644 {} \;

# Make scripts executable
chmod +x *.sh

# Rebuild
./build.sh
```

## How to Copy the Project Correctly

To avoid this issue in the future:

### Method 1: Use cp with -p flag
```bash
cp -rp /source/cpp_cli_srv /destination/
```

### Method 2: Use tar (recommended for transfers)
```bash
# On source machine:
tar czf cpp_cli_srv.tar.gz cpp_cli_srv/

# Transfer the file, then on target machine:
tar xzf cpp_cli_srv.tar.gz
```

### Method 3: Use rsync (for network copies)
```bash
rsync -av /source/cpp_cli_srv/ user@remote:/destination/
```

### Method 4: Use Git (best option)
```bash
git clone <repository-url>
```
Git automatically preserves executable permissions.

## Why This Happens

When files are copied (especially via certain file transfer methods like Windows network shares, some FTP clients, or plain `cp`), they may lose their permission bits. The build system (make) needs to be able to:
- **Read** source files (.cpp, .h)
- **Execute** directory traversal
- **Execute** build scripts

The fix script restores these permissions:
- **755** for directories (rwxr-xr-x)
- **644** for files (rw-r--r--)
- **755** for scripts (rwxr-xr-x)

## Verify Permissions Are Fixed

Check a few key files:
```bash
ls -la cli/main.cpp      # Should show: -rw-r--r--
ls -la build.sh          # Should show: -rwxr-xr-x
ls -la core/            # Should show: drwxr-xr-x
```
