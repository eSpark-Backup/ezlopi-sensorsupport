set(srcs "src/status.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES 
							app_update
							config
						)