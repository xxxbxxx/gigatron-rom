#include <vector>
#include <algorithm>

#if defined(_WIN32)
#include "dirent/dirent.h"
#else
#include <dirent.h>
#endif

#include <SDL.h>
#include "cpu.h"
#include "audio.h"
#include "editor.h"
#include "loader.h"
#include "timing.h"
#include "graphics.h"
#include "assembler.h"


namespace Editor
{
    struct FileEntry
    {
        FileType _fileType;
        std::string _name;
    };


    int _cursorX = 0;
    int _cursorY = 0;
    bool _hexEdit = false;
    bool _singleStep = false;
    bool _singleStepMode = false;
    uint32_t _singleStepTicks = 0;
    uint8_t _singleStepWatch = 0x00;
    const std::string _basePath = "./vCPU";
    std::string _filePath = "";
    bool _startMusic = false;

    MemoryMode _memoryMode = RAM;
    EditorMode _editorMode = Hex, _prevEditorMode = Hex;
    uint8_t _memoryDigit = 0;
    uint8_t _addressDigit = 0;
    uint16_t _hexBaseAddress = HEX_BASE_ADDRESS;
    uint16_t _loadBaseAddress = LOAD_BASE_ADDRESS;
    uint16_t _varsBaseAddress = VARS_BASE_ADDRESS;
    uint16_t _singleStepWatchAddress = VIDEO_Y_ADDRESS;
    
    int _fileEntriesIndex = 0;
    std::vector<FileEntry> _fileEntries;


    int getCursorX(void) {return _cursorX;}
    int getCursorY(void) {return _cursorY;}
    bool getHexEdit(void) {return _hexEdit;}
    bool getStartMusic(void) {return _startMusic;}
    bool getSingleStep(void) {return _singleStep;}
    bool getSingleStepMode(void) {return _singleStepMode;}
    MemoryMode getMemoryMode(void) {return _memoryMode;}
    EditorMode getEditorMode(void) {return _editorMode;}
    uint8_t getMemoryDigit(void) {return _memoryDigit;}
    uint8_t getAddressDigit(void) {return _addressDigit;}
    uint16_t getHexBaseAddress(void) {return _hexBaseAddress;}
    uint16_t getLoadBaseAddress(void) {return _loadBaseAddress;}
    uint16_t getVarsBaseAddress(void) {return _varsBaseAddress;}
    uint16_t getSingleStepWatchAddress(void) {return _singleStepWatchAddress;}
    int getFileEntriesIndex(void) {return _fileEntriesIndex;}
    int getFileEntriesSize(void) {return int(_fileEntries.size());}
    std::string getBrowserPath(void) {return _basePath + _filePath;}
    FileType getFileEntryType(int index) {return _fileEntries[index % _fileEntries.size()]._fileType;}
    std::string* getFileEntryName(int index) {return &_fileEntries[index % _fileEntries.size()]._name;}

    void setCursorX(int x) {_cursorX = x;}
    void setCursorY(int y) {_cursorY = y;}
    void setStartMusic(bool startMusic) {_startMusic = startMusic;}
    void setSingleStep(bool singleStep) {_singleStep = singleStep;}
    void setSingleStepMode(bool singleStepMode) {_singleStepMode = singleStepMode;}
    void setLoadBaseAddress(uint16_t address) {_loadBaseAddress = address;}
    void setSingleStepWatchAddress(uint16_t address) {_singleStepWatchAddress = address;}


    void handleMouseWheel(const SDL_Event& event)
    {
        if(event.wheel.y > 0)
        {
            if(_editorMode == Load)
            {
                _fileEntriesIndex--;
                if(_fileEntriesIndex < 0) _fileEntriesIndex = 0;
            }
            else
            {
                _hexBaseAddress = (_hexBaseAddress - HEX_CHARS_X) & (RAM_SIZE-1);
            }
        }
        if(event.wheel.y < 0)
        {
            if(_editorMode == Load)
            {
                if(_fileEntries.size() > HEX_CHARS_Y)
                {
                    _fileEntriesIndex++;
                    if(_fileEntries.size() - _fileEntriesIndex < HEX_CHARS_Y) _fileEntriesIndex--;
                }
            }
            else
            {
                _hexBaseAddress = (_hexBaseAddress + HEX_CHARS_X) & (RAM_SIZE-1);
            }
        }
    }

