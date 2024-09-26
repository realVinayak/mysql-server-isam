SET(MEMEM_SOURCES  ha_memem.cc ha_memem.h)
MYSQL_ADD_PLUGIN(
    memem ${MEMEM_SOURCES} STORAGE_ENGINE
    LINK_LIBRARIES extra::rapidjson
    )
