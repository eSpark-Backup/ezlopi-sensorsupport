set(srcs "src/led.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES 
							ownled 
							status 
							ws2812fx 
							playlist
						)
