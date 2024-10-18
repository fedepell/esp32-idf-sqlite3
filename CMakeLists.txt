set(COMPONENT_ADD_INCLUDEDIRS "include")

idf_component_register(SRCS "sqlite3.c" "esp32.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "private_include"
                       REQUIRES mbedtls
                       PRIV_REQUIRES console spi_flash esp_timer)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu99
    -Wall
    -Wno-error=unused-value
    -Wno-error=unused-label
    -Wno-error=unused-function
    -Wno-error=unused-but-set-variable
    -Wno-error=deprecated-declarations
    -Wno-error=char-subscripts
    -Wno-error=unused-variable
    -Wno-error=maybe-uninitialized
    -MMD
    -c
    -DESP32
  )

    # =============================
    # @TODO  Double check these
    # eventually remove or restore
    # before merging!!
    # =============================

    # -fno-stack-protector
    # -fstrict-volatile-bitfields
    # -mlongcalls
    # -nostdlib
