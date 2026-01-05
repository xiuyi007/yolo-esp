set(src_dirs        ./)

set(include_dirs    ./)

set(requires        pest_detect esp_wifi nvs_flash esp_http_client)


set(embed_files     "pest.jpg")

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires} EMBED_FILES ${embed_files})
