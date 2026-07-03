cmake_minimum_required(VERSION 3.16)
project(TouchCarousel CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base gl)
include(${wxWidgets_USE_FILE})

find_package(OpenGL REQUIRED)

add_executable(TouchCarousel main.cpp Carousel.h)

target_link_libraries(TouchCarousel
    ${wxWidgets_LIBRARIES}
    OpenGL::GL
)

if(APPLE)
    target_link_libraries(TouchCarousel "-framework OpenGL")
endif()
