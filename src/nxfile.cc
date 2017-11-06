//----------------------------------------------------------------------------------------------------------------------
// NxFile implementation
//----------------------------------------------------------------------------------------------------------------------

#include "nxfile.h"

//----------------------------------------------------------------------------------------------------------------------
// FourCC
//----------------------------------------------------------------------------------------------------------------------

FourCC::FourCC()
    : m_fcc('0000')
{

}

FourCC::FourCC(u32 fcc)
    : m_fcc(fcc)
{

}