    void handleKeyDown(SDL_Keycode keyCode)
    {
        int limitY = (_editorMode != Load) ? HEX_CHARS_Y : int(_fileEntries.size());

        switch(keyCode)
        {
            case SDLK_d:      Cpu::setIN(Cpu::getIN() & ~INPUT_RIGHT);   break;
            case SDLK_a:      Cpu::setIN(Cpu::getIN() & ~INPUT_LEFT);    break;
            case SDLK_s:      Cpu::setIN(Cpu::getIN() & ~INPUT_DOWN);    break;
            case SDLK_w:      Cpu::setIN(Cpu::getIN() & ~INPUT_UP);      break;
            case SDLK_SPACE:  Cpu::setIN(Cpu::getIN() & ~INPUT_START);   break;
            case SDLK_z:      Cpu::setIN(Cpu::getIN() & ~INPUT_SELECT);  break;
            case SDLK_SLASH:  Cpu::setIN(Cpu::getIN() & ~INPUT_B);       break;
            case SDLK_PERIOD: Cpu::setIN(Cpu::getIN() & ~INPUT_A);       break;

            case SDLK_RIGHT: _cursorX = (++_cursorX >= HEX_CHARS_X) ? 0  : _cursorX;  _memoryDigit = 0; _addressDigit = 0; break;
            case SDLK_LEFT:  _cursorX = (--_cursorX < 0) ? HEX_CHARS_X-1 : _cursorX;  _memoryDigit = 0; _addressDigit = 0; break;
            case SDLK_DOWN:  _cursorY = (++_cursorY >= limitY) ? 0       : _cursorY;  _memoryDigit = 0; _addressDigit = 0; break;
            case SDLK_UP:    _cursorY = (--_cursorY < -1) ? limitY-1     : _cursorY;  _memoryDigit = 0; _addressDigit = 0; break;

            case SDLK_PAGEUP:
            {
                if(_editorMode == Load)
                {
                    _fileEntriesIndex--;
                    if(_fileEntriesIndex < 0) _fileEntriesIndex = 0;
                }
                else
                {
                    _hexBaseAddress = (_hexBaseAddress - HEX_CHARS_X*HEX_CHARS_Y) & (RAM_SIZE-1);
                }
            }
            break;

            case SDLK_PAGEDOWN:
            {
                if(_editorMode == Load)
                {
                    if(_fileEntries.size() > HEX_CHARS_Y)
                    {
                        _fileEntriesIndex++;
                        if(_fileEntries.size() - _fileEntriesIndex < HEX_CHARS_Y) _fileEntriesIndex--;
                    }
                }
                else
                {
                    _hexBaseAddress = (_hexBaseAddress + HEX_CHARS_X*HEX_CHARS_Y) & (RAM_SIZE-1);
                }
            }
            break;

            case SDLK_EQUALS: 
            {
                double timingHack = Timing::getTimingHack() - TIMING_HACK*0.05;
                if(timingHack >= 0.0) Timing::setTimingHack(timingHack);
            }
            break;
            case SDLK_MINUS:
            {
                double timingHack = Timing::getTimingHack() + TIMING_HACK*0.05;
                if(timingHack <= TIMING_HACK) Timing::setTimingHack(timingHack);
            }
            break;

            case SDLK_ESCAPE:
            {
                SDL_Quit();
                exit(0);
            }

            // Testing
            case SDLK_F1:
            {
                if(!_singleStepMode) Cpu::reset();
            }
            break;

            //case SDLK_F2: _startMusic = !_startMusic; break;
#if 0
            {
                uint8_t x1 = rand() % GIGA_WIDTH;
                uint8_t x2 = rand() % GIGA_WIDTH;
                uint8_t y1 = rand() % GIGA_HEIGHT;
                uint8_t y2 = rand() % GIGA_HEIGHT;
                uint8_t colour = rand() % 256;
                Graphics::drawLine(x1, y1, x2, y2, colour);
                // Graphics::drawLineGiga(x1, y1, x2, y2, colour);

                //Graphics::life(true);
            }
            break;
#endif

            case SDLK_F3: 
            {
                Audio::nextScore(); break;
                //Graphics::life(false);
            }
            break;

            case SDLK_F4: Loader::saveHighScore(); break;
        }
    }

