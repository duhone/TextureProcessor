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
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_0.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_1.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_2.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_3.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_4.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_5.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_6.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_7.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_8.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_9.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_10.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_11.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_12.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_13.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_14.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_15.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_16.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_17.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_18.tga
    ${texture_processor_root}/asset_db/textures/BonusHarrySelect_19.tga
    ${texture_processor_root}/asset_db/textures/CompletionScreen.tga
)

add_executable(TextureProcessor  
	${TEXTURE_PROCESSOR_SRCS} 
	${TEXTURE_PROCESSOR_BUILD}
	${TEXTURE_PROCESSOR_TEST_FILES}
)
	
target_link_libraries(TextureProcessor 
	amdcompress
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
      ${amdcompress_root}/lib/x64/AMDCompress_MT_DLL.dll
      $<TARGET_FILE_DIR:TextureProcessor>          
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${TEXTURE_PROCESSOR_TEST_FILES}
      $<TARGET_FILE_DIR:TextureProcessor>) 
		