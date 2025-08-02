#!/bin/bash

# Example script showing how to use the Backup and Recovery System

BACKUP_SYSTEM="./build/backup_system"
SOURCE_DIR="./test_data"
BACKUP_DIR="./backups"

echo "Backup and Recovery System - Demo Script"
echo "========================================"

# Create test data
echo "Creating test data..."
mkdir -p "$SOURCE_DIR"
echo "This is test file 1" > "$SOURCE_DIR/file1.txt"
echo "This is test file 2" > "$SOURCE_DIR/file2.txt"
mkdir -p "$SOURCE_DIR/subdir"
echo "This is a file in subdirectory" > "$SOURCE_DIR/subdir/file3.txt"

# Create backup directory
mkdir -p "$BACKUP_DIR"

echo ""
echo "1. Creating full backup..."
$BACKUP_SYSTEM --backup --source "$SOURCE_DIR" --dest "$BACKUP_DIR" --compress

echo ""
echo "2. Modifying files..."
echo "Modified content" >> "$SOURCE_DIR/file1.txt"
echo "This is a new file" > "$SOURCE_DIR/new_file.txt"

echo ""
echo "3. Creating incremental backup..."
$BACKUP_SYSTEM --incremental --source "$SOURCE_DIR" --dest "$BACKUP_DIR" --compress

echo ""
echo "4. Listing available backups..."
$BACKUP_SYSTEM --list --dest "$BACKUP_DIR"

echo ""
echo "5. Verifying latest backup..."
LATEST_BACKUP=$(ls -1t "$BACKUP_DIR" | head -n1)
if [ -n "$LATEST_BACKUP" ]; then
    $BACKUP_SYSTEM --verify --backup-path "$BACKUP_DIR/$LATEST_BACKUP"
fi

echo ""
echo "6. Restoring backup to test_restore directory..."
if [ -n "$LATEST_BACKUP" ]; then
    mkdir -p "./test_restore"
    $BACKUP_SYSTEM --restore --backup-path "$BACKUP_DIR/$LATEST_BACKUP" --restore-path "./test_restore"
fi

echo ""
echo "Demo completed! Check the following directories:"
echo "  - Source data: $SOURCE_DIR"
echo "  - Backups: $BACKUP_DIR"
echo "  - Restored data: ./test_restore"
