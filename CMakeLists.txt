cmake_minimum_required(VERSION 3.14.0)
project(unicam VERSION 1.0.0)

include(cmake/bin2h.cmake)
include(cmake/verstring.cmake)
include(cmake/sfdc.cmake)

if(NOT TARGET devicetree)
    add_subdirectory(devicetree.resource EXCLUDE_FROM_ALL)
endif()

if (NOT TARGET mailbox)
    add_subdirectory(mailbox.resource EXCLUDE_FROM_ALL)
endif()

get_verstring(VERSTRING)
sfdc(sfd/unicam_lib.sfd)

add_executable(unicam.resource
    src/main.c
    src/init.c
    src/mbox.c
    src/smoothing.c
    src/start.c
    src/stop.c
    src/unicam.c
    src/videocore.c
    src/getframebuffer.c
    src/getcropsize.c
    src/getkernel.c
    src/getconfig.c
    src/getsize.c
)

bin_to_header(unicam.resource)

target_compile_options(unicam.resource PRIVATE -Os -m68040 -msmall-code -mpcrel -mregparm=4 -fomit-frame-pointer)
target_compile_definitions(unicam.resource PRIVATE VERSION_STRING="${VERSTRING}")
target_include_directories(unicam.resource PRIVATE include)
target_link_options(unicam.resource PRIVATE -ffreestanding -nostdlib -nostartfiles -s -Wl,-e_rom_start -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds)
target_link_libraries(unicam.resource unicam devicetree mailbox)

target_include_directories(unicam INTERFACE include_pub)

set_target_properties(unicam.resource PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds)

# no install for devicetree.resource itself, it goes to ROM
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sfd DESTINATION Developer)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include_pub/ DESTINATION Developer/include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include DESTINATION Developer)
