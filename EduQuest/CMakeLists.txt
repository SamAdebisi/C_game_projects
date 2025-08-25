// =============================================
// file: CMakeLists.txt
// =============================================
cmake_minimum_required(VERSION 3.16)
project(EduQuestC C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB EDUQ_SRC CONFIGURE_DEPENDS src/*.c player/*.c)
add_executable(eduquest ${EDUQ_SRC})
target_include_directories(eduquest PRIVATE src)
