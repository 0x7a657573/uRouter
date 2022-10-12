set(srcs "main.c"
         "app/debug/debugtask.c"
         "app/RTOSAnalysis/RTOSAnalysis.c"
         "app/pppos/pppos.c"
         "bsp/uart/uart.c"
         )
        
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "app" "bsp")
