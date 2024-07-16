cmake_minimum_required(VERSION 3.5)  
  
project(nextflap)  
  
# Set the C++ standard to use  
set(CMAKE_CXX_STANDARD 20)  

set(CMAKE_BUILD_TYPE Debug)
  
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXECUTABLE "Build executable" ON)
option(DEBUG_SAS_TRANSLATIN "Debug SAS translation" OFF)
option(DEBUG_NUM_RPG_HEURISTICS "Debug numeric RPG heuristics" OFF)
option(DEBUG_TEMP_RPG_HEURISTICS "Debug temporal RPG heuristics" OFF)
option(DEBUG_LANDMARKS "Debug landmarks" OFF)
option(DEBUG_SUCCESSOR "Debug successors" OFF)
option(DEBUG_PLANNER "Debug planning module" OFF)


# Add source files  
set(SOURCES  
   grounder/grounder.cpp  
   grounder/groundedTask.cpp  
   heuristics/evaluator.cpp  
   heuristics/hFF.cpp  
   heuristics/hLand.cpp  
   heuristics/landmarks.cpp  
   heuristics/numericRPG.cpp  
   heuristics/rpg.cpp  
   heuristics/temporalRPG.cpp  
   parser/parsedTask.cpp  
   parser/parser.cpp  
   parser/syntaxAnalyzer.cpp  
   planner/intervalCalculations.cpp  
   planner/linearizer.cpp  
   planner/plan.cpp  
   planner/planBuilder.cpp  
   planner/planComponents.cpp  
   planner/planEffects.cpp  
   planner/planner.cpp  
   planner/plannerSetting.cpp  
   planner/printPlan.cpp  
   planner/selector.cpp  
   planner/state.cpp  
   planner/successors.cpp  
   planner/z3Checker.cpp  
   preprocess/preprocess.cpp  
   preprocess/preprocessedTask.cpp  
   sas/mutexGraph.cpp  
   sas/sasTask.cpp  
   sas/sasTranslator.cpp  
   utils/utils.cpp  
)  

# set options with variables
if(DEBUG_SAS_TRANSLATIN)
  add_definitions(-DDEBUG_SASTRANS_ON)
endif()  
if(DEBUG_NUM_RPG_HEURISTICS)
  add_definitions(-DNUMRPG_DEBUG)
endif()
if(DEBUG_TEMP_RPG_HEURISTICS)
  add_definitions(-DDEBUG_TEMPORALRPG_ON)
endif()
if(DEBUG_LANDMARKS)
  add_definitions(-DDEBUG_LANDMARKS_ON)
endif()
if(DEBUG_SUCCESSOR)
  add_definitions(-DDEBUG_SUCC_ON)
endif()
if(DEBUG_PLANNER)
  add_definitions(-D_DEBUG)
endif()

 

# Find Z3 library    
find_library(Z3_LIBRARY NAMES libz3.so.4.13)  

# Find Z3 includes
find_path(Z3_INCLUDE_DIRS z3++.h PATH_SUFFIXES z3)


if(NOT Z3_LIBRARY)  
  message(FATAL_ERROR "Z3 library not found! Please install version 4.13")
else()
  message(STATUS "Z3 library found at ${Z3_LIBRARY}.")
  message(STATUS "Z3 includes found at ${Z3_INCLUDE_DIRS}.")
endif()   

if(BUILD_SHARED_LIBS)
  message(STATUS "Building shared library.")
  # Create shared library
  add_library(${PROJECT_NAME} SHARED ${SOURCES})  

  # Link necessary libraries  
  target_link_libraries(${PROJECT_NAME}  ${Z3_LIBRARY})  
  
  # Include necessary directories  
  target_include_directories(${PROJECT_NAME} PRIVATE  
  ${Z3_INCLUDE_DIRS}
  )  
  # Install shared library
  install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
  install(DIRECTORY grounder heuristics parser planner preprocess sas utils DESTINATION include/nextflap FILES_MATCHING PATTERN "*.h")
endif()

if(BUILD_EXECUTABLE)
  # Create the executable target
  add_executable(nextflap_planner nextflap.cpp
                               ${SOURCES})  
  
  # Link necessary libraries  
  target_link_libraries(nextflap_planner  ${Z3_LIBRARY})  
  
  # Include necessary directories  
  target_include_directories(nextflap_planner PRIVATE  
  ${Z3_INCLUDE_DIRS}
  )   
  # Install executable
  install(TARGETS nextflap_planner DESTINATION bin)
endif()
