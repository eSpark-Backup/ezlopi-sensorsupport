set(srcs "src/ownled.c" "src/gamma.c")

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "include"
						REQUIRES
							fastrmt
						)