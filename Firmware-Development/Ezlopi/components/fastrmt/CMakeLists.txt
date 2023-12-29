# target_link_libraries(${COMPONENT_TARGET} "-u ld_include_xt_highint5")

set(srcs "src/fastrmt.S")

idf_component_register(	SRCS ${srcs}
						# INCLUDE_DIRS "include"
						# REQUIRES 
							# led
						)


#ld_include_highint_hdl is added as an undefined symbol because otherwise the
#linker will ignore highint_hdl.S as it has no other files depending on any
#symbols in it.
# set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u ld_include_xt_highint5")