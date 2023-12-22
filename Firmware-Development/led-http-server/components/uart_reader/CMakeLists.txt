set(srcs "src/uart_reader.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES 
							json
							config
						)