$srcs = (Get-ChildItem src\*.cpp).FullName
g++ -std=c++17 -O2 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 -static -static-libgcc -static-libstdc++ -o veche.exe $srcs -I src -lgdi32