juce_add_plugin(AudioSenderPlugin
    PLUGIN_MANUFACTURER_CODE DIST
    PLUGIN_CODE SEND
    FORMATS VST3 Standalone
    PRODUCT_NAME "Distributed Audio Sender"
    COMPANY_NAME "Paul Bonargo"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE)

juce_generate_juce_header(AudioSenderPlugin)

target_sources(AudioSenderPlugin PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp 
    SenderThread.cpp)

target_include_directories(AudioSenderPlugin PRIVATE ${CMAKE_SOURCE_DIR}/shared)

target_compile_definitions(AudioSenderPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AudioSenderPlugin PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)