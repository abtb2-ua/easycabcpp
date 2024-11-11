cmake_minimum_required(VERSION 3.29)
project(easycabcpp)

add_compile_options(-Wall)

set(CMAKE_CXX_STANDARD 20)

# Set include directories
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/glib-2.0/include/)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)

# Check for required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(KAFKA REQUIRED rdkafka)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(NCURSES REQUIRED ncurses)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

execute_process(
        COMMAND mysql_config --cflags
        OUTPUT_VARIABLE MYSQL_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND mysql_config --libs
        OUTPUT_VARIABLE MYSQL_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

    # Set env variables
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(G_MESSAGES_DEBUG "all")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    unset(LOG_LEVEL)
endif()

# Set library flags
set(COMMON_INCLUDE_DIRS ${GLIB_INCLUDE_DIRS} ${KAFKA_INCLUDE_DIRS})
set(GUI_INCLUDE_DIRS ${NCURSES_INCLUDE_DIRS})

set(COMMON_LIBS ${GLIB_LIBRARIES} ${KAFKA_LIBRARIES} Threads::Threads)
set(GUI_LIBS ${NCURSES_LIBRARIES})

set(COMMON_CFLAGS ${GLIB_CFLAGS_OTHER} ${KAFKA_CFLAGS_OTHER})
set(GUI_CFLAGS ${NCURSES_CFLAGS_OTHER})


# Find files
file(GLOB COMMON_SOURCES "src/Common/*.cpp")
file(GLOB EC_CENTRAL_SOURCES "src/EC_Central/*.cpp")
file(GLOB EC_DE_SOURCES "src/EC_DE/*.cpp")
file(GLOB EC_SE_SOURCES "src/EC_SE/*.cpp")
file(GLOB EC_CUSTOMER_SOURCES "src/EC_Customer/*.cpp")

# Create executables
add_executable(EC_Central ${COMMON_SOURCES} ${EC_CENTRAL_SOURCES})
add_executable(EC_DE ${COMMON_SOURCES} ${EC_DE_SOURCES})
add_executable(EC_SE ${COMMON_SOURCES} ${EC_SE_SOURCES})
add_executable(EC_Customer ${COMMON_SOURCES} ${EC_CUSTOMER_SOURCES})

# Link libraries
target_link_libraries(EC_Central PUBLIC ${COMMON_LIBS} ${GUI_LIBS})
target_link_libraries(EC_DE PUBLIC ${COMMON_LIBS} ${GUI_LIBS})
target_link_libraries(EC_SE PUBLIC ${COMMON_LIBS} ${GUI_LIBS})
target_link_libraries(EC_Customer PUBLIC ${COMMON_LIBS})

# Include directories
target_include_directories(EC_Central PUBLIC ${COMMON_INCLUDE_DIRS} ${GUI_INCLUDE_DIRS})
target_include_directories(EC_DE PUBLIC ${COMMON_INCLUDE_DIRS} ${GUI_INCLUDE_DIRS})
target_include_directories(EC_SE PUBLIC ${COMMON_INCLUDE_DIRS} ${GUI_INCLUDE_DIRS})
target_include_directories(EC_Customer PUBLIC ${COMMON_INCLUDE_DIRS})

# Set compiler flags
target_compile_options(EC_Central PUBLIC ${COMMON_CFLAGS} ${GUI_CFLAGS})
target_compile_options(EC_DE PUBLIC ${COMMON_CFLAGS} ${GUI_CFLAGS})
target_compile_options(EC_SE PUBLIC ${COMMON_CFLAGS} ${GUI_CFLAGS})
target_compile_options(EC_Customer PUBLIC ${COMMON_CFLAGS})
