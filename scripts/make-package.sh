#!/bin/bash -e
if [ ! -f build/PotreeElevationProfile ] ||  [ ! -f build/PotreeExtractRegion ]; then
  mkdir -p build
  g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++11 -Wall -lbsd -c src/pmath.cpp -lstdc++fs
  g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++11 -Wall -lbsd -c src/PotreeReader.cpp -lstdc++fs
  g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++11 -Wall -lbsd -c src/main_extract_region.cpp -lstdc++fs
  g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++11 -Wall -lbsd -c src/main_elevation_profile.cpp -lstdc++fs
  g++ -std=c++11 -Wall -lbsd -o build/PotreeElevationProfile main_elevation_profile.o pmath.o PotreeReader.o -lstdc++fs
  g++ -std=c++11 -Wall -lbsd -o build/PotreeExtractRegion main_extract_region.o pmath.o PotreeReader.o -lstdc++fs
fi;
