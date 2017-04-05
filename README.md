# CPotree
Potree Utilities 




CPotree.exe "D:/dev/pointclouds/converted/CA13/cloud.js" "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" 14.0 0 1 
CPotree.exe "D:/dev/pointclouds/converted/CA13/cloud.js" "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" --width 14.0 --minLevel 0 --maxLevel 5 

CPotree.exe "D:/dev/pointclouds/converted/CA13/cloud.js" ^
 "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" ^
 14.0 0 1



.\PotreeElevationProfile.exe "D:/dev/pointclouds/converted/CA13/cloud.js" ^
 --coordinates "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" ^
 --width 14.0 --min-level 0 --max-level 1
 --stdout
 
.\PotreeElevationProfile.exe "D:/dev/pointclouds/converted/CA13/cloud.js" ^
 --coordinates "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" ^
 --width 14.0 --min-level 0 --max-level 1
 -o result.potree
 
.\PotreeElevationProfile.exe "D:/dev/pointclouds/converted/CA13/cloud.js" ^
 --coordinates "{693550.968, 3915914.169},{693890.618, 3916387.819},{694584.820, 3916458.180},{694786.239, 3916307.199}" ^
 --width 14.0 --min-level 0 --max-level 1
 -o result.las
 
 

