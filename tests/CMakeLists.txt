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

gamehq_add_test(tst_versionnumber
    tst_versionnumber.cpp
    "${CMAKE_SOURCE_DIR}/src/updates/VersionNumber.cpp")

gamehq_add_test(tst_updatemaintenance
    tst_updatemaintenance.cpp
    "${CMAKE_SOURCE_DIR}/src/core/UpdateMaintenance.cpp")

gamehq_add_test(tst_integrationprotocol
    tst_integrationprotocol.cpp
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationProtocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.h")
target_link_libraries(tst_integrationprotocol PRIVATE Qt6::Network)

gamehq_add_test(tst_integrationservice
    tst_integrationservice.cpp
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationProtocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.h"
    "${CMAKE_SOURCE_DIR}/src/integration/ExternalGameContext.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/ExternalGameContext.h"
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationService.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationService.h")
target_link_libraries(tst_integrationservice PRIVATE Qt6::Network)

add_executable(integration_server_fixture
    integration_server_fixture.cpp
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationProtocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/LocalIntegrationServer.h"
    "${CMAKE_SOURCE_DIR}/src/integration/ExternalGameContext.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/ExternalGameContext.h"
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationService.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationService.h")
target_include_directories(integration_server_fixture PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(integration_server_fixture PRIVATE Qt6::Core Qt6::Network)
set_target_properties(integration_server_fixture PROPERTIES WIN32_EXECUTABLE FALSE)

gamehq_add_test(tst_integrationclient
    tst_integrationclient.cpp
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationProtocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/integration/IntegrationClient.cpp")
target_link_libraries(tst_integrationclient PRIVATE Qt6::Network)
target_compile_definitions(tst_integrationclient PRIVATE
    INTEGRATION_SERVER_FIXTURE="$<TARGET_FILE:integration_server_fixture>")
add_dependencies(tst_integrationclient integration_server_fixture)

gamehq_add_test(tst_updatedownloader
    tst_updatedownloader.cpp
    "${CMAKE_SOURCE_DIR}/src/updates/UpdateDownloader.cpp")
target_link_libraries(tst_updatedownloader PRIVATE Qt6::Network)

gamehq_add_test(tst_updatepreflight
    tst_updatepreflight.cpp
    "${CMAKE_SOURCE_DIR}/src/updates/UpdatePreflight.cpp")

gamehq_add_test(tst_hdrtonemap
    tst_hdrtonemap.cpp
    "${CMAKE_SOURCE_DIR}/src/capture/hdr/HdrToneMapMath.cpp")

# GPU smoke test (t24): needs a real D3D11 device, so it is not "pure logic"
# like the tests above — it self-skips via QSKIP when no D3D11 hardware
# device or no FP16 support is available (see the test file header comment).
gamehq_add_test(tst_hdrgputonemap
    tst_hdrgputonemap.cpp
    "${CMAKE_SOURCE_DIR}/src/capture/hdr/HdrToneMapMath.cpp"
    "${CMAKE_SOURCE_DIR}/src/capture/hdr/GpuToneMapper.cpp")
target_link_libraries(tst_hdrgputonemap PRIVATE d3d11 d3dcompiler)

gamehq_add_test(tst_updateinstaller
    tst_updateinstaller.cpp
    "${CMAKE_SOURCE_DIR}/src/updates/UpdateInstaller.cpp"
    "${CMAKE_SOURCE_DIR}/src/updates/UpdateDownloader.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterTransaction.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/UpdaterHandshake.cpp")
target_link_libraries(tst_updateinstaller PRIVATE Qt6::Network)

gamehq_add_test(tst_updatertransaction
    tst_updatertransaction.cpp
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterDataSnapshot.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterSwap.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterHealth.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterRecovery.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/UpdateMaintenance.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterStaging.cpp"
    "${CMAKE_SOURCE_DIR}/src/updater/UpdaterTransaction.cpp"
    "${CMAKE_SOURCE_DIR}/src/launcher/UpdaterPromotion.cpp")
add_executable(updater_health_fixture updater_health_fixture.cpp)
target_link_options(updater_health_fixture PRIVATE -static -municode)
target_compile_definitions(tst_updatertransaction PRIVATE
    UPDATER_EXE="$<TARGET_FILE:GameHQUpdater>"
    HEALTH_FIXTURE_EXE="$<TARGET_FILE:updater_health_fixture>"
    GAMEHQ_UPDATER_VERSION="${GAMEHQ_VERSION_FULL}")
add_dependencies(tst_updatertransaction GameHQUpdater updater_health_fixture)
target_link_libraries(tst_updatertransaction PRIVATE miniz bcrypt)
