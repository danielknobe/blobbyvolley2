if (UNIX AND APPLE)
	if(BUILD_MACOS_BUNDLE)
		# enable bundle
		set_target_properties(blobby PROPERTIES MACOSX_BUNDLE TRUE)

		# add data folder content and bundle icon to bundle
		set(icns_file "${CMAKE_CURRENT_LIST_DIR}/icon.icns")
		target_sources(blobby PUBLIC ${data_files} ${icns_file})
		set_target_properties(blobby PROPERTIES RESOURCE "${data_files};${icns_file}")

		# add plist
		set_target_properties(blobby PROPERTIES 
		                             XCODE_ATTRIBUTE_GENERATE_INFOPLIST_FILE YES
		                             XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_LIST_DIR}/Info.plist")

		# Allow c++ code to change behavior
		target_compile_definitions(blobby PUBLIC BUILD_MACOS_BUNDLE)

		# add libs to bundle
		install(CODE [=[ include(BundleUtilities)
		                 fixup_bundle("${CMAKE_INSTALL_PREFIX}/bin/blobby.app" "" "") ]=])
	endif()
endif()
