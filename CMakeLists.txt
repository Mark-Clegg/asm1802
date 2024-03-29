cmake_minimum_required(VERSION 3.5)

project(asm1802 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

find_package(fmt)

add_executable(asm1802 main.cpp
    README.md
    LICENSE

    assembler.h assembler.cpp
    sourcecodereader.h sourcecodereader.cpp
    utils.h utils.cpp
    listingfilewriter.h listingfilewriter.cpp
    opcodetable.h opcodetable.cpp
    symboltable.h symboltable.cpp
    errortable.h errortable.cpp
    macro.h macro.cpp
    preprocessor.h preprocessor.cpp
    expressiontokenizer.h expressiontokenizer.cpp

    assemblyexception.h assemblyexception.cpp
    preprocessorexception.h preprocessorexception.cpp
    expressionexception.h expressionexception.cpp

    expressionevaluatorbase.h expressionevaluatorbase.cpp
    preprocessorexpressionevaluator.h preprocessorexpressionevaluator.cpp
    assemblyexpressionevaluator.h assemblyexpressionevaluator.cpp

    binarywriter.h binarywriter.cpp
    binarywriter_intelhex.h binarywriter_intelhex.cpp
    binarywriter_idiot4.h binarywriter_idiot4.cpp
    binarywriter_binary.h binarywriter_binary.cpp
    binarywriter_elfos.h binarywriter_elfos.cpp

    lsp/cdp1802-languageserver.py
    syntax/asm1802.xml
    )

install(TARGETS asm1802
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES syntax/asm1802.xml
    DESTINATION share/org.kde.syntax-highlighting/syntax
)

install(PROGRAMS lsp/cdp1802-languageserver.py
    DESTINATION bin
)

target_link_libraries(asm1802 fmt::fmt)
