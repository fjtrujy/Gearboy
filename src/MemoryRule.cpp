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

#include "MemoryRule.h"
#include "Video.h"
#include "Memory.h"
#include "Processor.h"
#include "Input.h"
#include "Cartridge.h"

const int kMBC1RamBanksSize = 0x8000;

MemoryRule::MemoryRule(Processor* pProcessor, Memory* pMemory,
        Video* pVideo, Input* pInput, Cartridge* pCartridge, Audio* pAudio)
{
    m_pProcessor = pProcessor;
    m_pMemory = pMemory;
    m_pVideo = pVideo;
    m_pInput = pInput;
    m_pCartridge = pCartridge;
    m_pAudio = pAudio;
    m_bCGB = false;
    InitPointer(m_pRamChangedCallback);
    m_pRAMBanks = new u8[kMBC1RamBanksSize];
    Reset(false);
}

MemoryRule::~MemoryRule()
{
    SafeDeleteArray(m_pRAMBanks);
}

void MemoryRule::SaveRam(std::ostream &file)
{
    Log("MemoryRule save RAM...");
    Log("MemoryRule saving %d banks...", m_pCartridge->GetRAMBankCount());

    u32 ramSize = m_pCartridge->GetRAMBankCount() * 0x2000;

    for (u32 i = 0; i < ramSize; i++)
    {
        u8 ram_byte = m_pRAMBanks[i];
        file.write(reinterpret_cast<const char*> (&ram_byte), 1);
    }

    Log("MemoryRule save RAM done");
}

bool MemoryRule::LoadRam(std::istream &file, s32 fileSize)
{
    Log("MemoryRule load RAM...");
    Log("MemoryRule loading %d banks...", m_pCartridge->GetRAMBankCount());

    s32 ramSize = m_pCartridge->GetRAMBankCount() * 0x2000;

    if ((fileSize > 0) && (fileSize != ramSize))
    {
        Log("MemoryRule incorrect size. Expected: %d Found: %d", ramSize, fileSize);
        return false;
    }
    else if (fileSize == 0)
    {
        // compatibility with old saves
        u8 mode;
        file.read(reinterpret_cast<char*> (&mode), 1);
        Log("MemoryRule load RAM mode %d", mode);
    }

    for (s32 i = 0; i < ramSize; i++)
    {
        u8 ram_byte = 0;
        file.read(reinterpret_cast<char*> (&ram_byte), 1);
        m_pRAMBanks[i] = ram_byte;
    }

    Log("MemoryRule load RAM done");

    return true;
}

void MemoryRule::SetRamChangedCallback(RamChangedCallback callback)
{
    m_pRamChangedCallback = callback;
}

size_t MemoryRule::GetRamSize()
{
    return m_pCartridge->GetRAMBankCount() * 0x2000;
}

size_t MemoryRule::GetRTCSize()
{
    Log("MemoryRule::GetRTCSize not implemented");
    return 0;
}

u8* MemoryRule::GetRamBanks()
{
    return m_pMemory->GetMemoryMap() + 0xA000;
}

u8* MemoryRule::GetCurrentRamBank()
{
    if (m_pCartridge->GetRAMSize() > 0)
        return &m_pRAMBanks[m_CurrentRAMAddress];
    else
        return NULL;
}

u8* MemoryRule::GetRomBank0()
{
    return m_pMemory->GetMemoryMap() + 0x0000;
}

u8* MemoryRule::GetCurrentRomBank1()
{
    u8* pROM = m_pCartridge->GetTheROM();
    return &pROM[m_CurrentROMAddress];
}

u8* MemoryRule::GetRTCMemory()
{
    Log("MemoryRule::GetRTCMemory not implemented");
    return NULL;
}

void MemoryRule::SaveState(std::ostream& stream)
{
    using namespace std;

    stream.write(reinterpret_cast<const char*> (&m_iMode), sizeof(m_iMode));
    stream.write(reinterpret_cast<const char*> (&m_iCurrentRAMBank), sizeof(m_iCurrentRAMBank));
    stream.write(reinterpret_cast<const char*> (&m_iCurrentROMBank), sizeof(m_iCurrentROMBank));
    stream.write(reinterpret_cast<const char*> (&m_bRamEnabled), sizeof(m_bRamEnabled));
    stream.write(reinterpret_cast<const char*> (&m_HigherRomBankBits), sizeof(m_HigherRomBankBits));
    stream.write(reinterpret_cast<const char*> (m_pRAMBanks), kMBC1RamBanksSize);
    stream.write(reinterpret_cast<const char*> (&m_CurrentROMAddress), sizeof(m_CurrentROMAddress));
    stream.write(reinterpret_cast<const char*> (&m_CurrentRAMAddress), sizeof(m_CurrentRAMAddress));
}

void MemoryRule::Reset(bool bCGB)
{
    m_bCGB = bCGB;
    m_iMode = 0;
    m_iCurrentRAMBank = 0;
    m_iCurrentROMBank = 1;
    m_HigherRomBankBits = 0;
    m_bRamEnabled = false;
    for (int i = 0; i < kMBC1RamBanksSize; i++)
        m_pRAMBanks[i] = 0xFF;
    m_CurrentROMAddress = 0x4000;
    m_CurrentRAMAddress = 0;
}

