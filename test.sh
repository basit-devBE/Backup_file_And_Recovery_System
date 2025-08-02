#!/bin/bash

# Test script for the Backup and Recovery System
# Tests various scenarios and edge cases

BACKUP_SYSTEM="./build/backup_system"
TEST_DIR="./test_scenario"
BACKUP_DIR="./test_backups"

echo "Backup and Recovery System - Comprehensive Tests"
echo "==============================================="

# Clean up previous test runs
rm -rf "$TEST_DIR" "$BACKUP_DIR" "./restored_*"

# Test 1: Basic backup and restore
echo ""
echo "Test 1: Basic backup and restore"
echo "---------------------------------"

mkdir -p "$TEST_DIR"
echo "Test file content" > "$TEST_DIR/test.txt"
mkdir -p "$TEST_DIR/docs"
echo "Document content" > "$TEST_DIR/docs/document.txt"

echo "Creating backup..."
$BACKUP_SYSTEM --backup --source "$TEST_DIR" --dest "$BACKUP_DIR"

echo "Restoring backup..."
mkdir -p "./restored_basic"
FIRST_BACKUP=$(ls -1 "$BACKUP_DIR" | head -n1)
$BACKUP_SYSTEM --restore --backup-path "$BACKUP_DIR/$FIRST_BACKUP" --restore-path "./restored_basic"

echo "Verifying restored files..."
if diff -r "$TEST_DIR" "./restored_basic" > /dev/null; then
    echo "✓ Basic backup and restore test PASSED"
else
    echo "✗ Basic backup and restore test FAILED"
fi

# Test 2: Incremental backup
echo ""
echo "Test 2: Incremental backup"
echo "-------------------------"

echo "Adding new files..."
echo "New file content" > "$TEST_DIR/new_file.txt"
echo "Modified content" >> "$TEST_DIR/test.txt"

echo "Creating incremental backup..."
$BACKUP_SYSTEM --incremental --source "$TEST_DIR" --dest "$BACKUP_DIR"

echo "Restoring incremental backup..."
mkdir -p "./restored_incremental"
SECOND_BACKUP=$(ls -1t "$BACKUP_DIR" | head -n1)
$BACKUP_SYSTEM --restore --backup-path "$BACKUP_DIR/$SECOND_BACKUP" --restore-path "./restored_incremental"

echo "Verifying incremental restore..."
if [ -f "./restored_incremental/new_file.txt" ]; then
    echo "✓ Incremental backup test PASSED"
else
    echo "✗ Incremental backup test FAILED"
fi

# Test 3: Compression test
echo ""
echo "Test 3: Compression"
echo "------------------"

# Create a large test file
echo "Creating large test file..."
mkdir -p "$TEST_DIR/large"
dd if=/dev/zero of="$TEST_DIR/large/big_file.dat" bs=1M count=1 2>/dev/null

echo "Creating compressed backup..."
$BACKUP_SYSTEM --backup --source "$TEST_DIR" --dest "$BACKUP_DIR" --compress --level 9

COMPRESSED_BACKUP=$(ls -1t "$BACKUP_DIR" | head -n1)
BACKUP_SIZE=$(du -s "$BACKUP_DIR/$COMPRESSED_BACKUP" | cut -f1)
SOURCE_SIZE=$(du -s "$TEST_DIR" | cut -f1)

if [ "$BACKUP_SIZE" -lt "$SOURCE_SIZE" ]; then
    echo "✓ Compression test PASSED (backup smaller than source)"
else
    echo "✗ Compression test FAILED (backup not smaller)"
fi

# Test 4: Verification
echo ""
echo "Test 4: Backup verification"
echo "--------------------------"

echo "Verifying backup integrity..."
if $BACKUP_SYSTEM --verify --backup-path "$BACKUP_DIR/$COMPRESSED_BACKUP"; then
    echo "✓ Verification test PASSED"
else
    echo "✗ Verification test FAILED"
fi

# Test 5: List backups
echo ""
echo "Test 5: List backups"
echo "-------------------"

echo "Listing all backups:"
$BACKUP_SYSTEM --list --dest "$BACKUP_DIR"

# Test 6: Error handling
echo ""
echo "Test 6: Error handling"
echo "---------------------"

echo "Testing with non-existent source..."
if ! $BACKUP_SYSTEM --backup --source "/non/existent/path" --dest "$BACKUP_DIR" 2>/dev/null; then
    echo "✓ Error handling test PASSED (correctly failed on invalid source)"
else
    echo "✗ Error handling test FAILED (should have failed)"
fi

echo ""
echo "All tests completed!"
echo ""
echo "Summary:"
echo "  - Test files created in: $TEST_DIR"
echo "  - Backups stored in: $BACKUP_DIR"
echo "  - Restored files in: ./restored_*"
echo ""
echo "Clean up test files with: rm -rf $TEST_DIR $BACKUP_DIR ./restored_*"
