
mkdir build64
cd build64
cmake .. -G "Visual Studio 16 2019" -A x64 -DDEVELOPER_MODE=ON
cmake --build . --config Release

cd ..
mkdir build64_re3
cd build64_re3
cmake .. -G "Visual Studio 16 2019" -A x64 -DDEVELOPER_MODE=ON -DRE3=ON
cmake --build . --config Release

copy /y bin\dinput8.dll "C:\Program Files (x86)\Steam\steamapps\common\RE3"
cd ..