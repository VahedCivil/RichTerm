set(RICHTERM_EXAMPLES
    showcase
    console
    markup
    formatting
    table
    panel
    datetime
    progress
    status
)

foreach(example IN LISTS RICHTERM_EXAMPLES)
    add_executable(richterm_${example} ${example}.cpp)
    target_link_libraries(richterm_${example} PRIVATE RichTerm::RichTerm)
    if(RICHTERM_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(richterm_${example} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(richterm_${example} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        endif()
    endif()
endforeach()
