set(SOURCE_FILES    asm.c
                    neobytecode.c
                    calibrate.c
                    codegen_neovm.c
                    compiler.c
                    display.c
                    dump.c
                    instruction_details.c
                    machine.c
                    main.c
                    scanner.c
                    test.c
                    util.c
                    neovm.c
                    Cell/cell.c)
                
add_executable(the8085 ${SOURCE_FILES})
