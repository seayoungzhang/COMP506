#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

// Defination of data structure for operation
struct operation {
  int lineNum = -1;
  string label = "";
  string code = "";
  string op1 = "";
  string op2 = "";
  string op3 = "";
  string op4 = "";

  operation(string l="", string c="", string o1="", string o2="", string o3="", string o4="") {
    label = l;
    code = c;
    op1 = o1;
    op2 = o2;
    op3 = o3;
    op4 = o4;
  }
};

string opa[] = {"nop", "halt", "br", "cbr", "output", "coutput", "write", "cwrite", "read", "cread", "not", "loadI", "load", "cload", "store", "cstore", "i2i", "c2c", "i2c", "c2i", "storeAI", "storeAO", "cstoreAI", "cstoreAO"};
set<string> ops(opa, opa+24);
vector<int> lead;
vector<int> last;
vector<operation> gops;
map<string, int> llnm;

// Parse ILOC code from input file stream
void readIloc(string filename) {bool unrollFlag;

  ifstream file(filename, ofstream::in);
  int lineCnt = 0;
  string line = "";

  while(getline(file, line)) {
    operation op;
    string next;
    istringstream is(line);
    is >> next;

    if(next.back() == ':') {
      next.pop_back();
      op.label = next;
      op.code = "nop";
      llnm[next] = lineCnt;
    }
    else {
      op.code = next;
      if(op.code == "br") {
        is >> next >> op.op3;
      } else if(op.code == "cbr") {
        is >> op.op1 >> next >> op.op3 >> op.op4;
        op.op3.pop_back();
      } else if(op.code == "output" || op.code == "coutput" || op.code == "write" || op.code == "cwrite") {
        is >> op.op1;
      } else if(op.code == "read" || op.code == "cread") {
        is >> next >> op.op3;
      } else if(op.code == "not" || op.code == "loadI" || op.code == "load" || op.code == "cload" || op.code == "store" || op.code == "cstore" || op.code == "i2i" || op.code == "c2c" || op.code == "i2c" || op.code == "c2i") {
        is >> op.op1 >> next >> op.op3;
      } else if(op.code == "storeAI" || op.code == "storeAO" || op.code == "cstoreAI" || op.code == "cstoreAO") {
        is >> op.op1 >> next >> op.op3 >> op.op4;
        op.op3.pop_back();
      } else {
        is >> op.op1 >> op.op2 >> next >> op.op3;
        op.op1.pop_back();
      }
    }

    op.lineNum = lineCnt;
    gops.push_back(op);
    ++ lineCnt;
  }

  file.close();
}

// Write ILOC back to new file
void writeIloc(string filename) {
  ofstream file(filename, ofstream::out);
  for(auto op: gops) {
    if(op.code == "nop") {
      file << op.label << ":\tnop" << endl;
    } else if(op.code == "halt") {
      file << "\thalt" << endl;
    } else if(op.code == "br") {
      file << "\tbr -> " << op.op3 << endl;
    } else if(op.code == "cbr") {
      file << "\tcbr " << op.op1 << " -> " << op.op3 << ", " << op.op4 << endl;
    } else if(op.code == "output" || op.code == "coutput" || op.code == "write" || op.code == "cwrite") {
      file << "\t" << op.code << " " << op.op1 << endl;
    } else if(op.code == "read" || op.code == "cread") {
      file << "\t" << op.code << " => " << op.op3 << endl;
    } else if(op.code == "not" || op.code == "loadI" || op.code == "load" || op.code == "cload" || op.code == "store" || op.code == "cstore" || op.code == "i2i" || op.code == "c2c" || op.code == "i2c" || op.code == "c2i") {
      file << "\t" << op.code << " " << op.op1 << " => " << op.op3 << endl;
    } else if(op.code == "storeAI" || op.code == "storeAO" || op.code == "cstoreAI" || op.code == "cstoreAO") {
      file << "\t" << op.code << " " << op.op1 << " => " << op.op3 << ", " << op.op4 << endl;
    } else {
      file << "\t" << op.code << " " << op.op1 << ", " << op.op2 << " => " << op.op3 << endl;
    }
  }

  file.close();
}

// Print help information
void help(){
  cout << "Usage: ./opt [-h] [-u] [-v] [filename]" << endl;
  cout << "\t-u Loop Unrolling" << endl;
  cout << "\t-v Value Numbering" << endl;
  cout << "\t-h Help Information" << endl;

  return;
}

// Build CFG
void cfg(){
  set<int> leads;
  lead.clear();
  last.clear();
  lead.push_back(0);
  leads.insert(0);

  for(auto op: gops) {
    if(op.code == "br") {
      int lineNum = llnm[op.op3];
      if(!leads.count(lineNum)) {
        lead.push_back(lineNum);
        leads.insert(lineNum);
      }
    }
    else if(op.code == "cbr") {
      int lineNum = llnm[op.op3];
      if(!leads.count(lineNum)) {
        lead.push_back(lineNum);
        leads.insert(lineNum);
      }
      lineNum = llnm[op.op4];
      if(!leads.count(lineNum)) {
        lead.push_back(lineNum);
        leads.insert(lineNum);
      }
    }
  }

  for(auto l: lead) {
    while(l < gops.size()-1 && !leads.count(l+1)) ++l;
    last.push_back(l);
  }
}

