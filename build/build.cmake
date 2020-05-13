set(root "${CMAKE_CURRENT_LIST_DIR}/..")

set(SRCS
    ${root}/src/Tga.h
    ${root}/src/Tga.cpp
    ${root}/src/main.cpp
)

set(BUILD
    ${root}/build/build.cmake
)

set(TEST_FILES
    ${root}/asset_db/textures/BonusHarrySelect_0.tga
    ${root}/asset_db/textures/BonusHarrySelect_1.tga
    ${root}/asset_db/textures/BonusHarrySelect_2.tga
    ${root}/asset_db/textures/BonusHarrySelect_3.tga
    ${root}/asset_db/textures/BonusHarrySelect_4.tga
    ${root}/asset_db/textures/BonusHarrySelect_5.tga
    ${root}/asset_db/textures/BonusHarrySelect_6.tga
    ${root}/asset_db/textures/BonusHarrySelect_7.tga
    ${root}/asset_db/textures/BonusHarrySelect_8.tga
    ${root}/asset_db/textures/BonusHarrySelect_9.tga
    ${root}/asset_db/textures/BonusHarrySelect_10.tga
    ${root}/asset_db/textures/BonusHarrySelect_11.tga
    ${root}/asset_db/textures/BonusHarrySelect_12.tga
    ${root}/asset_db/textures/BonusHarrySelect_13.tga
    ${root}/asset_db/textures/BonusHarrySelect_14.tga
    ${root}/asset_db/textures/BonusHarrySelect_15.tga
    ${root}/asset_db/textures/BonusHarrySelect_16.tga
    ${root}/asset_db/textures/BonusHarrySelect_17.tga
    ${root}/asset_db/textures/BonusHarrySelect_18.tga
    ${root}/asset_db/textures/BonusHarrySelect_19.tga
    ${root}/asset_db/textures/CompletionScreen.tga
)

add_executable(TextureProcessor  
	${SRCS} 
	${BUILD}
	${TEST_FILES}
)

settingsCR(TextureProcessor)		
createPCH(TextureProcessor)	
			
target_link_libraries(TextureProcessor 
	amdcompress
	cli11
    doctest
	fmt
    glm
	spdlog
	zstd
	core
	platform
	datacompression
)

source_group("Test Textures" FILES ${TEXTURE_PROCESSOR_TEST_FILES})
	
add_custom_command(TARGET TextureProcessor POST_BUILD       
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${amdcompress_dll_path}
      $<TARGET_FILE_DIR:TextureProcessor>          
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${TEST_FILES}
      $<TARGET_FILE_DIR:TextureProcessor>) 
		