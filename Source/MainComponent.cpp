//=======================================================================================================================================
//File [2] - Main Component

#include <JuceHeader.h>
#include <iostream>
#include "TransportMixerAudioSource.cpp"

using namespace std;
using namespace juce;

class TableComponent: public juce::AudioAppComponent, public juce::TableListBoxModel, private KeyListener {
public:
    
    
    
    //===================================================================================================================================
    //Section [1] -- Constructor
    TableComponent()
    {
        //Subsection [a] -- Create 'Add Audio Cue' Button and its Function
        addAudioCueButton.onClick = [this] { addAudioCue(getNumRows()+1, "", ""); };
        addAndMakeVisible (addAudioCueButton);
        
//        addAndMakeVisible (addGroupButton);
//        addAndMakeVisible (addFadeButton);
//        addAndMakeVisible (addPlayCueButton);
//        addAndMakeVisible (addStopCueButton);
//        addAndMakeVisible (addPauseCueButton);
        
        //Subsection [b] -- Set Placeholder Text for Editing a Cue
        number.setTextToShowWhenEmpty("Enter Cue Number",juce::Colours::grey);
        name.setTextToShowWhenEmpty("Enter Cue Name",juce::Colours::grey);
        target.setTextToShowWhenEmpty("Enter Target File or Target Cue",juce::Colours::grey);
        
        //Subsection [c] -- Set the Function for the 'Update Cue' and 'Search for Target' Button
        updateCueButton.onClick = [this] { updateCueParams(number.getText(), name.getText(), target.getText()); };
        searchForTargetButton.onClick = [this] { searchForTarget(); };
        
        //Subsection [e] -- Only enable Cue Buttons when File is loaded here
        if(currentFile.getFullPathName() == ""){
            addAudioCueButton.setEnabled(false);
//            addGroupButton.setEnabled(false);
//            addFadeButton.setEnabled(false);
//            addPlayCueButton.setEnabled(false);
//            addStopCueButton.setEnabled(false);
//            addPauseCueButton.setEnabled(false);
        }
        
        //Subsection [f] -- Create 'New Project' and 'Open Project' Button and its Function
        addAndMakeVisible (newProject);
        newProject.onClick = [this] { newProjectWorkspace(); };
        addAndMakeVisible (openProject);
        openProject.onClick = [this] { openProjectWorkspace(); };

        //Subsection [g] -- Create Table
        addAndMakeVisible (table);
        table.setModel(this);
        table.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);
        table.setOutlineThickness(1);

