cmake_minimum_required(VERSION 3.9.6)
project(osmpexecutable C)
project(osmprun C)

set(CMAKE_C_STANDARD 11)

#set(CMAKE_C_FLAGS "-fsanitize=addaddressress -fsanitize-address-use-after-scope -O1 -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "fsanitize=leak")

#set(CMAKE_C_FLAGS "-fsanitize=address")

add_executable(osmpexecutable osmpexecutable.c OSMP.h OSMP.c int_to_string)

add_executable(osmprun osmprun.c OSMP.h int_to_string.h)

if (UNIX AND NOT APPLE)

    TARGET_LINK_LIBRARIES(osmpexecutable rt)

    TARGET_LINK_LIBRARIES(osmprun rt)

    TARGET_LINK_LIBRARIES(osmpexecutable m)

    TARGET_LINK_LIBRARIES(osmprun m)

    TARGET_LINK_LIBRARIES(osmpexecutable pthread)

    TARGET_LINK_LIBRARIES(osmprun pthread)

endif ()