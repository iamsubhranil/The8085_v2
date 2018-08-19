set(SOURCE_FILES    bytecode.c
                    neobytecode.c
                    codegen_bcvm.c
                    codegen_neovm.c
                    compiler.c
                    display.c
                    dump.c
                    machine.c
                    main.c
                    scanner.c
                    test.c
                    util.c
                    vm.c
                    neovm.c
                    Cell/cell.c)
                
add_executable(mp8085 ${SOURCE_FILES})