        //Subsection [h] -- Get Necessary Formats and Permissions for Playing Audio
        formatManager.registerBasicFormats();
        if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio) && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio)){
            juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio, [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
        } else {
            setAudioChannels (2, 2);
        }
        
        //Subsection [i] -- Create Keyboard Listener
        setWantsKeyboardFocus(true);
        addKeyListener(this);
    }
    
    //Deconstructor
    ~TableComponent() override { shutdownAudio(); }
    //====================================================================================================================================
    
    
    
    //====================================================================================================================================
    //Section [2] -- Playing Audio Helper Methods
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override {
         mixer.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }
    void releaseResources() override {
        mixer.releaseResources();
    }
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override {
        if(rowNumSelected != 0){
            if (readerSource.get() == nullptr){
                bufferToFill.clearActiveBufferRegion();
                return;
            }
                        
            mixer.getNextAudioBlock(bufferToFill);
        }
    }
    //====================================================================================================================================
    
    
    
    //====================================================================================================================================
    //Section [3] -- User Input Helper Methods
    
    //Subsection [a] -- Method to Start a Project
    void newProjectWorkspace(){
        if(dirChooser.browseForDirectory()){
            if(filesOpen > 0){
                dataList = nullptr;
                columnList = nullptr;
                table.getHeader().removeAllColumns();
                table.setModel(nullptr);
                filesOpen--;
                table.updateContent();
                repaint();
                
                table.setModel(this);
            
                juce::String dir = dirChooser.getResult().getFullPathName();
                            
                XmlDocument file (juce::File(dir+"/Untitled.xml"));

                XmlElement headTag (juce::String("TABLE_DATA"));
                
                XmlElement* headersTag = new XmlElement("HEADERS");
                headTag.addChildElement(headersTag);

                XmlElement* numColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(numColTag);
                XmlElement* nameColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(nameColTag);
                XmlElement* targetColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(targetColTag);

                numColTag->setAttribute("name", "Number");
                numColTag->setAttribute("columnId", "1");
                numColTag->setAttribute("width", "200");

                nameColTag->setAttribute("name", "Name");
                nameColTag->setAttribute("columnId", "2");
                nameColTag->setAttribute("width", "200");

                targetColTag->setAttribute("name", "Target");
                targetColTag->setAttribute("columnId", "3");
                targetColTag->setAttribute("width", "200");

                XmlElement* dataTag = new XmlElement("DATA");
                headTag.addChildElement(dataTag);
                
                headTag.XmlElement::writeTo(juce::File(dir+"/Untitled.xml"), XmlElement::TextFormat());
                
                setCurrentFile(dir+"/Untitled.xml");
                            
                if (headersTag != nullptr){
                    forEachXmlChildElement (*headersTag, columnXml){
                        table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
                                                     columnXml->getIntAttribute ("columnId"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
                        
                        numCols++;
                        
                    }
                }
                
                loadData(dir+"/Untitled.xml");
                
                table.updateContent();
                repaint();
                
                if(currentFile.getFullPathName() != ""){
                    addAudioCueButton.setEnabled(true);
                    addGroupButton.setEnabled(false);
                    addFadeButton.setEnabled(false);
                    addPlayCueButton.setEnabled(false);
                    addStopCueButton.setEnabled(false);
                    addPauseCueButton.setEnabled(false);
                }
                
                filesOpen++;
            } else {
                juce::String dir = dirChooser.getResult().getFullPathName();
                            
                XmlDocument file (juce::File(dir+"/Untitled.xml"));

                XmlElement headTag (juce::String("TABLE_DATA"));
                
                XmlElement* headersTag = new XmlElement("HEADERS");
                headTag.addChildElement(headersTag);

                XmlElement* numColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(numColTag);
                XmlElement* nameColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(nameColTag);
                XmlElement* targetColTag = new XmlElement("COLUMN");
                headersTag->addChildElement(targetColTag);

                numColTag->setAttribute("name", "Number");
                numColTag->setAttribute("columnId", "1");
                numColTag->setAttribute("width", "200");

                nameColTag->setAttribute("name", "Name");
                nameColTag->setAttribute("columnId", "2");
                nameColTag->setAttribute("width", "200");

                targetColTag->setAttribute("name", "Target");
                targetColTag->setAttribute("columnId", "3");
                targetColTag->setAttribute("width", "200");

                XmlElement* dataTag = new XmlElement("DATA");
                headTag.addChildElement(dataTag);
                
                headTag.XmlElement::writeTo(juce::File(dir+"/Untitled.xml"), XmlElement::TextFormat());
                
                setCurrentFile(dir+"/Untitled.xml");
                            
                if (headersTag != nullptr){
                    forEachXmlChildElement (*headersTag, columnXml){
                        table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
                                                     columnXml->getIntAttribute ("columnId"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
                        
                        numCols++;
                        
                    }
                }
                
                loadData(dir+"/Untitled.xml");
                
                table.updateContent();
                repaint();
                
                if(currentFile.getFullPathName() != ""){
                    addAudioCueButton.setEnabled(true);
                    addGroupButton.setEnabled(false);
                    addFadeButton.setEnabled(false);
                    addPlayCueButton.setEnabled(false);
                    addStopCueButton.setEnabled(false);
                    addPauseCueButton.setEnabled(false);
                }
                
                filesOpen++;
            }
        }
    }
    
    //Subsection [b] -- Method to Open a Project
    void openProjectWorkspace(){
        if(chooser.browseForFileToOpen(nullptr)){
            if(filesOpen > 0){
                dataList = nullptr;
                columnList = nullptr;
                table.getHeader().removeAllColumns();
                table.setModel(nullptr);
                filesOpen--;
                table.updateContent();
                repaint();
                
                table.setModel(this);
                
                juce::String file = chooser.getResult().getFullPathName();
                
                setCurrentFile(file);
                            
                loadData(currentFile);
                
                if (columnList != nullptr){
                    forEachXmlChildElement (*columnList, columnXml){
                        table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
                                                     columnXml->getIntAttribute ("columnId"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
                        
                        numCols++;
                        
                    }
                }
                
                table.updateContent();
                repaint();
                
                if(currentFile.getFullPathName() != ""){
                    addAudioCueButton.setEnabled(true);
                    addGroupButton.setEnabled(false);
                    addFadeButton.setEnabled(false);
                    addPlayCueButton.setEnabled(false);
                    addStopCueButton.setEnabled(false);
                    addPauseCueButton.setEnabled(false);
                }
                
                filesOpen++;
            } else {
                juce::String file = chooser.getResult().getFullPathName();
                
                setCurrentFile(file);
                            
                loadData(currentFile);
                
                if (columnList != nullptr){
                    forEachXmlChildElement (*columnList, columnXml){
                        table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
                                                     columnXml->getIntAttribute ("columnId"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     columnXml->getIntAttribute ("width"),
                                                     juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
                        
                        numCols++;
                        
                    }
                }
                
                table.updateContent();
                repaint();
                
                if(currentFile.getFullPathName() != ""){
                    addAudioCueButton.setEnabled(true);
                    addGroupButton.setEnabled(false);
                    addFadeButton.setEnabled(false);
                    addPlayCueButton.setEnabled(false);
                    addStopCueButton.setEnabled(false);
                    addPauseCueButton.setEnabled(false);
                }
                
                filesOpen++;
            }
        }
    }
    
    //Subsection [c] -- Method to Search for a Target File to add to a Cue
    void searchForTarget(){
        if(tarChooser.browseForFileToOpen(nullptr)){
            juce::String file = tarChooser.getResult().getFullPathName();
            
            target.setText(file);
        }
    }
    
    //Subsection [d] -- Method to Add a Audio Cue to the GUI Table and the XML File
    void addAudioCue(int num, juce::String name, juce::String filePath) {
        loadData(currentFile);
        
        XmlElement* newItem = dataList->createNewChildElement("ITEM");
        
        newItem->setAttribute("Number", num);
        newItem->setAttribute("Name", name);
        newItem->setAttribute("Target", filePath);
                
        data->XmlElement::writeTo(currentFile, XmlElement::TextFormat());
        
        numRows++;
        
        table.updateContent();
        repaint();
    }
    
    //Subsection [e] -- Method to Update Cue
    void updateCueParams(juce::String num, juce::String name, juce::String filePath){
        XmlElement* cueToUpdate = dataList->getChildElement(rowNumSelected-1);
        
        cueToUpdate->setAttribute("Number", num);
        cueToUpdate->setAttribute("Name", name);
        cueToUpdate->setAttribute("Target", filePath);
        
        data->XmlElement::writeTo(currentFile, XmlElement::TextFormat());

        table.updateContent();
        repaint();
                
        this->number.clear();
        this->name.clear();
        this->target.clear();
    }
    
    //Subsection [f] -- Method to Delete Cue when selected and pressed
    void deleteKeyPressed (int currentSelectedRow) override {
        XmlElement* existingItem = dataList->getChildElement(rowNumSelected-1);

        dataList->removeChildElement(existingItem, true);
        
        data->XmlElement::writeTo(juce::File(currentFile), XmlElement::TextFormat());
        
        numRows--;
        
        table.updateContent();
        repaint();
    }
    
    bool keyPressed(const KeyPress &k, Component *c) override {
        //Subsection [g] -- Method Part (a) to Play Cue when selected and pressed
        if( k.getKeyCode() == juce::KeyPress::spaceKey ) {
            
            auto file = getAttributeFileForRowId(rowNumSelected-1);
            
            auto* reader = formatManager.createReaderFor (file);
            
            if (reader != nullptr) {
                std::unique_ptr<juce::AudioFormatReaderSource> newSource (new juce::AudioFormatReaderSource (reader, true));
            
                transportSource[rowNumSelected].setSource (newSource.get(), 0, nullptr, reader->sampleRate);
                                                        
                mixer.addInputSource(&transportSource[rowNumSelected], false);
                
                readerSource.reset (newSource.release());
            }
            
            transportSource[rowNumSelected].start();
            
            file = "";
            reader = nullptr;
            
            table.selectRow(rowNumSelected, false, true);
                        
            return true;

        }
        //Subsection [h] -- Method Part (a) to Play Cue when selected and pressed
        if( k.getKeyCode() == juce::KeyPress::escapeKey ) {
        
            releaseResources();
//            transportSource[rowNumSelected].setPosition (0.0);
            
            return true;
        }
            
        return false;
    }
    //=======================================================================================================================================

    
    
    //=======================================================================================================================================
    //Section [4] -- GUI Helper Methods
    
    //Subsection [a] -- Get the Current Number of Rows
    int getNumRows() override {
        return numRows;
    }
    
    //Subsection [b] -- Get the Current Number of Columns
    int getNumCols() {
        return numCols;
    }
    
    //Subsection [c] -- Sets the Current File Open
    void setCurrentFile(juce::String file){
        currentFile = juce::File(file);
    }

    //Subsection [d] -- Paints the Alternating Row Colors
    void paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        auto alternateColour = getLookAndFeel().findColour(juce::ListBox::backgroundColourId)
                                               .interpolatedWith(getLookAndFeel()
                                               .findColour(juce::ListBox::textColourId), 0.03f);
        if (rowIsSelected){
            g.fillAll(juce::Colours::lightblue);
            rowNumSelected = rowNumber+1;
        }else if (rowNumber % 2){
            g.fillAll(alternateColour);
        }
        
        if(rowNumSelected > 0){
            addAndMakeVisible (number);
            addAndMakeVisible (name);
            addAndMakeVisible (target);
            addAndMakeVisible (updateCueButton);
            addAndMakeVisible (searchForTargetButton);
        } else {
            this->removeChildComponent (&number);
            number.setVisible (false);
            this->removeChildComponent (&name);
            name.setVisible (false);
            this->removeChildComponent (&target);
            target.setVisible (false);
            this->removeChildComponent (&updateCueButton);
            updateCueButton.setVisible(false);
            this->removeChildComponent (&searchForTargetButton);
            searchForTargetButton.setVisible(false);
        }
    }

    //Subsection [e] -- Paints each cell, and enter's the data's text
    void paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        g.setColour (rowIsSelected ? juce::Colours::darkblue : getLookAndFeel().findColour (juce::ListBox::textColourId));
        g.setFont (font);
        
        if (auto* rowElement = dataList->getChildElement (rowNumber))
        {
            auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (columnId));
            
            g.drawText (text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }

        g.setColour (getLookAndFeel().findColour (juce::ListBox::backgroundColourId));
        g.fillRect (width - 1, 0, 1, height);
    }
    
    //Subsection [f] -- Sets Sizes, and Bounds
    void resized() override {
        table.setBounds(0, 25, getWidth(), getHeight()-175);
        
        newProject.setBounds(0, 0, 100, 25);
        openProject.setBounds(100, 0, 100, 25);
        
        addAudioCueButton.setBounds(300, 0, 100, 25);
        addGroupButton.setBounds(450, 0, 100, 25);
        addFadeButton.setBounds(600, 0, 100, 25);
        addPlayCueButton.setBounds(750, 0, 100, 25);
        addStopCueButton.setBounds(850, 0, 100, 25);
        addPauseCueButton.setBounds(950, 0, 100, 25);
        
        number.setBounds(25, getHeight()-125, 250, 25);
        name.setBounds(25, getHeight()-100, 250, 25);
        target.setBounds(25, getHeight()-75, 250, 25);
        
        updateCueButton.setBounds(25, getHeight()-50, 250, 25);
        
        searchForTargetButton.setBounds(275, getHeight()-75, 25, 25);
    }
    //=======================================================================================================================================

    
    
private:
    
    
    
    //=======================================================================================================================================
    //Section [5] - Private Variables
    
    //Subsection [a] -- Audio Helper Variables
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    TransportMixerAudioSource mixer;
    AudioTransportSource transportSource [1024];
    
    //Subsection [b] -- GUI Variables
    juce::TableListBox table  { {}, this };
    juce::TextButton addGroupButton {"Group"};
    juce::TextButton addAudioCueButton {"Audio"};
    juce::TextButton addFadeButton {"Fade"};
    juce::TextButton addPlayCueButton {"Play"};
    juce::TextButton addStopCueButton {"Stop"};
    juce::TextButton addPauseCueButton {"Pause"};

    juce::TextEditor number;
    juce::TextEditor name;
    juce::TextEditor target;
    juce::TextButton updateCueButton {"Update Cue"};
    juce::TextButton searchForTargetButton {"..."};
    juce::TextButton newProject {"New"};
    juce::TextButton openProject {"Open"};
    juce::Font font           { 14.0f };
    
    //Subsection [c] -- Data Loading Variables
    std::unique_ptr<juce::XmlElement> data;
    juce::XmlElement* columnList = nullptr;
    juce::XmlElement* dataList = nullptr;
    int numRows = 0;
    int numCols = 0;
    int rowNumSelected = 0;
    int filesOpen = 0;
    
    FileChooser chooser {"Choose a Project to open ...", juce::File::getCurrentWorkingDirectory(), "", false, this};
    FileChooser dirChooser {"Choose a Directory to start your new project ...", juce::File::getCurrentWorkingDirectory(), "", false, this};
    FileChooser tarChooser {"Choose a File for this Cue ...", juce::File::getCurrentWorkingDirectory(), "", false, this};
    
    juce::File currentFile = juce::File("");
    
    //=======================================================================================================================================
    
    
    
    //=======================================================================================================================================
    //Section [6] -- Load Data Helper Methods
    void loadData(juce::File file)
    {
        if (file.exists())
        {
            data = juce::XmlDocument::parse (file);

            dataList   = data->getChildByName ("DATA");
            columnList = data->getChildByName ("HEADERS");

            numRows = dataList->getNumChildElements();
        }
    }

    juce::String getAttributeNameForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("name");
        }

        return {};
    }
    
    juce::String getAttributeNumberForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("columnId");
        }

        return {};
    }
    
    juce::String getAttributeFileForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("target");
        }

        return {};
    }
    
    juce::String getAttributeFileForRowId (const int rowId) const
    {
        if (auto* rowElement = dataList->getChildElement(rowId))
        {
            auto file = rowElement->getStringAttribute (getAttributeNameForColumnId (3));
            
            return file;
        }
    }
    //=======================================================================================================================================
};



class MainComponent : public juce::Component
{
public:
    MainComponent() : juce::Component()
    {
        //Add table to the window and make visible
        addAndMakeVisible (table);
        
        //Set window size
        setSize (1200, 625);
    }
    
    void resized() override
    {
        table.setBounds (getLocalBounds());
    }

private:
    //Definition of the table
    TableComponent table;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

//=======================================================================================================================================

