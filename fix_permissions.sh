#!/bin/bash
# Fix file permissions for cpp_cli_srv project

echo "Fixing file permissions..."

# Make all directories readable and executable
find . -type d -exec chmod 755 {} \; 2>/dev/null

# Make all files readable
find . -type f -exec chmod 644 {} \; 2>/dev/null

# Make scripts executable
chmod +x *.sh 2>/dev/null
chmod +x build/*.sh 2>/dev/null

echo "Permissions fixed!"
echo ""
echo "Now run: ./build.sh"
