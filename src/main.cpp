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
#include "ProcessReads.h"
#include "SplitCode.h"


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
       << "Options (for configuring on the command-line):" << endl
       << "-b, --barcodes   List of barcode sequences (comma-separated)" << endl
       << "-d, --distances  List of error distance (mismatch:indel:total) thresholds (comma-separated)" << endl
       << "-l, --locations  List of locations (file:pos1:pos2) (comma-separated)" << endl
       << "-i, --ids        List of barcode names/identifiers (comma-separated)" << endl
       << "-f, --minFinds   List of minimum times a barcode must be found in a read (comma-separated)" << endl
       << "-F, --maxFinds   List of maximum times a barcode can be found in a read (comma-separated)" << endl
       << "-e, --exclude    List of what to exclude from final barcode (comma-separated; 1 = exclude, 0 = include)" << endl
       << "-L, --left       List of what barcodes to include when trimming from the left (comma-separated; 1 = include, 0 = exclude)" << endl
       << "-R, --right      List of what barcodes to include when trimming from the right (comma-separated; 1 = include, 0 = exclude)" << endl
       << "                 (Note: for --left/--right, can specify an included barcode as 1:x where x = number of extra bp's to trim" << endl
       << "                 from left/right side if the that included barcode is at the leftmost/rightmost position)" << endl
       << "Options (configurations supplied in a file):" << endl
       << "-c, --config     Configuration file" << endl
       << "Output Options:" << endl
       << "-m, --mapping    Output file where the mapping between final barcode sequences and names will be written" << endl
       << "-o, --output     FASTQ file(s) where output will be written (comma-separated)" << endl
       << "                 Number of output FASTQ files should equal --nFastqs" << endl
       << "-O, --outb       FASTQ file where final barcodes will be written" << endl
       << "                 If not supplied, the final barcodes will be prepended to reads of first FASTQ file" << endl
       << "-u, --unassigned FASTQ file(s) where output of unassigned reads will be written (comma-separated)" << endl
       << "                 Number of FASTQ files should equal --nFastqs" << endl
       << "-E, --empty      Sequence to fill in empty reads in output FASTQ files (default: no sequence is used to fill in those reads)" << endl
       << "-p, --pipe       Write to standard output (instead of output FASTQ files)" << endl
       << "    --gzip       Output compressed gzip'ed FASTQ files" << endl
       << "    --no-output  Don't output any sequences (output statistics only)" << endl
       << "    --mod-names  Modify names of outputted sequences to include identified barcodes" << endl
       << "Other Options:" << endl
       << "-N, --nFastqs    Number of FASTQ file(s) per run" << endl
       << "                 (default: 1) (specify 2 for paired-end)" << endl
       << "-A, --append     An existing mapping file that will be added on to" << endl
       << "-k, --keep       File containing a list of final barcodes to keep" << endl
       << "-r, --remove     File containing a list of final barcodes to remove/discard" << endl
       << "-t, --threads    Number of threads to use" << endl
       << "-T, --trim-only  All reads are assigned and trimmed regardless of barcode identification" << endl
       << "-h, --help       Displays usage information" << endl
       << "    --version    Prints version number" << endl
       << "    --cite       Prints citation information" << endl;
}