void loopUnrolling() {
  cfg();
  sort(lead.begin(), lead.end());
  sort(last.begin(), last.end());

  // Find the next label number
  int labelCnt = -1;
  for(auto iter=llnm.begin(); iter!=llnm.end(); ++iter) {
    labelCnt = max(labelCnt, stoi((iter->first).substr(1, (iter->first).length()-1)));
  }

  // Find the next register number
  int regCnt = -1;
  for(auto iter=gops.begin(); iter!=gops.end(); ++iter) {
    if((iter->op1).length() > 0 && (iter->op1)[0] == 'r') regCnt = max(regCnt, stoi((iter->op1).substr(1, (iter->op1).length()-1)));
    if((iter->op2).length() > 0 && (iter->op2)[0] == 'r') regCnt = max(regCnt, stoi((iter->op2).substr(1, (iter->op2).length()-1)));
    if((iter->op3).length() > 0 && (iter->op3)[0] == 'r') regCnt = max(regCnt, stoi((iter->op3).substr(1, (iter->op3).length()-1)));
    if((iter->op4).length() > 0 && (iter->op4)[0] == 'r') regCnt = max(regCnt, stoi((iter->op4).substr(1, (iter->op4).length()-1)));
  }

  // Unroll each loop
  vector<operation> newGops;
  for(int i=0; i<lead.size(); ++i){
    if(gops[lead[i]].label != "" && gops[last[i]].code == "cbr" && gops[lead[i]].label == gops[last[i]].op3 && gops[last[i]-2].code == "addI") {
      newGops.push_back(gops[lead[i]]);
      newGops.push_back(operation("", "sub", gops[last[i]-1].op2, gops[last[i]-1].op1, "r"+to_string(++regCnt)));
      newGops.push_back(operation("", "rshiftI", "r"+to_string(regCnt), "2", "r"+to_string(regCnt)));
      newGops.push_back(operation("", "lshiftI", "r"+to_string(regCnt), "2", "r"+to_string(regCnt)));
      newGops.push_back(operation("", "add", gops[last[i]-1].op1, "r"+to_string(regCnt), "r"+to_string(regCnt+1))); ++regCnt;
      newGops.push_back(operation("", "cbr", "r"+to_string(regCnt), "", "L"+to_string(labelCnt+1), "L"+to_string(labelCnt+2)));
      newGops.push_back(operation("L"+to_string(labelCnt+1), "nop"));

      for(int j=0; j<4; ++j) {
        newGops.insert(newGops.end(), gops.begin()+lead[i]+1, gops.begin()+last[i]-1);
      }

      if(gops[last[i]-1].code == "cmp_LE") {
        newGops.push_back(operation("", "cmp_LT", gops[last[i]-1].op1, "r"+to_string(regCnt), gops[last[i]].op1));
      } else {
        newGops.push_back(operation("", "cmp_LE", gops[last[i]-1].op1, "r"+to_string(regCnt), gops[last[i]].op1));
      }

      newGops.push_back(operation("", "cbr", gops[last[i]].op1, "", "L"+to_string(labelCnt+1), "L"+to_string(labelCnt+2)));
      newGops.push_back(operation("L"+to_string(labelCnt+2), "nop"));
      newGops.push_back(gops[last[i]-1]);
      newGops.push_back(operation("", "cbr", gops[last[i]].op1, "", "L"+to_string(labelCnt+3), gops[last[i]].op4));
      newGops.push_back(operation("L"+to_string(labelCnt+3), "nop"));
      newGops.insert(newGops.end(), gops.begin()+lead[i]+1, gops.begin()+last[i]+1);
      labelCnt += 3;
    }
    else {
      newGops.insert(newGops.end(), gops.begin()+lead[i], gops.begin()+last[i]+1);
    }
  }

  gops.swap(newGops);

  // Update line numbers
  for(int i=0; i<gops.size(); ++i) {
    gops[i].lineNum = i;
    if(gops[i].label.size() > 0) {
      llnm[gops[i].label] = i;
    }
  }
  return;
}

void valueNumbering() {
  cfg();

  map<int, string> vom;
  map<string, int> evm;
  map<string, int> ovm;
  int blockNum = lead.size();
  for(int i=0; i<blockNum; ++i) {
    vom.clear();
    evm.clear();
    ovm.clear();
    int valueNumber = 0;

    for(int j=lead[i]; j<=last[i]; ++j) {
      if(gops[j].code == "halt") break;
      if(ops.count(gops[j].code)) continue;

      if(!ovm.count(gops[j].op1)) ovm[gops[j].op1] = valueNumber++;
      if(!ovm.count(gops[j].op2)) ovm[gops[j].op2] = valueNumber++;
      string eq = to_string(ovm[gops[j].op1]) + gops[j].code + to_string(ovm[gops[j].op2]);
      if (evm.count(eq)) {
      	int vn = evm[eq];
      	string op = vom[vn];
      	if (ovm[op] == vn) {
      		gops[j].code = "i2i";
      		gops[j].op1 = op;
          gops[j].op2 = "";
          gops[j].op4 = "";
      	}
      	ovm[gops[j].op3] = vn;
      } else {
      	evm[eq] = valueNumber;
      	ovm[gops[j].op3] = valueNumber;
      	vom[valueNumber ++] = gops[j].op3;
      }
    }
  }

  return;
}

// Driver
int main (int argc, char* argv[]) {
  if(argc <= 2) {
    help();
    return 0;
  }

  string inFileName(argv[argc-1]);
  string outFileName = inFileName;
  readIloc(inFileName);
  for(int i=1; i<argc-1; ++i) {
    if(string(argv[i]) == "-v") {
      outFileName = "v_" + outFileName;
      valueNumbering();
    }
    else if(string(argv[i]) == "-u") {
      outFileName = "u_" + outFileName;
      loopUnrolling();
    }
    else if(string(argv[i]) == "-i") {
      cout << "-i: loop-invariant code motion not implemented" << endl;
    }
    else {
      help();
      break;
    }
  }
  outFileName = "opt_" + outFileName;
  writeIloc(outFileName);

  return 0;
}