u8 MemoryRule::PerformRead(u16 address)
{
    switch (address & 0xE000)
    {
        case 0x4000:
        case 0x6000:
        {
            u8* pROM = m_pCartridge->GetTheROM();
            return pROM[(address - 0x4000) + m_CurrentROMAddress];
        }
        case 0xA000:
        {
            if (m_bRamEnabled)
            {
                if (m_iMode == 0)
                {
                    if ((m_pCartridge->GetRAMSize() == 1) && (address >= 0xA800))
                    {
                        // only 2KB of ram
                        Log("--> ** Attempting to read from invalid RAM %X", address);
                    }
                    return m_pRAMBanks[address - 0xA000];
                }
                else
                    return m_pRAMBanks[(address - 0xA000) + m_CurrentRAMAddress];
            }
            else
            {
                Log("--> ** Attempting to read from disabled ram %X", address);
                return 0xFF;
            }
        }
        default:
        {
            return m_pMemory->Retrieve(address);
        }
    }
}

void MemoryRule::PerformWrite(u16 address, u8 value)
{
    switch (address & 0xE000)
    {
        case 0x0000:
        {
            if (m_pCartridge->GetRAMSize() > 0)
            {
                bool previous = m_bRamEnabled;
                m_bRamEnabled = ((value & 0x0F) == 0x0A);

                if (IsValidPointer(m_pRamChangedCallback) && previous && !m_bRamEnabled)
                {
                    (*m_pRamChangedCallback)();
                }
            }
            break;
        }
        case 0x2000:
        {
            if (m_iMode == 0)
            {
                m_iCurrentROMBank = (value & 0x1F) | (m_HigherRomBankBits << 5);
            }
            else
            {
                m_iCurrentROMBank = value & 0x1F;
            }

            if (m_iCurrentROMBank == 0x00 || m_iCurrentROMBank == 0x20
                    || m_iCurrentROMBank == 0x40 || m_iCurrentROMBank == 0x60)
                m_iCurrentROMBank++;

            m_iCurrentROMBank &= (m_pCartridge->GetROMBankCount() - 1);
            m_CurrentROMAddress = m_iCurrentROMBank * 0x4000;
            break;
        }
        case 0x4000:
        {
            if (m_iMode == 1)
            {
                m_iCurrentRAMBank = value & 0x03;
                m_iCurrentRAMBank &= (m_pCartridge->GetRAMBankCount() - 1);
                m_CurrentRAMAddress = m_iCurrentRAMBank * 0x2000;
            }
            else
            {
                m_HigherRomBankBits = value & 0x03;
                m_iCurrentROMBank = (m_iCurrentROMBank & 0x1F) | (m_HigherRomBankBits << 5);

                if (m_iCurrentROMBank == 0x00 || m_iCurrentROMBank == 0x20
                        || m_iCurrentROMBank == 0x40 || m_iCurrentROMBank == 0x60)
                    m_iCurrentROMBank++;

                m_iCurrentROMBank &= (m_pCartridge->GetROMBankCount() - 1);
                m_CurrentROMAddress = m_iCurrentROMBank * 0x4000;
            }
            break;
        }
        case 0x6000:
        {
            if ((m_pCartridge->GetRAMSize() != 3) && (value & 0x01))
            {
                Log("--> ** Attempting to change MBC1 to mode 1 with incorrect RAM banks %X %X", address, value);
            }
            else
            {
                m_iMode = value & 0x01;
            }
            break;
        }
        case 0xA000:
        {
            if (m_bRamEnabled)
            {
                if (m_iMode == 0)
                {
                    if ((m_pCartridge->GetRAMSize() == 1) && (address >= 0xA800))
                    {
                        // only 2KB of ram
                        Log("--> ** Attempting to write on invalid RAM %X %X", address, value);
                    }

                    m_pRAMBanks[address - 0xA000] = value;
                }
                else
                    m_pRAMBanks[(address - 0xA000) + m_CurrentRAMAddress] = value;
            }
            else
            {
                Log("--> ** Attempting to write on RAM when ram is disabled %X %X", address, value);
            }
            break;
        }
        default:
        {
            m_pMemory->Load(address, value);
            break;
        }
    }
}

void MemoryRule::LoadState(std::istream& stream)
{
    using namespace std;

    stream.read(reinterpret_cast<char*> (&m_iMode), sizeof(m_iMode));
    stream.read(reinterpret_cast<char*> (&m_iCurrentRAMBank), sizeof(m_iCurrentRAMBank));
    stream.read(reinterpret_cast<char*> (&m_iCurrentROMBank), sizeof(m_iCurrentROMBank));
    stream.read(reinterpret_cast<char*> (&m_bRamEnabled), sizeof(m_bRamEnabled));
    stream.read(reinterpret_cast<char*> (&m_HigherRomBankBits), sizeof(m_HigherRomBankBits));
    stream.read(reinterpret_cast<char*> (m_pRAMBanks), kMBC1RamBanksSize);
    stream.read(reinterpret_cast<char*> (&m_CurrentROMAddress), sizeof(m_CurrentROMAddress));
    stream.read(reinterpret_cast<char*> (&m_CurrentRAMAddress), sizeof(m_CurrentRAMAddress));
}