void ParseOptions(int argc, char **argv, ProgramOptions& opt) {
  int help_flag = 0;
  int version_flag = 0;
  int cite_flag = 0;
  int no_output_flag = 0;
  int gzip_flag = 0;
  int mod_names_flag = 0;

  const char *opt_string = "t:N:b:d:i:l:f:F:e:c:o:O:u:m:k:r:A:L:R:E:Tph";
  static struct option long_options[] = {
    // long args
    {"version", no_argument, &version_flag, 1},
    {"cite", no_argument, &cite_flag, 1},
    {"no-output", no_argument, &no_output_flag, 1},
    {"gzip", no_argument, &gzip_flag, 1},
    {"mod-names", no_argument, &mod_names_flag, 1},
    // short args
    {"help", no_argument, 0, 'h'},
    {"pipe", no_argument, 0, 'p'},
    {"trim-only", no_argument, 0, 'T'},
    {"threads", required_argument, 0, 't'},
    {"nFastqs", required_argument, 0, 'N'},
    {"barcodes", required_argument, 0, 'b'},
    {"distances", required_argument, 0, 'd'},
    {"locations", required_argument, 0, 'l'},
    {"ids", required_argument, 0, 'i'},
    {"maxFinds", required_argument, 0, 'F'},
    {"minFinds", required_argument, 0, 'f'},
    {"exclude", required_argument, 0, 'e'},
    {"config", required_argument, 0, 'c'},
    {"output", required_argument, 0, 'o'},
    {"outb", required_argument, 0, 'O'},
    {"unassigned", required_argument, 0, 'u'},
    {"mapping", required_argument, 0, 'm'},
    {"keep", required_argument, 0, 'k'},
    {"remove", required_argument, 0, 'r'},
    {"append", required_argument, 0, 'A'},
    {"left", required_argument, 0, 'L'},
    {"right", required_argument, 0, 'R'},
    {"empty", required_argument, 0, 'E'},
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
    case 'p': {
      opt.pipe = true;
      break;
    }
    case 'T': {
      opt.trim_only = true;
      break;
    }
    case 't': {
      stringstream(optarg) >> opt.threads;
      break;
    }
    case 'N': {
      stringstream(optarg) >> opt.nfiles;
      break;
    }
    case 'b': {
      stringstream(optarg) >> opt.barcode_str;
      break;
    }
    case 'd': {
      stringstream(optarg) >> opt.distance_str;
      break;
    }
    case 'l': {
      stringstream(optarg) >> opt.location_str;
      break;
    }
    case 'i': {
      stringstream(optarg) >> opt.barcode_identifiers_str;
      break;
    }
    case 'F': {
      stringstream(optarg) >> opt.max_finds_str;
      break;
    }
    case 'f': {
      stringstream(optarg) >> opt.min_finds_str;
      break;
    }
    case 'e': {
      stringstream(optarg) >> opt.exclude_str;
      break;
    }
    case 'L': {
      stringstream(optarg) >> opt.left_str;
      break;
    }
    case 'R': {
      stringstream(optarg) >> opt.right_str;
      break;
    }
    case 'c': {
      stringstream(optarg) >> opt.config_file;
      break;
    }
    case 'm': {
      stringstream(optarg) >> opt.mapping_file;
      break;
    }
    case 'k': {
      stringstream(optarg) >> opt.keep_file;
      break;
    }
    case 'r': {
      opt.discard = true;
      stringstream(optarg) >> opt.keep_file;
      break;
    }
    case 'o': {
      std::string files;
      stringstream(optarg) >> files;
      std::stringstream ss(files);
      std::string filename;
      while (std::getline(ss, filename, ',')) { 
        opt.output_files.push_back(filename);
      }
      break;
    }
    case 'O': {
      stringstream(optarg) >> opt.outputb_file;
      break;
    }
    case 'A': {
      stringstream(optarg) >> opt.append_file;
      break;
    }
    case 'E': {
      stringstream(optarg) >> opt.empty_read_sequence;
      for (auto& c: opt.empty_read_sequence) {
        c = toupper(c);
      }
      break;
    }
    case 'u': {
      std::string files;
      stringstream(optarg) >> files;
      std::stringstream ss(files);
      std::string filename;
      while (std::getline(ss, filename, ',')) { 
        opt.unassigned_files.push_back(filename);
      }
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
  if (no_output_flag) {
    opt.no_output = true;
  }
  if (mod_names_flag) {
    opt.mod_names = true;
  }
  if (gzip_flag) {
    opt.gzip = true;
  }
  
  for (int i = optind; i < argc; i++) {
    opt.files.push_back(argv[i]);
  }
}

bool CheckOptions(ProgramOptions& opt, SplitCode& sc) {
  bool ret = true;
  if (opt.threads <= 0) {
    cerr << "Error: invalid number of threads " << opt.threads << endl;
    ret = false;
  } else {
    unsigned int n = std::thread::hardware_concurrency();
    if (n != 0 && n < opt.threads) {
      cerr << "Warning: you asked for " << opt.threads
           << ", but only " << n << " cores on the machine" << endl;
    }    
  }
  if (opt.files.size() == 0) {
    cerr << ERROR_STR << " Missing read files" << endl;
    ret = false;
  } else {
    struct stat stFileInfo;
    for (auto& fn : opt.files) {
      auto intStat = stat(fn.c_str(), &stFileInfo);
      if (intStat != 0) {
        cerr << ERROR_STR << " file not found " << fn << endl;
        ret = false;
      }
    }
  }
  if (opt.nfiles <= 0) {
    std::cerr << ERROR_STR << " nFastqs must be a non-zero positive number" << std::endl;
    ret = false;
  }
  else {
    if (opt.files.size() % opt.nfiles != 0) {
      std::cerr << ERROR_STR << " incorrect number of FASTQ file(s)" << std::endl;
      ret = false;
    }
  }
  if (opt.mapping_file.empty() && !opt.trim_only) {
    std::cerr << ERROR_STR << " --mapping must be provided" << std::endl;
    ret = false;
  }
  
  bool output_files_specified = opt.output_files.size() > 0 || opt.unassigned_files.size() > 0 || !opt.outputb_file.empty();
  if (opt.output_files.size() == 0 && output_files_specified && !opt.pipe) {
    std::cerr << ERROR_STR << " --output not provided" << std::endl;
    ret = false;
  }
  if (opt.no_output) {
    if (output_files_specified || opt.pipe) {
      std::cerr << ERROR_STR << " Cannot specify an output option when --no-output is specified" << std::endl;
      ret = false;
    }
    if (opt.mod_names) {
      std::cerr << ERROR_STR << " Cannot use --mod-names when --no-output is specified" << std::endl;
      ret = false;
    }
    if (opt.gzip) {
      std::cerr << ERROR_STR << " Cannot use --gzip when --no-output is specified" << std::endl;
      ret = false;
    }
  } else {
    if (!output_files_specified && !opt.pipe) {
      std::cerr << ERROR_STR << " Must either specify an output option or --no-output" << std::endl;
      ret = false;
    } else if (opt.pipe) {
      if (opt.output_files.size() > 0 || !opt.outputb_file.empty()) { // Still allow --unassigned with --pipe
        std::cerr << ERROR_STR << " Cannot provide output files when --pipe is specified" << std::endl;
        ret = false;
      } else if (opt.gzip && opt.unassigned_files.size() == 0) { // Still allow --unassigned with --pipe --gzip
        std::cerr << ERROR_STR << " Cannot use --gzip when no output files are specified" << std::endl;
        ret = false;
      }
    } else {
      if (opt.output_files.size() % opt.nfiles != 0 || opt.unassigned_files.size() % opt.nfiles != 0) {
        std::cerr << ERROR_STR << " Incorrect number of output files" << std::endl;
        ret = false;
      }
    }
  }
  if (opt.trim_only && opt.no_output) {
    std::cerr << ERROR_STR << " Cannot use --trim-only with --no-output" << std::endl;
    ret = false;
  }
  if (opt.trim_only && opt.unassigned_files.size() != 0) {
    std::cerr << ERROR_STR << " Cannot use --trim-only with --unassigned" << std::endl;
    ret = false;
  }
  if (opt.trim_only && !opt.outputb_file.empty()) {
    std::cerr << ERROR_STR << " Cannot use --trim-only with --outb" << std::endl;
    ret = false;
  }
  if (opt.trim_only && !opt.mapping_file.empty()) {
    std::cerr << ERROR_STR << " Cannot use --trim-only with --mapping" << std::endl;
    ret = false;
  }
  opt.output_fastq_specified = output_files_specified;
  opt.verbose = !opt.pipe;
  
  if (!opt.barcode_str.empty() && !opt.config_file.empty()) {
    std::cerr << ERROR_STR << " Cannot specify both --barcodes and --config" << std::endl;
    ret = false;
  } else if (!opt.barcode_str.empty()) {
    stringstream ss1(opt.barcode_str);
    stringstream ss2(opt.distance_str);
    stringstream ss3(opt.barcode_identifiers_str);
    stringstream ss4(opt.location_str);
    stringstream ss5(opt.max_finds_str);
    stringstream ss6(opt.min_finds_str);
    stringstream ss7(opt.exclude_str);
    stringstream ss8(opt.left_str);
    stringstream ss9(opt.right_str);
    while (ss1.good()) {
      uint16_t max_finds = 0;
      uint16_t min_finds = 0;
      bool exclude = false;
      string name = "";
      string location = "";
      string distance = "";
      string left_str = "";
      string right_str = "";
      bool trim_left, trim_right;
      int trim_left_offset, trim_right_offset;
      int16_t file;
      int32_t pos_start;
      int32_t pos_end;
      int mismatch, indel, total_dist;
      string bc;
      getline(ss1, bc, ',');
      if (!opt.distance_str.empty()) {
        auto currpos = ss2.tellg();
        if (!ss2.good()) {
          std::cerr << ERROR_STR << " Number of values in --distances is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        getline(ss2, distance, ',');
        if (!ss2.good() && ss1.good() && currpos == 0) {
          ss2.clear();
          ss2.str(opt.distance_str);
        }
      }
      if (!SplitCode::parseDistance(distance, mismatch, indel, total_dist)) {
        std::cerr << ERROR_STR << " --distances is invalid" << std::endl;
        ret = false;
        break;
      }
      if (!opt.barcode_identifiers_str.empty()) {
        if (!ss3.good()) {
          std::cerr << ERROR_STR << " Number of values in --ids is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        getline(ss3, name, ',');
      }
      if (!opt.location_str.empty()) {
        auto currpos = ss4.tellg();
        if (!ss4.good()) {
          std::cerr << ERROR_STR << " Number of values in --locations is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        getline(ss4, location, ',');
        if (!ss4.good() && ss1.good() && currpos == 0) {
          ss4.clear();
          ss4.str(opt.location_str);
        }
      }
      if (!SplitCode::parseLocation(location, file, pos_start, pos_end, opt.nfiles)) {
        std::cerr << ERROR_STR << " --locations is invalid" << std::endl;
        ret = false;
        break;
      }
      if (!opt.max_finds_str.empty()) {
        auto currpos = ss5.tellg();
        if (!ss5.good()) {
          std::cerr << ERROR_STR << " Number of values in --maxFinds is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        string f;
        getline(ss5, f, ',');
        stringstream(f) >> max_finds;
        if (!ss5.good() && ss1.good() && currpos == 0) {
          ss5.clear();
          ss5.str(opt.max_finds_str);
        }
      }
      if (!opt.min_finds_str.empty()) {
        auto currpos = ss6.tellg();
        if (!ss6.good()) {
          std::cerr << ERROR_STR << " Number of values in --minFinds is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        string f;
        getline(ss6, f, ',');
        stringstream(f) >> min_finds;
        if (!ss6.good() && ss1.good() && currpos == 0) {
          ss6.clear();
          ss6.str(opt.min_finds_str);
        }
      }
      if (!opt.exclude_str.empty()) {
        if (!ss7.good()) {
          std::cerr << ERROR_STR << " Number of values in --exclude is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        string f;
        getline(ss7, f, ',');
        stringstream(f) >> exclude;
      }
      if (!opt.left_str.empty()) {
        auto currpos = ss8.tellg();
        if (!ss8.good()) {
          std::cerr << ERROR_STR << " Number of values in --left is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        string f;
        getline(ss8, f, ',');
        stringstream(f) >> left_str;
        if (!ss8.good() && ss1.good() && currpos == 0) {
          ss8.clear();
          ss8.str(opt.left_str);
        }
      }
      if (!SplitCode::parseTrimStr(left_str, trim_left, trim_left_offset)) {
        std::cerr << ERROR_STR << " --left is invalid" << std::endl;
        ret = false;
        break;
      }
      if (!opt.right_str.empty()) {
        auto currpos = ss9.tellg();
        if (!ss9.good()) {
          std::cerr << ERROR_STR << " Number of values in --right is less than that in --barcodes" << std::endl;
          ret = false;
          break;
        }
        string f;
        getline(ss9, f, ',');
        stringstream(f) >> right_str;
        if (!ss9.good() && ss1.good() && currpos == 0) {
          ss9.clear();
          ss9.str(opt.right_str);
        }
      }
      if (!SplitCode::parseTrimStr(right_str, trim_right, trim_right_offset)) {
        std::cerr << ERROR_STR << " --right is invalid" << std::endl;
        ret = false;
        break;
      }
      if (trim_left && trim_right) {
        std::cerr << ERROR_STR << " One of the barcodes has both --left and --right trimming specified" << std::endl;
        ret = false;
        break;
      }
      auto trim_dir = trim_left ? sc.left : (trim_right ? sc.right : sc.nodir);
      auto trim_offset = trim_left ? trim_left_offset : (trim_right ? trim_right_offset : 0);
      if (!sc.addTag(bc, name.empty() ? bc : name, mismatch, indel, total_dist, file, pos_start, pos_end, max_finds, min_finds, exclude, trim_dir, trim_offset)) {
        std::cerr << ERROR_STR << " Could not finish processing supplied barcode list" << std::endl;
        ret = false;
        break;
      }
    }
    if (ret && !opt.distance_str.empty() && ss2.good()) {
      std::cerr << ERROR_STR << " Number of values in --distances is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.barcode_identifiers_str.empty() && ss3.good()) {
      std::cerr << ERROR_STR << " Number of values in --ids is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.location_str.empty() && ss4.good()) {
      std::cerr << ERROR_STR << " Number of values in --locations is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.max_finds_str.empty() && ss5.good()) {
      std::cerr << ERROR_STR << " Number of values in --maxFinds is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.min_finds_str.empty() && ss6.good()) {
      std::cerr << ERROR_STR << " Number of values in --minFinds is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.exclude_str.empty() && ss7.good()) {
      std::cerr << ERROR_STR << " Number of values in --exclude is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.left_str.empty() && ss8.good()) {
      std::cerr << ERROR_STR << " Number of values in --left is greater than that in --barcodes" << std::endl;
      ret = false;
    }
    if (ret && !opt.right_str.empty() && ss9.good()) {
      std::cerr << ERROR_STR << " Number of values in --right is greater than that in --barcodes" << std::endl;
      ret = false;
    }
  } else if (!opt.distance_str.empty()) {
    std::cerr << ERROR_STR << " --distances cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.barcode_identifiers_str.empty()) {
    std::cerr << ERROR_STR << " --ids cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.location_str.empty()) {
    std::cerr << ERROR_STR << " --locations cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.max_finds_str.empty()) {
    std::cerr << ERROR_STR << " --maxFinds cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.min_finds_str.empty()) {
    std::cerr << ERROR_STR << " --minFinds cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.exclude_str.empty()) {
    std::cerr << ERROR_STR << " --exclude cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.left_str.empty()) {
    std::cerr << ERROR_STR << " --left cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.right_str.empty()) {
    std::cerr << ERROR_STR << " --right cannot be supplied unless --barcodes is" << std::endl;
    ret = false;
  } else if (!opt.config_file.empty()) {
    ret = ret && sc.addTags(opt.config_file);
  }
  
  if (ret && !opt.append_file.empty()) {
    ret = ret && sc.addExistingMapping(opt.append_file);
  }

  if (ret && !opt.keep_file.empty()) {
    ret = ret && sc.addFilterList(opt.keep_file, opt.discard);
  }
  
  if (ret && (sc.getNumTags() == 0 || sc.getMapSize() == 0)) {
    std::cerr << ERROR_STR << " No barcodes found" << std::endl;
    ret = false;
  }
  
  return ret;
}


int main(int argc, char *argv[]) {
  std::cout.sync_with_stdio(false);
  setvbuf(stdout, NULL, _IOFBF, 1048576);
  ProgramOptions opt;
  ParseOptions(argc,argv,opt);
  SplitCode sc(opt.nfiles, opt.trim_only);
  if (!CheckOptions(opt, sc)) {
    usage();
    exit(1);
  }
  if (opt.verbose) {
  std::cerr << "* Using a list of " << sc.getNumTags() << " barcodes (map size: " << pretty_num(sc.getMapSize()) << "; num elements: " << pretty_num(sc.getMapSize(false)) << ")" << std::endl;
  }
  MasterProcessor MP(sc, opt);
  ProcessReads(MP, opt);
  fflush(stdout);
  if (!opt.mapping_file.empty()) {
    sc.writeBarcodeMapping(opt.mapping_file);
  }

  return 0;
}
