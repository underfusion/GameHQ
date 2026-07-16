# First automated test target (0.5.71) — pure logic only: no database, no GUI,
# no game process. See docs/dev-setup.md for the run command.
#
# GameHQ is an executable, not a library, so there is nothing to link against.
# Each test compiles the handful of sources it actually needs straight into its
# own exe. That keeps the tests honest about their dependencies: if a unit needs
# half the app to build, it is not pure logic and does not belong here.
find_package(Qt6 REQUIRED COMPONENTS Test)

function(gamehq_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_BINARY_DIR}/generated")
    target_link_libraries(${name} PRIVATE Qt6::Test)
    # Console subsystem, explicitly: the app sets WIN32_EXECUTABLE project-wide,
    # and a GUI-subsystem test exe still passes but writes its results to a
    # console it does not have — ctest then reports a pass with no output.
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

gamehq_add_test(tst_gameidentity
    tst_gameidentity.cpp
    "${CMAKE_SOURCE_DIR}/src/core/GameIdentity.cpp")

gamehq_add_test(tst_configmanager
    tst_configmanager.cpp
    "${CMAKE_SOURCE_DIR}/src/config/ConfigManager.cpp")

gamehq_add_test(tst_gamerowrepair
    tst_gamerowrepair.cpp
    "${CMAKE_SOURCE_DIR}/src/storage/GameRowRepair.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/GameIdentity.cpp")
# GameRowRepair.cpp's other half (normalizeDuplicateNames) talks to QSqlDatabase,
# so the translation unit needs Qt::Sql to link even though the tested function
# does not touch it.
target_link_libraries(tst_gamerowrepair PRIVATE Qt6::Sql)
