cmake_minimum_required(VERSION 3.4.1)

project("JNIffmpegStaticPlay")

file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h)
add_library( # Sets the name of the library.
        jniffmpegstaticplay
        SHARED
        ${SOURCE})

find_library( # Sets the name of the path variable.
        log-lib
        log)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(my_lib_path ${CMAKE_SOURCE_DIR}/../../../libs/${CMAKE_ANDROID_ARCH_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${my_lib_path}")

#message("setPath: -L$-L${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")
#message("setPath2: ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")
#message("setPath2.1: ${CMAKE_SOURCE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}")
#message("setPath3: ${CMAKE_SOURCE_DIR}/")

target_link_libraries( # Specifies the target library.
        jniffmpegstaticplay
        avfilter avformat avcodec avutil swresample swscale
        ${log-lib}
        z
        OpenSLES
        android
        )
