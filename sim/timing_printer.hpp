#pragma once

#include <map>

#include "Inst.hpp"
#include "Utils.hpp"

#define ENTER_OFFSET 0

struct pipelineTimes {
    uint32_t Enter = 0;
    uint32_t IF_stage = 0;
    uint32_t DEC_stage = 0;
    uint32_t RN_stage = 0;
    uint32_t IS_stage = 0;
    uint32_t LD_stage = 0;
    uint32_t EX_stage = 0;
    uint32_t COM_stage = 0;
    bool br_taken = false;
    bool br_mispredict = false;
    uint32_t br_pc_avail = 0;
    bool L1I_miss = false;
    bool L1D_miss = false;
};

class TimingPrinter {
  private:
    std::map<uint32_t, pipelineTimes> m_timeMap;

    int fileIndex = 0;
    int maxFileSize = 0x1000000;
    std::string outDir = "timing_trace";
    std::string uArchName = "SoomRV";
    std::string fileNameBase = uArchName + "_timing";
    std::string filePostfix = ".csv";
    std::ofstream timingFile;

    bool predTaken_prev = false;
    bool br_mispredict = false;
    int br_mispredict_cause = 0;
    uint32_t pc_avail = 0;
    bool L1I_miss_prev = false;

    std::string getFileName(void) {
        std::stringstream fileName;
        fileName << outDir << "/" << fileNameBase << "_" << std::setw(4)
                 << std::setfill('0') << fileIndex << filePostfix;
        return fileName.str();
    }

    bool outFileFull(void) { return timingFile.tellp() > maxFileSize; };

    void write_header() {
        std::stringstream ret_strs;
        ret_strs << "Enter";
        ret_strs << "," << "IF_stage";
        ret_strs << "," << "DEC_stage";
        ret_strs << "," << "RN_stage";
        ret_strs << "," << "IS_stage";
        ret_strs << "," << "LD_stage";
        ret_strs << "," << "EX_stage";
        ret_strs << "," << "COM_stage";
        ret_strs << "," << "br:taken";
        ret_strs << "," << "br:mispredict";
        ret_strs << "," << "br:pc_avail";
        ret_strs << "," << "L1I:miss";
        ret_strs << "," << "L1D:miss";
        ret_strs << std::endl;
        timingFile << ret_strs.str();
    }

    void swapOutFile(void) {
        timingFile.close();
        fileIndex += 1;
        timingFile.open(getFileName());
        write_header();
    }

    void writeLine(Inst &inst) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            std::stringstream ret_strs;
            ret_strs << it->second.Enter;
            ret_strs << "," << it->second.IF_stage;
            ret_strs << "," << it->second.DEC_stage;
            ret_strs << "," << it->second.RN_stage;
            ret_strs << "," << it->second.IS_stage;
            ret_strs << "," << it->second.LD_stage;
            ret_strs << "," << it->second.EX_stage;
            ret_strs << "," << it->second.COM_stage;
            ret_strs << "," << it->second.br_taken;
            ret_strs << "," << it->second.br_mispredict;
            ret_strs << "," << it->second.br_pc_avail;
            ret_strs << "," << it->second.L1I_miss;
            ret_strs << "," << it->second.L1D_miss;
            ret_strs << std::endl;
            timingFile << ret_strs.str();
        }

        if (outFileFull()) {
            swapOutFile();
        }
    }

  public:
    uint64_t init_cycles = UINT64_MAX;

    void init() {
        ensure_output_dir(outDir.c_str());
        timingFile.open(getFileName());
        write_header();
    }

    void close() { timingFile.close(); }

    void LogPredec(Inst &inst, uint64_t curr_cycle) {
        // Filter out initial Cycles
        init_cycles = curr_cycle - ENTER_OFFSET < init_cycles
                          ? curr_cycle - ENTER_OFFSET
                          : init_cycles;

        pipelineTimes temp;
        temp.Enter = curr_cycle - init_cycles - ENTER_OFFSET;
        temp.IF_stage = curr_cycle - init_cycles;

        if (br_mispredict_cause == 1) {
            temp.br_taken = true;
        } else if (br_mispredict_cause == 2) {
            temp.br_taken = false;
        } else {
            temp.br_taken = predTaken_prev;
        }
        predTaken_prev = inst.predTaken;

        temp.br_mispredict = br_mispredict;
        temp.L1I_miss = L1I_miss_prev;
        m_timeMap.insert({inst.id, temp});

        br_mispredict = false;
        br_mispredict_cause = 0;
        L1I_miss_prev = false;
    }

    void LogDecode(Inst &inst, uint64_t curr_cycle) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.DEC_stage = curr_cycle - init_cycles;
        }
    }

    void LogRename(Inst &inst, uint64_t curr_cycle) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.RN_stage = curr_cycle - init_cycles;
        }
    }

    void LogIssue(Inst &inst, uint64_t curr_cycle) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.IS_stage = curr_cycle - init_cycles;
        }
    }

    void LogExec(Inst &inst, uint64_t curr_cycle) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.LD_stage = curr_cycle - init_cycles;
        }
    }

    void LogResult(Inst &inst, uint64_t curr_cycle) {
        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.EX_stage = curr_cycle - init_cycles;
            if ((inst.inst & 0x7f) == OPCODE_LOAD &&
                (it->second.EX_stage - it->second.LD_stage) > 2) {
                it->second.L1D_miss = true;
            }
        }
    }

    void LogFlush(Inst &inst, uint64_t curr_cycle) { 
        // Deletion of "flushed" timing values is not working as expected.
        // (Some of the gathered values are commited)
        //m_timeMap.erase(inst.id);
    }

    void LogCommit(Inst &inst, uint64_t curr_cycle) {

        // std::cout << "Commit at " << curr_cycle << std::endl;

        auto it = m_timeMap.find(inst.id);

        if (it != m_timeMap.end()) {
            it->second.COM_stage = curr_cycle - init_cycles;
            if (it->second.br_taken && !it->second.br_mispredict) {
                it->second.br_pc_avail = pc_avail;
            }
            pc_avail = it->second.IF_stage;

            writeLine(inst);
            m_timeMap.erase(inst.id);
        }
    }

    void captureBrMispredict(int cause) {
        br_mispredict = true;
        br_mispredict_cause = cause;
    }

    void captureL1IMiss() { L1I_miss_prev = true; }
};