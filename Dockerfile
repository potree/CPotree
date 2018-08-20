FROM debian:stretch
RUN apt-get update && apt-get install -y build-essential libbsd0 libbsd-dev
RUN mkdir -p /app/build
COPY . /app
WORKDIR app
RUN g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++14 -Wall -lbsd -c src/pmath.cpp -lstdc++fs
RUN g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++14 -Wall -lbsd -c src/PotreeReader.cpp -lstdc++fs
RUN g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++14 -Wall -lbsd -c src/main_extract_region.cpp -lstdc++fs
RUN g++ -I libs/rapidjson/include/ -I libs/glm/ -I include/ -std=c++14 -Wall -lbsd -c src/main_elevation_profile.cpp -lstdc++fs
RUN g++ -std=c++14 -Wall -lbsd -o build/PotreeElevationProfile main_extract_region.o pmath.o PotreeReader.o -lstdc++fs
RUN g++ -std=c++14 -Wall -lbsd -o build/PotreeExtractRegion main_elevation_profile.o pmath.o PotreeReader.o -lstdc++fs
