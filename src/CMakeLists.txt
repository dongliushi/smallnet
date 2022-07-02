find_package(Threads)
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC *.cc)
file(GLOB HEADER ${PROJECT_SOURCE_DIR}/include/*.h)

add_library(smallnet SHARED ${SRC})

target_link_libraries(smallnet PUBLIC ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS smallnet 
    LIBRARY DESTINATION lib
    )

install(FILES ${HEADER} DESTINATION include/smallnet)