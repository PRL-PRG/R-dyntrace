#include <cstdlib>
#include <cstring>
#include <map>
#include <fstream>

#include "rdt.h"

static std::map<std::string, uint64_t> specialsxp_count;

struct trace_specialsxp {
    static void end() {
        std::ofstream myfile;
        myfile.open ("specialsxp_analysis.txt");
        for (auto &pair : specialsxp_count) {
            Rprintf("%s : %llu\n", pair.first.c_str(), pair.second);
            myfile << pair.first << " : " << pair.second << "\n";
        }
        myfile.close();
    }

    static void specialsxp_entry(const SEXP call, const SEXP op, const SEXP rho) {

        std::string call_name = std::string(get_name(call));

        if (specialsxp_count.find(call_name) == specialsxp_count.end()) {
            specialsxp_count[call_name] = 1;
        } else {
            specialsxp_count[call_name]++;
        }
    }
};


rdt_handler *setup_specialsxp_tracing(SEXP options) {
    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));

    REG_HOOKS_BEGIN(h, trace_specialsxp);
        ADD_HOOK(end);
        ADD_HOOK(specialsxp_entry);
    REG_HOOKS_END;

    return h;
}
