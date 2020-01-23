cd build64
cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release -DDEVELOPER_MODE=ON
cmake --build . --config Release
copy /y bin\dinput8.dll "C:\Program Files (x86)\Steam\steamapps\common\RESIDENT EVIL 2  BIOHAZARD RE2\"
cd ..