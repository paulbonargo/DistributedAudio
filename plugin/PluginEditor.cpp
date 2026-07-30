/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================

namespace
{
	namespace Palette
	{
		const juce::Colour background { 0xfffff3e6 }; // #FFF3E6 deck body
		const juce::Colour panel      { 0xfffce5cd }; // #FCE5CD menu fill
		const juce::Colour outline    { 0xffd8a878 }; // #D8A878 borders
		const juce::Colour accent     { 0xffc35858 }; // #C35858 slider track
		const juce::Colour text       { 0xff2c1414 }; // #2C1414 primary text
		const juce::Colour textMuted  { 0xff4c2222 }; // #4C2222 secondary text
		const juce::Colour buttonBg   { 0xff4c2222 }; // #4C2222 button fill
		const juce::Colour buttonText { 0xfffff3e6 }; // #FFF3E6 button text
		const juce::Colour sliderBack { 0xffd8a878 }; // #D8A878 slider groove
	}

	void styleSlider(juce::Slider& s)
	{
		s.setColour (juce::Slider::trackColourId,           	Palette::accent);
		s.setColour (juce::Slider::backgroundColourId,      	Palette::sliderBack);
		s.setColour (juce::Slider::thumbColourId,           	Palette::text);
		s.setColour (juce::Slider::textBoxTextColourId,     	Palette::text);
		s.setColour (juce::Slider::textBoxBackgroundColourId, 	Palette::panel);
		s.setColour (juce::Slider::textBoxOutlineColourId, 		Palette::outline);
	}

    constexpr int kLatencyChoices[] = { 256, 512, 1024, 2048, 4096, 8192 };
    constexpr int kNumLatencyChoices = 6;
}


//==============================================================================