    void updateEditor(SDL_Keycode keyCode)
    {
        int range = 0;
        if(keyCode >= SDLK_0  &&  keyCode <= SDLK_9) range = 1;
        if(keyCode >= SDLK_a  &&  keyCode <= SDLK_f) range = 2;
        if(range == 1  ||  range == 2)
        {
            uint16_t value = 0;    
            switch(range)
            {
                case 1: value = keyCode - SDLK_0;      break;
                case 2: value = keyCode - SDLK_a + 10; break;
            }

            // Edit address
            if(_cursorY == -1  &&   _hexEdit)
            {
                // Hex address or load address
                if((_cursorX & 0x01) == 0)
                {
                    // Hex address
                    if(_editorMode != Load)
                    {
                        switch(_addressDigit)
                        {
                            case 0: value = (value << 12) & 0xF000; _hexBaseAddress = _hexBaseAddress & 0x0FFF | value; break;
                            case 1: value = (value << 8)  & 0x0F00; _hexBaseAddress = _hexBaseAddress & 0xF0FF | value; break;
                            case 2: value = (value << 4)  & 0x00F0; _hexBaseAddress = _hexBaseAddress & 0xFF0F | value; break;
                            case 3: value = (value << 0)  & 0x000F; _hexBaseAddress = _hexBaseAddress & 0xFFF0 | value; break;
                        }
                    }
                    // Load address
                    else
                    {
                        switch(_addressDigit)
                        {
                            case 0: value = (value << 12) & 0xF000; _loadBaseAddress = _loadBaseAddress & 0x0FFF | value; break;
                            case 1: value = (value << 8)  & 0x0F00; _loadBaseAddress = _loadBaseAddress & 0xF0FF | value; break;
                            case 2: value = (value << 4)  & 0x00F0; _loadBaseAddress = _loadBaseAddress & 0xFF0F | value; break;
                            case 3: value = (value << 0)  & 0x000F; _loadBaseAddress = _loadBaseAddress & 0xFFF0 | value; break;
                        }

                        if(_loadBaseAddress < LOAD_BASE_ADDRESS) _loadBaseAddress = LOAD_BASE_ADDRESS;
                    }
                }
                // Vars address
                else
                {
                    switch(_addressDigit)
                    {
                        case 0: value = (value << 12) & 0xF000; _varsBaseAddress = _varsBaseAddress & 0x0FFF | value; break;
                        case 1: value = (value << 8)  & 0x0F00; _varsBaseAddress = _varsBaseAddress & 0xF0FF | value; break;
                        case 2: value = (value << 4)  & 0x00F0; _varsBaseAddress = _varsBaseAddress & 0xFF0F | value; break;
                        case 3: value = (value << 0)  & 0x000F; _varsBaseAddress = _varsBaseAddress & 0xFFF0 | value; break;
                    }
                }

                _addressDigit = (++_addressDigit) & 0x03;
            }
            // Edit memory
            else if(_memoryMode == RAM  &&  _hexEdit)
            {
                uint16_t address = _hexBaseAddress + _cursorX + _cursorY*HEX_CHARS_X;
                switch(_memoryDigit)
                {
                    case 0: value = (value << 4) & 0x00F0; Cpu::setRAM(address, Cpu::getRAM(address) & 0x000F | value); break;
                    case 1: value = (value << 0) & 0x000F; Cpu::setRAM(address, Cpu::getRAM(address) & 0x00F0 | value); break;
                }
                _memoryDigit = (++_memoryDigit) & 0x01;
            }
        }
    }

    void browseDirectory(void)
    {
        std::string path = _basePath + _filePath;
        _fileEntries.clear();

        DIR *dir;
        struct dirent *ent;
        std::vector<std::string> dirnames;
        std::vector<std::string> filenames;
        if((dir = opendir(path.c_str())) != NULL)
        {
            while((ent = readdir(dir)) != NULL)
            {
                std::string name = std::string(ent->d_name);
                if(ent->d_type == S_IFDIR  &&  name != "."  &&  !(path == _basePath  &&  name == ".."))
                {
                    dirnames.push_back(name);
                }
                else if(ent->d_type == DT_REG  &&  (name.find(".vasm") != std::string::npos  ||  name.find(".gt1") != std::string::npos))
                {
                    filenames.push_back(name);
                }
            }
            closedir (dir);
        }

        std::sort(dirnames.begin(), dirnames.end());
        for(int i=0; i<dirnames.size(); i++)
        {
            FileEntry fileEntry = {Dir, dirnames[i]};
            _fileEntries.push_back(fileEntry);
        }

        std::sort(filenames.begin(), filenames.end());
        for(int i=0; i<filenames.size(); i++)
        {
            FileEntry fileEntry = {File, filenames[i]};
            _fileEntries.push_back(fileEntry);
        }
    }

    void changeBrowseDirectory(void)
    {
        std::string entry = *getFileEntryName(getCursorY());
        setCursorY(0);
        _filePath = (entry == "..") ? "" : "/" + entry;
        browseDirectory();
    }

