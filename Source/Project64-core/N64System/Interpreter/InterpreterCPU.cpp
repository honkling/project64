#include "stdafx.h"
#include "InterpreterCPU.h"

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps32.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/Plugins/Plugin.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/Debugger.h>

R4300iOp::Func * CInterpreterCPU::m_R4300i_Opcode = nullptr;

void ExecuteInterpreterOps(uint32_t /*Cycles*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2)
{
    R4300iOpcode Command;

    if (!g_MMU->MemoryValue32(PC + 4, Command.Value))
    {
        //g_Notify->DisplayError("Failed to load word 2");
        //ExitThread(0);
        return true;
    }

    switch (Command.op)
    {
    case R4300i_SPECIAL:
        switch (Command.funct)
        {
        case R4300i_SPECIAL_SLL:
        case R4300i_SPECIAL_SRL:
        case R4300i_SPECIAL_SRA:
        case R4300i_SPECIAL_SLLV:
        case R4300i_SPECIAL_SRLV:
        case R4300i_SPECIAL_SRAV:
        case R4300i_SPECIAL_MFHI:
        case R4300i_SPECIAL_MTHI:
        case R4300i_SPECIAL_MFLO:
        case R4300i_SPECIAL_MTLO:
        case R4300i_SPECIAL_DSLLV:
        case R4300i_SPECIAL_DSRLV:
        case R4300i_SPECIAL_DSRAV:
        case R4300i_SPECIAL_ADD:
        case R4300i_SPECIAL_ADDU:
        case R4300i_SPECIAL_SUB:
        case R4300i_SPECIAL_SUBU:
        case R4300i_SPECIAL_AND:
        case R4300i_SPECIAL_OR:
        case R4300i_SPECIAL_XOR:
        case R4300i_SPECIAL_NOR:
        case R4300i_SPECIAL_SLT:
        case R4300i_SPECIAL_SLTU:
        case R4300i_SPECIAL_DADD:
        case R4300i_SPECIAL_DADDU:
        case R4300i_SPECIAL_DSUB:
        case R4300i_SPECIAL_DSUBU:
        case R4300i_SPECIAL_DSLL:
        case R4300i_SPECIAL_DSRL:
        case R4300i_SPECIAL_DSRA:
        case R4300i_SPECIAL_DSLL32:
        case R4300i_SPECIAL_DSRL32:
        case R4300i_SPECIAL_DSRA32:
            if (Command.rd == 0)
            {
                return false;
            }
            if (Command.rd == Reg1 || Command.rd == Reg2)
            {
                return true;
            }
            break;
        case R4300i_SPECIAL_MULT:
        case R4300i_SPECIAL_MULTU:
        case R4300i_SPECIAL_DIV:
        case R4300i_SPECIAL_DIVU:
        case R4300i_SPECIAL_DMULT:
        case R4300i_SPECIAL_DMULTU:
        case R4300i_SPECIAL_DDIV:
        case R4300i_SPECIAL_DDIVU:
            break;
        default:
            if (CDebugSettings::HaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Does %s effect delay slot at %X?", R4300iInstruction(PC + 4, Command.Value).Name(), PC).c_str());
            }
            return true;
        }
        break;
    case R4300i_CP0:
        switch (Command.rs)
        {
        case R4300i_COP0_MT: break;
        case R4300i_COP0_MF:
            if (Command.rt == 0)
            {
                return false;
            }
            if (Command.rt == Reg1 || Command.rt == Reg2)
            {
                return true;
            }
            break;
        default:
            if ((Command.rs & 0x10) != 0)
            {
                switch (Command.funct)
                {
                case R4300i_COP0_CO_TLBR: break;
                case R4300i_COP0_CO_TLBWI: break;
                case R4300i_COP0_CO_TLBWR: break;
                case R4300i_COP0_CO_TLBP: break;
                default:
                    if (CDebugSettings::HaveDebugger())
                    {
                        g_Notify->DisplayError(stdstr_f("Does %s effect delay slot at %X?\n6", R4300iInstruction(PC + 4, Command.Value).Name(), PC).c_str());
                    }
                    return true;
                }
            }
            else
            {
                if (CDebugSettings::HaveDebugger())
                {
                    g_Notify->DisplayError(stdstr_f("Does %s effect delay slot at %X?\n7", R4300iInstruction(PC + 4, Command.Value).Name(), PC).c_str());
                }
                return true;
            }
        }
        break;
    case R4300i_CP1:
        switch (Command.fmt)
        {
        case R4300i_COP1_MF:
            if (Command.rt == 0)
            {
                return false;
            }
            if (Command.rt == Reg1 || Command.rt == Reg2)
            {
                return true;
            }
            break;
        case R4300i_COP1_CF: break;
        case R4300i_COP1_MT: break;
        case R4300i_COP1_CT: break;
        case R4300i_COP1_S: break;
        case R4300i_COP1_D: break;
        case R4300i_COP1_W: break;
        case R4300i_COP1_L: break;
        default:
            if (CDebugSettings::HaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Does %s effect delay slot at %X?", R4300iInstruction(PC + 4, Command.Value).Name(), PC).c_str());
            }
            return true;
        }
        break;
    case R4300i_ANDI:
    case R4300i_ORI:
    case R4300i_XORI:
    case R4300i_LUI:
    case R4300i_ADDI:
    case R4300i_ADDIU:
    case R4300i_SLTI:
    case R4300i_SLTIU:
    case R4300i_DADDI:
    case R4300i_DADDIU:
    case R4300i_LB:
    case R4300i_LH:
    case R4300i_LW:
    case R4300i_LWL:
    case R4300i_LWR:
    case R4300i_LDL:
    case R4300i_LDR:
    case R4300i_LBU:
    case R4300i_LHU:
    case R4300i_LD:
    case R4300i_LWC1:
    case R4300i_LDC1:
        if (Command.rt == 0)
        {
            return false;
        }
        if (Command.rt == Reg1 || Command.rt == Reg2)
        {
            return true;
        }
        break;
    case R4300i_CACHE: break;
    case R4300i_SB: break;
    case R4300i_SH: break;
    case R4300i_SW: break;
    case R4300i_SWR: break;
    case R4300i_SWL: break;
    case R4300i_SWC1: break;
    case R4300i_SDC1: break;
    case R4300i_SD: break;
    default:
        if (CDebugSettings::HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("Does %s effect delay slot at %X?", R4300iInstruction(PC + 4, Command.Value).Name(), PC).c_str());
        }
        return true;
    }
    return false;
}

void CInterpreterCPU::BuildCPU()
{
    R4300iOp::m_TestTimer = false;

    if (g_Settings->LoadBool(Game_32Bit))
    {
        m_R4300i_Opcode = R4300iOp32::BuildInterpreter();
    }
    else
    {
        m_R4300i_Opcode = R4300iOp::BuildInterpreter();
    }
}

void CInterpreterCPU::InPermLoop()
{
    // Interrupts enabled
    if ((g_Reg->STATUS_REGISTER & STATUS_IE) == 0 ||
        (g_Reg->STATUS_REGISTER & STATUS_EXL) != 0 ||
        (g_Reg->STATUS_REGISTER & STATUS_ERL) != 0 ||
        (g_Reg->STATUS_REGISTER & 0xFF00) == 0)
    {
        if (g_Plugins->Gfx()->UpdateScreen != nullptr)
        {
            g_Plugins->Gfx()->UpdateScreen();
        }
        //CurrentFrame = 0;
        //CurrentPercent = 0;
        //DisplayFPS();
        g_Notify->DisplayError(GS(MSG_PERM_LOOP));
        g_System->CloseCpu();
    }
    else
    {
        if (*g_NextTimer > 0)
        {
            g_SystemTimer->UpdateTimers();
            *g_NextTimer = 0 - g_System->CountPerOp();
            g_SystemTimer->UpdateTimers();
        }
    }
}

void CInterpreterCPU::ExecuteCPU()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    bool & Done = g_System->m_EndEmulation;
    PIPELINE_STAGE & PipelineStage = g_System->m_PipelineStage;
    uint32_t & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    R4300iOpcode & Opcode = R4300iOp::m_Opcode;
    uint32_t & JumpToLocation = g_System->m_JumpToLocation;
    bool & TestTimer = R4300iOp::m_TestTimer;
    const int32_t & bDoSomething = g_SystemEvents->DoSomething();
    uint32_t CountPerOp = g_System->CountPerOp();
    int32_t & NextTimer = *g_NextTimer;
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (!g_MMU->MemoryValue32(PROGRAM_COUNTER, Opcode.Value))
            {
                g_Reg->DoTLBReadMiss(PipelineStage == PIPELINE_STAGE_JUMP, PROGRAM_COUNTER);
                PipelineStage = PIPELINE_STAGE_NORMAL;
                continue;
            }

            if (HaveDebugger())
            {
                if (HaveExecutionBP() && g_Debugger->ExecutionBP(PROGRAM_COUNTER))
                {
                    g_Settings->SaveBool(Debugger_SteppingOps, true);
                }

                g_Debugger->CPUStepStarted(); // May set stepping ops/skip op

                if (isStepping())
                {
                    g_Debugger->WaitForStep();
                }

                if (SkipOp())
                {
                    // Skip command if instructed by the debugger
                    g_Settings->SaveBool(Debugger_SkipOp, false);
                    PROGRAM_COUNTER += 4;
                    continue;
                }

                g_Debugger->CPUStep();
            }

            /* if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER < 0x80380000)
            {
            WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
            } */

            m_R4300i_Opcode[Opcode.op]();
            _GPR[0].DW = 0; // MIPS $zero hard-wired to 0
            NextTimer -= CountPerOp;

            if (CDebugSettings::HaveDebugger()) { g_Debugger->CPUStepEnded(); }

            PROGRAM_COUNTER += 4;
            switch (PipelineStage)
            {
            case PIPELINE_STAGE_NORMAL:
                break;
            case PIPELINE_STAGE_DELAY_SLOT:
                PipelineStage = PIPELINE_STAGE_JUMP;
                break;
            case  PIPELINE_STAGE_PERMLOOP_DO_DELAY:
                PipelineStage = PIPELINE_STAGE_PERMLOOP_DELAY_DONE;
                break;
            case  PIPELINE_STAGE_JUMP:
                CheckTimer = (JumpToLocation < PROGRAM_COUNTER - 4 || TestTimer);
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                if (CheckTimer)
                {
                    TestTimer = false;
                    if (NextTimer < 0)
                    {
                        g_SystemTimer->TimerDone();
                    }
                    if (bDoSomething)
                    {
                        g_SystemEvents->ExecuteEvents();
                    }
                }
                break;
            case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                CInterpreterCPU::InPermLoop();
                g_SystemTimer->TimerDone();
                if (bDoSomething)
                {
                    g_SystemEvents->ExecuteEvents();
                }
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CInterpreterCPU::ExecuteOps(int32_t Cycles)
{
    bool & Done = g_System->m_EndEmulation;
    uint32_t  & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    R4300iOpcode & Opcode = R4300iOp::m_Opcode;
    PIPELINE_STAGE & PipelineStage = g_System->m_PipelineStage;
    uint32_t & JumpToLocation = g_System->m_JumpToLocation;
    bool   & TestTimer = R4300iOp::m_TestTimer;
    const int32_t & DoSomething = g_SystemEvents->DoSomething();
    uint32_t CountPerOp = g_System->CountPerOp();
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (Cycles <= 0)
            {
                g_SystemTimer->UpdateTimers();
                return;
            }

            if (g_MMU->MemoryValue32(PROGRAM_COUNTER, Opcode.Value))
            {
                /*if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                /*if (PROGRAM_COUNTER > 0x80323000 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                m_R4300i_Opcode[Opcode.op]();
                _GPR[0].DW = 0; /* MIPS $zero hard-wired to 0 */

                Cycles -= CountPerOp;
                *g_NextTimer -= CountPerOp;

                /*static uint32_t TestAddress = 0x80077B0C, TestValue = 0, CurrentValue = 0;
                if (g_MMU->MemoryValue32(TestAddress, TestValue))
                {
                if (TestValue != CurrentValue)
                {
                WriteTraceF(TraceError,"%X: %X changed (%s)",PROGRAM_COUNTER,TestAddress,R4300iInstruction(PROGRAM_COUNTER, m_Opcode.Value).NameAndParam().c_str());
                CurrentValue = TestValue;
                }
                }*/

                switch (PipelineStage)
                {
                case PIPELINE_STAGE_NORMAL:
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_DELAY_SLOT:
                    PipelineStage = PIPELINE_STAGE_JUMP;
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_PERMLOOP_DO_DELAY:
                    PipelineStage = PIPELINE_STAGE_PERMLOOP_DELAY_DONE;
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_JUMP:
                    CheckTimer = (JumpToLocation < PROGRAM_COUNTER || TestTimer);
                    PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
                    if (CheckTimer)
                    {
                        TestTimer = false;
                        if (*g_NextTimer < 0)
                        {
                            g_SystemTimer->TimerDone();
                        }
                        if (DoSomething)
                        {
                            g_SystemEvents->ExecuteEvents();
                        }
                    }
                    break;
                case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                    PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
                    CInterpreterCPU::InPermLoop();
                    g_SystemTimer->TimerDone();
                    if (DoSomething)
                    {
                        g_SystemEvents->ExecuteEvents();
                    }
                    break;
                default:
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            else
            {
                g_Reg->DoTLBReadMiss(PipelineStage == PIPELINE_STAGE_JUMP, PROGRAM_COUNTER);
                PipelineStage = PIPELINE_STAGE_NORMAL;
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}