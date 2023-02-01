idf_component_register(SRCS "measure.c" "http-rest.c" "wifi-manager.c" "i2c-driver.c" "main.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES root_cert.pem)