    void handleKeyUp(SDL_Keycode keyCode)
    {
        switch(keyCode)
        {
            case SDLK_d:      Cpu::setIN(Cpu::getIN() | INPUT_RIGHT);    break;
            case SDLK_a:      Cpu::setIN(Cpu::getIN() | INPUT_LEFT);     break;
            case SDLK_s:      Cpu::setIN(Cpu::getIN() | INPUT_DOWN);     break;
            case SDLK_w:      Cpu::setIN(Cpu::getIN() | INPUT_UP);       break;
            case SDLK_SPACE:  Cpu::setIN(Cpu::getIN() | INPUT_START);    break;
            case SDLK_z:      Cpu::setIN(Cpu::getIN() | INPUT_SELECT);   break;
            case SDLK_SLASH:  Cpu::setIN(Cpu::getIN() | INPUT_B);        break;
            case SDLK_PERIOD: Cpu::setIN(Cpu::getIN() | INPUT_A);        break;
                   
            // Browse vCPU directory
            case SDLK_l:
            {
                if(!_singleStepMode)
                {
                    _cursorX = 0; _cursorY = 0;
                    _editorMode = _editorMode == Load ? Hex : Load;
                    if(_editorMode == Load) browseDirectory();
                }
            }
            break;

            // Execute vCPU code
            case SDLK_F5:
            {
                if(!_singleStepMode)
                {
                    Cpu::setRAM(0x0016, _hexBaseAddress-2 & 0x00FF);
                    Cpu::setRAM(0x0017, (_hexBaseAddress & 0xFF00) >>8);
                    Cpu::setRAM(0x001a, _hexBaseAddress-2 & 0x00FF);
                    Cpu::setRAM(0x001b, (_hexBaseAddress & 0xFF00) >>8);
                }
            }
            break;

            // Enter debug mode
            case SDLK_F6:
            {
                _prevEditorMode = _editorMode;
                _editorMode = Debug;
                _singleStepMode = true;
            }
            break;

            // RAM/ROM mode
            case SDLK_r:
            {
                static int memoryMode = RAM;
                memoryMode = (++memoryMode) % NumMemoryModes;
                _memoryMode = (MemoryMode)memoryMode;
            }
            break;

            // Toggle hex edit or start an upload
            case SDLK_RETURN:
            {
                if(_editorMode != Load  ||  _cursorY == -1)
                {
                    _hexEdit = !_hexEdit;
                }
                else
                {
                    Editor::FileType fileType = Editor::getFileEntryType(Editor::getCursorY());
                    switch(fileType)
                    {
                        case Editor::File: Loader::setStartUploading(true); break;
                        case Editor::Dir: changeBrowseDirectory(); break;
                    }
                }
                break;
            }
        }

        updateEditor(keyCode);
    }

    // Debug mode, handles it's own input and rendering
    bool singleStepDebug(void)
    {
        // Gprintfs
        Assembler::printGprintfStrings();

        // Single step
        if(_singleStep)
        {
            // Timeout on change of variable
            if(SDL_GetTicks() - _singleStepTicks > SINGLE_STEP_STALL_TIME)
            {
                setSingleStep(false); 
                setSingleStepMode(false);
                fprintf(stderr, "Editor::singleStepDebug() : Single step stall for %d milliseconds : exiting debugger...\n", SDL_GetTicks() - _singleStepTicks);
            }
            // Watch variable 
            else if(Cpu::getRAM(_singleStepWatchAddress) != _singleStepWatch) 
            {
                _singleStep = false;
                _singleStepMode = true;
            }
        }

        // Pause simulation and handle debugging keys
        while(_singleStepMode)
        {
            // Update graphics but only once every 16.66667ms
            static uint64_t prevFrameCounter = 0;
            double frameTime = double(SDL_GetPerformanceCounter() - prevFrameCounter) / double(SDL_GetPerformanceFrequency());

            Timing::setFrameUpdate(false);
            if(frameTime > TIMING_HACK)
            {
                Timing::setFrameUpdate(true);
                Graphics::refreshScreen();
                Graphics::render(false);
                prevFrameCounter = SDL_GetPerformanceCounter();
            }

            SDL_Event event;
            while(SDL_PollEvent(&event))
            {
                switch(event.type)
                {
                    case SDL_MOUSEWHEEL: handleMouseWheel(event); break;

                    case SDL_KEYUP:
                    {
                        // Leave debug mode
                        if(event.key.keysym.sym == SDLK_F6)
                        {
                            _singleStep = false;
                            _singleStepMode = false;
                            _editorMode = _prevEditorMode;
                        }
                        else
                        {
                            handleKeyUp(event.key.keysym.sym);
                        }
                    }
                    break;

                    case SDL_KEYDOWN:
                    {
                        // Single step
                        if(event.key.keysym.sym == SDLK_F10)
                        {
                            _singleStep = true;
                            _singleStepMode = false;
                            _singleStepTicks = SDL_GetTicks();
                            _singleStepWatch = Cpu::getRAM(_singleStepWatchAddress);
                        }
                        else
                        {
                            handleKeyDown(event.key.keysym.sym);
                        }
                    }
                    break;
                }
            }
        }

        return _singleStep;
    }

    void handleInput(void)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_MOUSEWHEEL: handleMouseWheel(event); break;
                case SDL_KEYDOWN: handleKeyDown(event.key.keysym.sym); break;
                case SDL_KEYUP: handleKeyUp(event.key.keysym.sym); break;
                case SDL_QUIT: 
                {
                    SDL_Quit();
                    exit(0);
                }
            }
        }
    }
}