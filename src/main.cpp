#include <string>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <getopt.h>
#include <thread>
#include <time.h>
#include <algorithm>
#include <limits>

#include <cstdio>

#include <zlib.h>

#include "common.h"


//#define ERROR_STR "\033[1mError:\033[0m"
#define ERROR_STR "Error:"

using namespace std;


int my_mkdir(const char *path, mode_t mode) {
  #ifdef _WIN64
  return mkdir(path);
  #else
  return mkdir(path,mode);
  #endif
}

bool checkFileExists(std::string fn) {
  struct stat stFileInfo;
  auto intStat = stat(fn.c_str(), &stFileInfo);
  return intStat == 0;
}


void PrintCite() {
  cout << "[no citation info to display yet]" 
       << endl;
}

void PrintVersion() {
  cout << "splitcode, version " << 	SPLITCODE_VERSION << endl;
}

void usage() {
  cout << "splitcode " << SPLITCODE_VERSION << endl << endl
       << "Usage: splitcode [arguments] fastq-files" << endl << endl
       << "Options:" << endl
       << "-t, --threads    Number of threads to use" << endl
       << "-h, --help       Displays usage information" << endl
       << "    --version    Prints version number" << endl
       << "    --cite       Prints citation information" << endl;
}

void ParseOptions(int argc, char **argv, ProgramOptions& opt) {
  int help_flag = 0;
  int version_flag = 0;
  int cite_flag = 0;

  const char *opt_string = "t:h";
  static struct option long_options[] = {
    // long args
    {"version", no_argument, &version_flag, 1},
    {"cite", no_argument, &cite_flag, 1},
    // short args
    {"help", no_argument, 0, 'h'},
    {"threads", required_argument, 0, 't'},
    {0,0,0,0}
  };
  
  int c;
  int option_index = 0;
  int num_opts_supplied = 0;
  while (true) {
    c = getopt_long(argc,argv,opt_string, long_options, &option_index);

    if (c == -1) {
      break;
    }
    
    num_opts_supplied++;
    
    switch (c) {
    case 0:
      break;
    case 'h': {
      help_flag = 1;
      break;
    }
    case 't': {
      stringstream(optarg) >> opt.threads;
      break;
    }
    default: break;
    }
  }
  
  if (help_flag || num_opts_supplied == 0) {
    usage();
    exit(0);
  }
  if (version_flag) {
    PrintVersion();
    exit(0);
  }
  if (cite_flag) {
    PrintCite();
    exit(0);
  }
}



int main(int argc, char *argv[]) {
  std::cout.sync_with_stdio(false);
  setvbuf(stdout, NULL, _IOFBF, 1048576);
  ProgramOptions opt;
  ParseOptions(argc,argv,opt);
  fflush(stdout);

  return 0;
}