AudioSenderEditor::AudioSenderEditor(AudioSenderProcessor& p) : AudioProcessorEditor(&p), processorRef(p)
{
	// basic sender ui
	// setSize(300, 150);

	// advanced ui
	hostField.setText(processorRef.getNodeHost(), false);
    hostField.setTextToShowWhenEmpty("Node IP", Palette::textMuted);
	
	hostField.setColour (juce::TextEditor::backgroundColourId,      Palette::panel);
	hostField.setColour (juce::TextEditor::textColourId,            Palette::text);
	hostField.setColour (juce::TextEditor::outlineColourId,         Palette::outline);
	hostField.setColour (juce::TextEditor::focusedOutlineColourId,  Palette::accent);
	addAndMakeVisible(hostField);

    connectButton.onClick = [this] { processorRef.connectControl( hostField.getText().trim() ); };
    
	connectButton.setColour(juce::TextButton::buttonColourId,  Palette::buttonBg);
	connectButton.setColour(juce::TextButton::textColourOffId, Palette::buttonText);
	addAndMakeVisible(connectButton);

    pluginMenu.setTextWhenNothingSelected("(No Plugin)");

    pluginMenu.onChange = [this]
    {
        if (buildingUi) 
			return;
        
		const int id = pluginMenu.getSelectedId() - 1; // registry stores id+1
        
		if (id >= 0) 
			processorRef.getControlClient().selectPlugin(id);
	};

	pluginMenu.setColour(juce::ComboBox::backgroundColourId, Palette::panel);
	pluginMenu.setColour(juce::ComboBox::textColourId,       Palette::text);
	pluginMenu.setColour(juce::ComboBox::outlineColourId,    Palette::outline);
	pluginMenu.setColour(juce::ComboBox::arrowColourId,      Palette::accent);
	addAndMakeVisible(pluginMenu);

    statusLabel.setJustificationType(juce::Justification::topLeft);

	statusLabel.setColour(juce::Label::textColourId, Palette::textMuted);
    addAndMakeVisible(statusLabel);

    metricsLabel.setJustificationType(juce::Justification::topLeft);
    metricsLabel.setFont(juce::FontOptions("Consolas", 13.0f, juce::Font::plain));

    metricsLabel.setColour(juce::Label::textColourId, Palette::textMuted);
    addChildComponent(metricsLabel);

    metricsToggle.onClick = [this]
    {
        metricsLabel.setVisible(metricsToggle.getToggleState());
        resized();
    };

    metricsToggle.setColour(juce::ToggleButton::textColourId, Palette::textMuted);
    metricsToggle.setColour(juce::ToggleButton::tickColourId, Palette::accent);
    metricsToggle.setColour(juce::ToggleButton::tickDisabledColourId, Palette::buttonBg);
    addAndMakeVisible(metricsToggle);

    latencyLabel.setText("Latency", juce::dontSendNotification);
    
    latencyLabel.setColour(juce::Label::textColourId, Palette::textMuted);
    addAndMakeVisible(latencyLabel);

    {
        const double sr = processorRef.getSampleRate() > 0.0 ? processorRef.getSampleRate() : 48000.0;

        for (int i = 0; i < kNumLatencyChoices; ++i)
            latencyMenu.addItem(juce::String(kLatencyChoices[i]) + " smp  (" + juce::String(1000.0 * kLatencyChoices[i] / sr, 1) + " ms)", i + 1);

        // pre-select latency matching current base of processor
        const int current = processorRef.getLatencyBudgetBase();
        for (int i = 0; i < kNumLatencyChoices; ++i)
            if (kLatencyChoices[i] == current)
                latencyMenu.setSelectedId(i + 1, juce::dontSendNotification);
    }


    latencyMenu.onChange = [this]
    {
        if (buildingUi) 
            return;
        
        const int i = latencyMenu.getSelectedId() - 1;
        if (i >= 0 && i < kNumLatencyChoices)
            processorRef.setLatencyBudget(kLatencyChoices[i]);
    };
    
    latencyMenu.setColour(juce::ComboBox::outlineColourId,    Palette::outline);
    latencyMenu.setColour(juce::ComboBox::backgroundColourId, Palette::panel);
    latencyMenu.setColour(juce::ComboBox::textColourId,       Palette::text);
    latencyMenu.setColour(juce::ComboBox::arrowColourId,      Palette::accent);
    addAndMakeVisible(latencyMenu);

    resetMetricsButton.onClick = [this] { processorRef.resetMetrics(); };
    
    resetMetricsButton.setColour(juce::TextButton::buttonColourId,  Palette::buttonBg);
    resetMetricsButton.setColour(juce::TextButton::textColourOffId, Palette::buttonText);
    addAndMakeVisible(resetMetricsButton);

    paramViewport.setViewedComponent(&paramHolder, false);
    paramViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(paramViewport);

	paramViewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, Palette::outline);

    processorRef.getControlClient().onStateChanged = [safe = juce::Component::SafePointer<AudioSenderEditor>(this)] 
	{
		if (safe != nullptr) 
			safe -> rebuildFromControlState(); 
	};

    setSize(420, 460);

    rebuildFromControlState();
    
    startTimerHz(5);
}

AudioSenderEditor::~AudioSenderEditor()
{
    stopTimer();
    processorRef.getControlClient().onStateChanged = nullptr;
}

void AudioSenderEditor::timerCallback()
{
    auto& cc = processorRef.getControlClient();

    statusLabel.setText(juce::String(cc.isConnectedToNode() ? "Connected" : "Not Connected") + "   |   " + (cc.getSelectedPluginName().isNotEmpty() ? cc.getSelectedPluginName() : juce::String("(No Plugin)")), juce::dontSendNotification);

    if (! metricsLabel.isVisible())
        return; // when hidden return nothing

    const double sr = processorRef.getSampleRate() > 0.0 ? processorRef.getSampleRate() : 48000.0;
    const int total = processorRef.getLatencyTotal();

    auto row = [](const juce::String& name, const juce::String& value, const juce::String& extra = {})
    {
        return name.paddedRight(' ', 11) + value.paddedLeft(' ', 8)
            + (extra.isNotEmpty() ? "   " + extra : juce::String()) + "\n";
    };

    juce::String m;

    m       << row("Latency",   juce::String(total), juce::String(1000.0 * total / sr, 1) + " ms")
            << row("  base",    juce::String(processorRef.getLatencyBudgetBase()),"+ plugin " + juce::String(processorRef.getRemotePluginLatency()))
    << "\n" << row("Sent",      juce::String(processorRef.getPacketsSent()))
            << row("Received",  juce::String(processorRef.getPacketsReceived()))
            << row("Lost",      juce::String(processorRef.getPacketsLost()), juce::String(processorRef.getReturnLossPercent(), 3) + " %")
    << "\n" << row("Blocks",    juce::String(processorRef.getBlocksProcessed()))
            << row("Underruns", juce::String(processorRef.getUnderruns()), juce::String(processorRef.getUnderrunRatePercent(), 3) + " %");

    metricsLabel.setText(m, juce::dontSendNotification);
}

