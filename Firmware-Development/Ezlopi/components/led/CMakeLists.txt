set(srcs "src/led.c" "src/effect_list_json.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES 
							ownled 
							# status 
							ws2812fx 
							# playlist
							json
						)
