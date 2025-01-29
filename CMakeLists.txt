
cmake_minimum_required(VERSION 3.15)
project(memoryarenapractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable the generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(memoryarenapractice Main.cpp)

set_target_properties(memoryarenapractice PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin  
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}    
)
