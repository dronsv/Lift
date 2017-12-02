cmake_minimum_required(VERSION 3.9)
project(Lift)

set(CMAKE_CXX_STANDARD 14)

if ( Win32)
    set( SYSTEM_LINK )
elseif ( APPLE)
    set(SYSTEM_LINK )
elseif ( UNIX)
    set( SYSTEM_LINK pthread)
endif()


add_executable(Lift main.cpp lift.cpp lift.hpp)
target_link_libraries(Lift ${SYSTEM_LINK})