juce_add_plugin(ProcessingNode
    PRODUCT_NAME "Distributed Audio Processing Node"
    COMPANY_NAME "Paul Bonargo")

juce_generate_juce_header(ProcessingNode)

target_sources(ProcessingNode PRIVATE
    Main.cpp
    MainComponent.cpp 
    ProcessingEngine.cpp 
)

target_include_directories(ProcessingNode PRIVATE ${CMAKE_SOURCE_DIR}/shared)

target_compile_definitions(ProcessingNode PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCEPLUGINHOST_AU=0)

target_link_libraries(ProcessingNode PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)