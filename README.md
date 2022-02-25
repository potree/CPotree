
# Build

__Linux:__
```
mkdir build
cd build
cmake ../
make
```

__Windows:__
```
mkdir build
cd build
cmake ../
```
* Then open the generated sln file in Visual Studio. 
* Make sure "Release" build is selected.
* Build "extract_profile".


# Usage

Minimal

    ./extract_profile <input> -o <output> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> 
    ./extract_area <input> -o <output> --area "{tx,rz,ry,0,-rz,ty,rx,0,-ry,-rx,tz,0,dx,dy,dz,1}"

Extract points in certain LOD level ranges

    ./extract_profile <input> -o <output> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> --min-level <integer> --max-level <integer>

Extract points with different attributes

    ./extract_profile <input> -o <output> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> --output-attributes <attributes> --output-format <format>

* __input__: A point cloud generated with PotreeConverter 2.
* __output__: Can be files ending with *.las, *.laz, *.csv, *.json, *.potree. If omitted, the result will be printed directly to the console. 
* __format__: LAS, LAZ, CSV, JSON, POTREE (default). If omitted, the files endings will be used.
* __attributes__: Point attributes like: position, rgb, intensity, classification, ... Default: same as input point cloud
* __min-level__, __max-level__: Level range including the min and max levels. Can be omitted to process all levels. 


With ```--get-candidates```, you'll get the number of candidate points, i.e., the number of points inside all nodes intersecting the profile. The actual number of points might be orders of magnitudes lower, especially if ```--width``` is small.

    ./extract_profile <input> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> --min-level <integer> --max-level <integer> --get-candidates

A practical example:

    ./extract_profile ~/dev/tmp/retz -o ~/dev/tmp/retz.laz --coordinates "{-37.601, -100.733, 4.940},{-22.478, 75.982, 8.287},{66.444, 54.042, 5.388},{71.294, -67.140, -2.481},{165.519, -26.288, 0.253}" --width 2 --min-level 0 --max-level 3
