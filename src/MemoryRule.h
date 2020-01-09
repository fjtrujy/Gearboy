/*
 * Gearboy - Nintendo Game Boy Emulator
 * Copyright (C) 2012  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef MEMORYRULE_H
#define	MEMORYRULE_H

#include "definitions.h"
#include <vector>

class Memory;
class Video;
class Processor;
class Input;
class Cartridge;
class Audio;

class MemoryRule
{
public:
    MemoryRule(Processor* pProcessor, Memory* pMemory, Video* pVideo,
            Input* pInput, Cartridge* pCartridge, Audio* pAudio);
    ~MemoryRule();
    u8 PerformRead(u16 address);
    void PerformWrite(u16 address, u8 value);
    void Reset(bool bCGB);
    void SaveRam(std::ostream &file);
    bool LoadRam(std::istream &file, s32 fileSize);
    void SetRamChangedCallback(RamChangedCallback callback);
    size_t GetRamSize();
    size_t GetRTCSize();
    u8* GetRamBanks();
    u8* GetCurrentRamBank();
    u8* GetRomBank0();
    u8* GetCurrentRomBank1();
    u8* GetRTCMemory();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

protected:
    Processor* m_pProcessor;
    Memory* m_pMemory;
    Video* m_pVideo;
    Input* m_pInput;
    Cartridge* m_pCartridge;
    Audio* m_pAudio;
    bool m_bCGB;
    RamChangedCallback m_pRamChangedCallback;

private:
    int m_iMode;
    int m_iCurrentRAMBank;
    int m_iCurrentROMBank;
    bool m_bRamEnabled;
    u8 m_HigherRomBankBits;
    u8* m_pRAMBanks;
    int m_CurrentROMAddress;
    int m_CurrentRAMAddress;
};

#endif	/* MEMORYRULE_H */
