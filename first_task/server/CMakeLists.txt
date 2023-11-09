set(OUTFILE server)

set(SOURCES 
    main.c
    server.c
    service.c
)

add_executable(${OUTFILE} ${SOURCES})