void AudioSenderEditor::paint(juce::Graphics& g)
{
	// basic sender ui
	g.fillAll(Palette::background);
	g.setColour(Palette::textMuted);
	g.setFont(17.0f);
	g.drawFittedText("Distributed Audio Sender", getLocalBounds().removeFromTop(28), juce::Justification::centred, 1);
}

void AudioSenderEditor::rebuildFromControlState()
{
    buildingUi = true;
    auto& cc = processorRef.getControlClient();

    pluginMenu.clear(juce::dontSendNotification);
    for (auto& info : cc.getPluginList())
        pluginMenu.addItem(info.name, info.id + 1); // id offset by 1 so id 0 is a valid id (1) for ComboBox

    const juce::String selected = cc.getSelectedPluginName();
    if (selected.isNotEmpty()) pluginMenu.setText(selected, juce::dontSendNotification);

    // rebuild the param rows from control state
    const auto params = cc.getParameters();
    paramRows.clear();
    paramHolder.removeAllChildren();

    for (auto& pinfo : params)
    {
        ParamRow row;
        row.index = pinfo.index;

        row.label = std::make_unique<juce::Label>();
        row.label -> setText(pinfo.name, juce::dontSendNotification);

		row.label -> setColour(juce::Label::textColourId, Palette::textMuted);
        paramHolder.addAndMakeVisible(*row.label);

        row.slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        row.slider -> setRange(0.0, 1.0, 0.0001);
        row.slider -> setValue(pinfo.value, juce::dontSendNotification);
		styleSlider(*row.slider);

        auto* sliderPtr = row.slider.get();
        const int idx = pinfo.index;
        row.slider->onValueChange = [this, idx, sliderPtr]
        {
            if (! buildingUi)
                processorRef.getControlClient().setParameter(idx, (float) sliderPtr->getValue());
        };
        paramHolder.addAndMakeVisible(*row.slider);
        paramRows.push_back(std::move(row));
    }

    buildingUi = false;
    resized();
}


void AudioSenderEditor::resized()
{
	auto r = getLocalBounds().reduced(10);
    r.removeFromTop(28);

    auto row1 = r.removeFromTop(28);
    connectButton.setBounds(row1.removeFromRight(90));

    row1.removeFromRight(6);
    hostField.setBounds(row1);
    
	r.removeFromTop(6);
    pluginMenu.setBounds(r.removeFromTop(28));

	r.removeFromTop(6);
    auto latencyRow = r.removeFromTop(26);
    resetMetricsButton.setBounds(latencyRow.removeFromRight(110));
    
    latencyRow.removeFromRight(6);
    latencyLabel.setBounds(latencyRow.removeFromLeft(56));
    latencyMenu.setBounds(latencyRow);
    
    r.removeFromTop(6);
    auto statusRow = r.removeFromTop(22);
    metricsToggle.setBounds(statusRow.removeFromRight(84));
    statusLabel.setBounds(statusRow);

    if (metricsLabel.isVisible())
    {
        r.removeFromTop(4);
        metricsLabel.setBounds(r.removeFromTop(132));
    }

    r.removeFromTop(6);
    paramViewport.setBounds(r);

    // height grows w/ parameter count
    const int rowH = 26;
    paramHolder.setSize(juce::jmax(paramViewport.getWidth() - 8, 100), juce::jmax((int) paramRows.size() * rowH, 1));
    int y = 0;

    for (auto& pr : paramRows)
    {
        pr.label -> setBounds(0, y, 150, rowH);
        pr.slider -> setBounds(154, y, paramHolder.getWidth() - 154, rowH);
        y += rowH;
    }
}
