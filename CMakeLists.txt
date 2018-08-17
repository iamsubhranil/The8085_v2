set(SOURCE_FILES    bytecode.c
                    compiler.c
                    display.c
                    main.c
                    scanner.c
                    test.c
                    util.c
                    vm.c
                    Cell/cell.c)
                
add_executable(mp8085 ${SOURCE_FILES})
