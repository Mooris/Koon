cmake_minimum_required(VERSION 2.8)

project(Koon)

find_package(BISON)
find_package(FLEX)

add_definitions(-g3 -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

set(SRC_DIR src/)
set(SRC ${SRC_DIR}main.cpp
    ${SRC_DIR}Nodes.cpp
    ${SRC_DIR}Kompiler.cpp
    ${SRC_DIR}KoonDriver.cpp)

BISON_TARGET(Parser ${SRC_DIR}parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(Scanner ${SRC_DIR}tokens.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} include)
add_executable(koon ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS} ${SRC})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/share/llvm/cmake")
find_package(LLVM REQUIRED CONFIG)

if( NOT LLVM_FOUND  )
    message(FATAL_ERROR "LLVM package can't be found. Set CMAKE_PREFIX_PATH variable to LLVM's installation prefix.")
endif()


include_directories( ${LLVM_INCLUDE_DIRS}  )
link_directories( ${LLVM_LIBRARY_DIRS}  )

set(LLVM_LINK_COMPONENTS
    ${LLVM_TARGETS_TO_BUILD}
    Analysis
    AsmPrinter
    CodeGen
    Core
    IRReader
    MC
    MIRParser
    ScalarOpts
    SelectionDAG
    Support
    Target
    TransformUtils
)

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES native ${LLVM_LINK_COMPONENTS})

target_link_libraries(koon ${REQ_LLVM_LIBRARIES})
