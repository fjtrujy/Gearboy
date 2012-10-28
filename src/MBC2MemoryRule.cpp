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

#include "MBC2MemoryRule.h"
#include "Video.h"
#include "Memory.h"
#include "Processor.h"
#include "Input.h"
#include "Cartridge.h"

MBC2MemoryRule::MBC2MemoryRule(Processor* pProcessor,
        Memory* pMemory, Video* pVideo, Input* pInput,
        Cartridge* pCartridge, Audio* pAudio) : MemoryRule(pProcessor,
pMemory, pVideo, pInput, pCartridge, pAudio)
{
    Reset(false);
}

MBC2MemoryRule::~MBC2MemoryRule()
{
}

u8 MBC2MemoryRule::PerformRead(u16 address)
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
            if (address < 0xA200)
            {
                if (m_bRamEnabled)
                    return m_pMemory->Retrieve(address);
                else
                {
                    Log("--> ** Attempting to read from disabled ram %X", address);
                    return 0xFF;
                }
            }
            else
            {
                Log("--> ** Attempting to read from non usable address %X", address);
                return 0x00;
            }
        }
        default:
        {
            return m_pMemory->Retrieve(address);
        }
    }
}

void MBC2MemoryRule::PerformWrite(u16 address, u8 value)
{
    switch (address & 0xE000)
    {
        case 0x0000:
        {
            if (!(address & 0x0100))
            {
                m_bRamEnabled = (value & 0x0F) == 0x0A;
            }
            break;
        }
        case 0x2000:
        {
            if (address & 0x0100)
            {
                m_iCurrentROMBank = value & 0x0F;
                if (m_iCurrentROMBank == 0)
                    m_iCurrentROMBank = 1;
                m_iCurrentROMBank &= (m_pCartridge->GetROMBankCount() - 1);
                m_CurrentROMAddress = m_iCurrentROMBank * 0x4000;
            }
            break;
        }
        case 0x4000:
        case 0x6000:
        {
            Log("--> ** Attempting to write on non usable address %X %X", address, value);
            break;
        }
        case 0xA000:
        {
            if (address < 0xA200)
            {
                if (m_bRamEnabled)
                {
                    m_pMemory->Load(address, value & 0x0F);
                }
                else
                {
                    Log("--> ** Attempting to write on RAM when ram is disabled %X %X", address, value);
                }
            }
            else
            {
                Log("--> ** Attempting to write on non usable address %X %X", address, value);
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

void MBC2MemoryRule::Reset(bool bCGB)
{
    m_bCGB = bCGB;
    m_iCurrentROMBank = 1;
    m_CurrentROMAddress = 0x4000;
    m_bRamEnabled = false;
}

void MBC2MemoryRule::SaveRam(std::ofstream & file)
{
    Log("MBC2MemoryRule save RAM...");

    for (int i = 0xA000; i < 0xA200; i++)
    {
        u8 ram_byte = 0;
        ram_byte = m_pMemory->Retrieve(i);
        file.write(reinterpret_cast<const char*> (&ram_byte), 1);
    }

    Log("MBC2MemoryRule save RAM done");
}

void MBC2MemoryRule::LoadRam(std::ifstream & file)
{
    Log("MBC2MemoryRule load RAM...");

    for (int i = 0xA000; i < 0xA200; i++)
    {
        u8 ram_byte = 0;
        file.read(reinterpret_cast<char*> (&ram_byte), 1);
        m_pMemory->Load(i, ram_byte);
    }

    Log("MBC2MemoryRule load RAM done");
}

int MBC2MemoryRule::GetRamBanksSize()
{
    return 0x200;
}



