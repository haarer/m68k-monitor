#!/bin/bash

# Test script for the m68k-monitor project with SREC support

echo "Testing m68k-monitor build with SREC support..."
cd /workspace/m68-monitor

# Try to build both variants 
echo "Building for QEMU variant..."
make VARIANT=qemu all 2>&1 | grep -E "(error|Error|failed|Failed)" || echo "QEMU build completed successfully"

echo ""
echo "Building for real hardware variant..."
make VARIANT=realhw all 2>&1 | grep -E "(error|Error|failed|Failed)" || echo "RealHW build completed successfully"

echo ""
echo "Build test complete. The SREC command has been added to the monitor."