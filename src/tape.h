//----------------------------------------------------------------------------------------------------------------------
// Emulation of a tape and tape deck
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "types.h"

#include "ui.h"

#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// A tape
// Contains blocks and converts t-state advances into EAR signals
//----------------------------------------------------------------------------------------------------------------------

class Tape
{
public:
    Tape();
    Tape(const vector<u8>& data);

    using Block = vector<u8>;

    enum class BlockType
    {
        Program,
        NumberArray,
        StringArray,
        Bytes,
        Block,
    };

    struct Header
    {
        string      fileName;
        BlockType   type;

        union {
            struct ProgramHeader
            {
                u16     autoStartLine;
                u16     programLength;
                u16     variableOffset;
            } p;
            struct ArrayHeader
            {
                char    variableName;
                u16     arrayLength;
            } a;
            struct BytesHeader
            {
                u16     startAddress;
                u16     dataLength;
            } b;
        };

        u8      checkSum;
    };

    // Return the number of tape blocks
    int numBlocks() const { return (int)m_blocks.size(); }

    // Get the block type
    BlockType getBlockType(int i) const;

    // Return the length of the data block
    u16 getBlockLength(int i) const;

    // Get the header information for a block
    Header getHeader(int i) const;

    //
    // Tape header control
    //

    void selectBlock(int i);
    int getCurrentBlock() const { return m_currentBlock; }

private:
    vector<Block> m_blocks;
    int m_currentBlock;
};

//----------------------------------------------------------------------------------------------------------------------
// TapeWindow
//----------------------------------------------------------------------------------------------------------------------

class TapeWindow final : public Window
{
public:
    TapeWindow(Nx& nx);

    void reset();
    void setTape(Tape *tape)    { m_tape = tape; reset(); m_tape->selectBlock(0); }
    void ejectTape()            { m_tape = nullptr; reset(); }

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    int m_topIndex;
    int m_index;
    Tape* m_tape;
};

//----------------------------------------------------------------------------------------------------------------------
// A tape-browser overlay
// Contains a single tape, and allows controls
//----------------------------------------------------------------------------------------------------------------------

class TapeBrowser final : public Overlay
{
public:
    TapeBrowser(Nx& nx);

    void loadTape(const vector<u8>& data);

protected:
    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

private:
    TapeWindow      m_window;
    vector<string>  m_commands;
    Tape*           m_currentTape;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

