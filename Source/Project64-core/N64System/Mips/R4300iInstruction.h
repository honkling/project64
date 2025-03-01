#pragma once
#include "R4300iOpcode.h"
#include <string>

class R4300iInstruction
{
public:
    R4300iInstruction(uint32_t Address, uint32_t Instruction);

    const char * Name();
    const char * Param();
    std::string NameAndParam();

private:
    R4300iInstruction(void);
    R4300iInstruction(const R4300iInstruction&);
    R4300iInstruction& operator=(const R4300iInstruction&);

    static const char * FPR_Type(uint32_t COP1OpCode);
        
    void DecodeName(void);
    void DecodeSpecialName(void);
    void DecodeRegImmName(void);
    void DecodeCop1Name(void);
    
    uint32_t m_Address;
    R4300iOpcode m_Instruction;
    char m_Name[40];
    char m_Param[200];
};