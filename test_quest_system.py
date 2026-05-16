#!/usr/bin/env python3
"""
Quick test script for quest system
Runs the game and captures output to verify quest initialization
"""

import subprocess
import time

print("=" * 60)
print("FROMVILLE - Quest System Initialization Test")
print("=" * 60)

# Run game for 10 seconds
print("\nStarting game... (capturing output for 10 seconds)\n")

try:
    proc = subprocess.Popen(
        ["./build-mingw/fromville.exe"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    # Collect output for 10 seconds
    time.sleep(10)
    proc.terminate()
    stdout, stderr = proc.communicate(timeout=2)
    
    # Look for quest-related messages
    output = stdout + stderr
    lines = output.split('\n')
    
    print("\n" + "=" * 60)
    print("QUEST SYSTEM OUTPUT:")
    print("=" * 60)
    
    quest_lines = [l for l in lines if 'Quest' in l or 'Consequence' in l or 'PHASE' in l]
    if quest_lines:
        for line in quest_lines:
            print(line)
    else:
        print("(No quest messages found)")
    
    print("\n" + "=" * 60)
    print("FULL OUTPUT (last 50 lines):")
    print("=" * 60)
    for line in lines[-50:]:
        if line.strip():
            print(line)
            
except Exception as e:
    print(f"Error running test: {e}")
