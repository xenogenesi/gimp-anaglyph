# TODO check if >= 3.2 is really required
cmake_minimum_required(VERSION 3.2)

#http://cmake.org/Wiki/CMake_Useful_Variables
#CMAKE_BUILD_TYPE
#cmake -DCMAKE_BUILD_TYPE=Debug ..
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
#cmake -DCMAKE_BUILD_TYPE=Release ..
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

project("gimp anaglyph plugin" C)

exec_program(gimptool-2.0 ARGS --cflags OUTPUT_VARIABLE GIMP_C_FLAGS)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GIMP_C_FLAGS}")
exec_program(gimptool-2.0 ARGS --libs OUTPUT_VARIABLE GIMP_LIBS)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GIMP_LIBS}")

#exec_program(gimptool-2.0 ARGS --dry-run --install-bin src/gimp-anaglyph OUTPUT_VARIABLE GIMP_USER_PLUGIN_DIR)
#install(FILES files... DESTINATION "${GIMP_USER_PLUGIN_DIR}")

add_custom_target(xinstall COMMAND gimptool-2.0 --install-bin gimp-anaglyph
                          DEPENDS src/gimp-anaglyph)
add_custom_target(xuninstall COMMAND gimptool-2.0 --uninstall-bin gimp-anaglyph)

add_executable(gimp-anaglyph anaglyph.c)

