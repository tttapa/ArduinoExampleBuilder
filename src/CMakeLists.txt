add_executable(arduino-example-builder 
    ArduinoExampleBuilder.cpp
    ArduinoBuildJob.cpp
    Exec.cpp
    Printing.cpp
    StringHelpers.cpp
)
target_link_options(arduino-example-builder
    PUBLIC "$<$<CONFIG:RELEASE>:-s>")
target_include_directories(arduino-example-builder 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(arduino-example-builder 
    Threads::Threads fmt::fmt OpenSSL::SSL)

install (TARGETS arduino-example-builder DESTINATION bin)
install (FILES ../board-options/board-options.txt 
    DESTINATION share/arduino-example-builder/)