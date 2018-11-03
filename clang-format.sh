clang-format -i src/**/*.cpp
clang-format -i src/**/*.h -style="$(< header_style)"
clang-format -i src/*.cpp
clang-format -i src/*.h -style="$(< header_style)"
clang-format -i examples/**/*.h
clang-format -i examples/**/*.cpp
clang-format -i test/*.cpp
