set(srcs "src/ws2812fx.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES 
							led
						)