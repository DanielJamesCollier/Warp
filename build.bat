cl /Feout/warp_client.exe %DEBUG_FLAGS% client.c utils.c /link %LINKER_FLAGS% 
cl /Feout/warp_server.exe %DEBUG_FLAGS% server.c utils.c /link %LINKER_FLAGS%
