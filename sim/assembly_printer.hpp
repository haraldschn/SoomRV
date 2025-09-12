#pragma once

#include "Inst.hpp"
#include "Simif.hpp"
#include "Utils.hpp"

class AssemblyPrinter {
  private:
    int fileIndex = 0;
    int maxFileSize = 0x1000000;
    std::string outDir = "asm_trace";
    std::string fileNameBase = "asm_trace";
    std::string filePostfix = ".csv";
    std::ofstream assemblyFile;

    std::string getFileName(void) {
        std::stringstream fileName;
        fileName << outDir << "/" << fileNameBase << "_" << std::setw(4)
                 << std::setfill('0') << fileIndex << filePostfix;
        return fileName.str();
    }

    bool outFileFull(void) { return assemblyFile.tellp() > maxFileSize; };

    void write_header() {
        std::stringstream ret_strs;
        ret_strs << std::setfill(' ') << std::setw(18) << std::left << "pc"
                 << " ; ";
        ret_strs << std::setfill(' ') << std::setw(50) << std::left
                 << "assembly" << " ; ";
        ret_strs << std::endl;
        assemblyFile << ret_strs.str();
    }

    void swapOutFile(void) {
        assemblyFile.close();
        fileIndex += 1;
        assemblyFile.open(getFileName());
        write_header();
    }

  public:
    void init() {
        ensure_output_dir(outDir.c_str());
        assemblyFile.open(getFileName());
        write_header();
    }

    void write(Inst &inst, SpikeSimif &simif) {

        std::ostringstream oss;
        oss << "0x" << std::right << std::setw(16) << std::setfill('0') << std::hex << static_cast<uint64_t>(inst.pc);
        oss << " ; ";
        oss << std::left << std::setw(50) << std::setfill(' ') << simif.disasm_timing(inst);
        oss << " ; \n";
        assemblyFile << oss.str();
        
        if(outFileFull())
        {
            swapOutFile();
        }
    }

    void close() { assemblyFile.close(); }
};