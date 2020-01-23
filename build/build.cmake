set(texture_processor_root "${CMAKE_CURRENT_LIST_DIR}/..")

set(TEXTURE_PROCESSOR_SRCS
    ${texture_processor_root}/src/Tga.h
    ${texture_processor_root}/src/Tga.cpp
    ${texture_processor_root}/src/main.cpp
)

set(TEXTURE_PROCESSOR_BUILD
    ${texture_processor_root}/build/build.cmake
)

set(TEXTURE_PROCESSOR_TEST_FILES
    ${texture_processor_root}/asset_db/textures/spencer_walk_0001.tga
    ${texture_processor_root}/asset_db/textures/CompletionScreen.tga
)

add_executable(TextureProcessor  
	${TEXTURE_PROCESSOR_SRCS} 
	${TEXTURE_PROCESSOR_BUILD}
	${TEXTURE_PROCESSOR_TEST_FILES}
)
	
target_link_libraries(TextureProcessor 
	cli11
	fmt
	spdlog
	zstd
	core
	platform
	datacompression
)

source_group("Build" FILES ${TEXTURE_PROCESSOR_BUILD})
source_group("Test Textures" FILES ${TEXTURE_PROCESSOR_TEST_FILES})
	
add_custom_command(TARGET TextureProcessor POST_BUILD        
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${texture_processor_root}/asset_db/textures/spencer_walk_0001.tga
      $<TARGET_FILE_DIR:TextureProcessor>     
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${texture_processor_root}/asset_db/textures/CompletionScreen.tga
      $<TARGET_FILE_DIR:TextureProcessor>)
		