# Generates the debug signing keystore used to sign the Android Remote Server APK.
#
# It exists as a script rather than an inline `keytool` command because `keytool` refuses to
# overwrite an existing alias, and CMake re-runs this rule every time anything it depends on (the
# freshly linked libraries) is rebuilt. Regenerating it unconditionally would also be wrong: it would
# silently change the APK's signing identity on every build, so an installed Remote Server would stop
# being upgradeable in place.
#
# An existing keystore is therefore left exactly as it is, and only a missing one is created. The
# rule's `restat` then lets downstream packaging run without touching the signature.
#
# Invoked as: cmake -DKEYTOOL=<path to keytool> -DKEYSTORE=<output path> -P this-file
if(EXISTS "${KEYSTORE}")
    return()
endif()

if(NOT DEFINED KEYTOOL OR NOT DEFINED KEYSTORE)
    message(FATAL_ERROR "MakeDebugKeystore.cmake requires KEYTOOL and KEYSTORE")
endif()

message(STATUS "Generating debug keystore ${KEYSTORE} (only happens once)")
execute_process(COMMAND "${KEYTOOL}" -genkey -keystore "${KEYSTORE}" -storepass android
                        -alias rdocandroidkey -keypass android -keyalg RSA -keysize 2048
                        -validity 10000 -dname "CN=, OU=, O=, L=, S=, C="
                RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "keytool failed to create ${KEYSTORE} (exit ${result})")
endif()
