# Builds obj2kac using GCC. This build includes some Qt 5 functionality.

SOURCE_FILES="
./src/obj2kac.cpp
./src/string_utils.cpp
../export_kac_1_0.cpp
"

g++-9 -g -pipe -Wall -pedantic -O2 -std=c++17 -fPIC -isystem /usr/include/x86_64-linux-gnu/qt5/ $SOURCE_FILES -o ./bin/obj2kac -lQt5Core -lQt5Gui
