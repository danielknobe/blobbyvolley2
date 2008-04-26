macro(add_zip_archive _dir _filetype)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip
		COMMAND zip ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip ${_dir}/*.${_filetype}
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/*.${_filetype}
		VERBATIM
		)
	add_custom_target(${_dir}_zip ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip)
#	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_dir].zip DESTINATION /usr)
endmacro(add_zip_archive _dir _filetype)


add_zip_archive(gfx bmp)
add_zip_archive(sounds wav)
add_zip_archive(scripts lua)

if (WIN32)
	install(FILES gfx.zip sounds.zip scripts.zip DESTINATION data)
elif (UNIX)
	install(FILES gfx.zip sounds.zip scripts.zip DESTINATION share/blobby)
endif (WIN32)
