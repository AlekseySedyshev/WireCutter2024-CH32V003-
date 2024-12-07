set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)


set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR STEM PROJECT_NAME)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
 
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

set(march -march=rv32ec)
if (GCC_MAJOR VERSION_LESS 12)
set(cross_toolchain_flags 
					 -march=rv32ecxw
           -mabi=ilp32e
           -msmall-data-limit=0
           -msave-restore
)

else()
set (cross_toolchain_flags

						-march=rv32ec
                            -mabi=ilp32e
                            -misa-spec=2.2
                            -msmall-data-limit=0
                            -msave-restore
)
endif()

SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/LD/Link.ld)

set(asm_flags   ${cross_toolchain_flags}
                -x assembler-with-cpp
                )
                
set(c_flags     ${cross_toolchain_flags}
                -fmessage-length=0
                -fsigned-char
                -ffunction-sections
                -fdata-sections
                -fno-common
                -Wunused
                -Wuninitialized  
                )

set(cxx_flags   ${cross_toolchain_flags}
                -fno-rtti
                -fno-exceptions
                )
                
add_compile_options(
    "$<$<COMPILE_LANGUAGE:ASM>:${asm_flags}>"
    "$<$<COMPILE_LANGUAGE:C>:${c_flags}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>"
)
PROJECT(${PROJECT_NAME} C CXX ASM)


add_definitions(
    # -DCH32V003
)
if ((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"))
    add_definitions(
        -DDEBUG=1
    )
endif()

include_directories(User
                    Core
                    Debug 
                    Peripheral/inc
                    )

file(GLOB_RECURSE SOURCES 
                "User/*.c"
                "Core/core_riscv.c" 
                "Debug/*.c"
                "Peripheral/SRC/*.c"
                "Startup/startup_ch32v00x.S"
                )                  

add_link_options( 
				 ${cross_toolchain_flags}
                -T ${LINKER_SCRIPT}
                -nostartfiles
                -Xlinker --gc-sections
                -Wl,-Map,${PROJECT_NAME}.map
                -Wl,-static --specs=nano.specs --specs=nosys.specs
                -Wl,--print-memory-usage
                )
                

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

if (GCC_MAJOR VERSION_LESS 12)
     target_link_libraries(${PROJECT_NAME}.elf printf)
endif()

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
)
