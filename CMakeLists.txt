set(SOURCE_FILES    bytecode.c
                    neobytecode.c
                    compiler.c
                    neocompiler.c
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
