CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(STANDALONEDEMO)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

ADD_COMPILE_OPTIONS(
  "-std=c++11" "-Wall" "-Wextra" "-O2"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-ggdb>"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-fno-omit-frame-pointer>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-DNDEBUG>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-fomit-frame-pointer>"
)

SET(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost COMPONENTS program_options system REQUIRED)
FIND_PACKAGE(LogHard REQUIRED)
FIND_PACKAGE(SharemindLibController REQUIRED)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${LogHard_INCLUDE_DIRS}
    ${SharemindLibController_INCLUDE_DIRS}
)

ADD_DEFINITIONS(
    ${LogHard_DEFINITIONS}
    ${SharemindLibController_DEFINITIONS}
    "-D__STDC_LIMIT_MACROS"
)

SET(STANDALONEDEMO_LIBRARIES
    ${Boost_LIBRARIES}
    ${LogHard_LIBRARIES}
    ${SharemindLibController_LIBRARIES}
)

FILE(GLOB_RECURSE STANDALONEDEMO_SECREC "${CMAKE_CURRENT_SOURCE_DIR}/secrec/*.sc")
FILE(GLOB_RECURSE STANDALONEDEMO_DEBUGFILES "${CMAKE_CURRENT_SOURCE_DIR}/debugfiles/*")

SET(STANDALONEDEMO_FILES ${STANDALONEDEMO_SECREC} ${STANDALONEDEMO_DEBUGFILES})

ADD_CUSTOM_TARGET("include_StandaloneDemo_Files_in_IDE"
                  SOURCES ${STANDALONEDEMO_FILES})

ADD_EXECUTABLE(ScalarProduct src/ScalarProduct.cpp)
TARGET_LINK_LIBRARIES(ScalarProduct ${STANDALONEDEMO_LIBRARIES})
INSTALL(TARGETS ScalarProduct RUNTIME DESTINATION "bin")

INSTALL(FILES ${STANDALONEDEMO_DEBUGFILES} DESTINATION "bin")