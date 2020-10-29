
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

Extract points with the elevation profile:

    // minimal
    ./extract_profile <input> -o <output> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> 

    // Extract points in certain LOD level ranges
    ./extract_profile <input> -o <output> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> --min-level <integer> --max-level <integer>

* __input__: A point cloud generated with PotreeConverter 2.
* __output__: Can be files ending with *.las, *.laz, *.potree or it can be "stdout". If stdout is specified, a potree format file will be printed directly to the console. 
* __min-level__, __max-level__: Level range including the min and max levels. Can be omitted to process all levels. 


With ```--get-candidates```, you'll get the number of candidate points, i.e., the number of points inside all nodes intersecting the profile. The actual number of points might be orders of magnitudes lower, especially if ```--width``` is small.

    ./extract_profile <input> --coordinates "{x0, y1}, {x1, y}, ..." --width <scalar> --min-level <integer> --max-level <integer> --get-candidates

A practical example:

    ./extract_profile ~/dev/tmp/retz -o ~/dev/tmp/retz.laz --coordinates "{-37.601, -100.733, 4.940},{-22.478, 75.982, 8.287},{66.444, 54.042, 5.388},{71.294, -67.140, -2.481},{165.519, -26.288, 0.253}" --width 2 --min-level 0 --max-level 3
