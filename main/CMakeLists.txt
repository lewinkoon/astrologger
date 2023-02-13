idf_component_register(SRCS "read-data.c" "sync-time.c" "blink-led.c" "http-rest.c" "wifi-manager.c" "i2c-driver.c" "main.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES root_cert.pem)
