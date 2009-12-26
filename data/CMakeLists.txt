macro(add_zip_archive _dir _filetype)
	file(GLOB archive_src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${_dir}/*.${_filetype})
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip
		COMMAND zip ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip ${archive_src}
		DEPENDS ${archive_src}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
		)
	add_custom_target(${_dir}_zip ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip)
#	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_dir].zip DESTINATION /usr)
endmacro(add_zip_archive _dir _filetype)


add_zip_archive(gfx bmp)
add_zip_archive(sounds wav)
add_zip_archive(scripts lua)
add_zip_archive(backgrounds bmp)

set(install_files
	${CMAKE_CURRENT_BINARY_DIR}/gfx.zip
	${CMAKE_CURRENT_BINARY_DIR}/sounds.zip
	${CMAKE_CURRENT_BINARY_DIR}/scripts.zip
	${CMAKE_CURRENT_BINARY_DIR}/backgrounds.zip
	config.xml
	inputconfig.xml
	server.xml)

if (WIN32)
	install(FILES ${install_files} DESTINATION data)
elseif (UNIX)
	install(FILES ${install_files} DESTINATION share/blobby)
endif (WIN32)
