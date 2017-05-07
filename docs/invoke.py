import subprocess
import struct

import sys

exe = "D:/dev/workspaces/CPotree/master/x64/Release/CPotree.exe"

file = "D:/dev/pointclouds/converted/CA13/cloud.js"
coordinates = "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}"
width = "14.0"
minLevel = "0"
maxLevel = "1"

p = subprocess.Popen([exe, file, coordinates, width, minLevel, maxLevel], stdout=subprocess.PIPE)
[out, err] = p.communicate()

headerSize = struct.unpack('i', out[0:4])[0];
header = out[4:4+headerSize].decode("ascii")
buffer = out[4+headerSize:]

print(header)
print(buffer[:50])