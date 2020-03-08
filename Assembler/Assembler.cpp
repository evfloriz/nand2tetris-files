#include <iostream>
#include <fstream>
#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <string>

using namespace std;

// parses through an asm file and obtains required data for converting the
// instructions to binary
class Parser
{
public:
  ifstream file;              // input stream
  char type;                  // type (A or C)
  string currentLine;         // current line in the file

  // constructor to open the input stream
  Parser(string fileName)
  {
    file.open(fileName);
  }

  // returns false if end of file (eof) is reached
  bool hasMoreCommands()
  {
    if (file.eof())
      return false;
    else
      return true;
  }

  // sets currentLine to the next line in the file
  void advance()
  {
    if (hasMoreCommands())
    {
      getline(file, currentLine);

      if (currentLine.find("//")!=-1)
        currentLine.erase(currentLine.find("//"));

      currentLine.erase(remove(currentLine.begin(), currentLine.end(), ' '), currentLine.end());

    }
  }

  // returns the type of command (@___ for A commands, ____ for C commands)
  char commandType()
  {
    if (currentLine[0] == '@')
      return 'A';
    else if (currentLine == "")
      return 'Z';
    else if ((currentLine[0] == '(') && (currentLine[currentLine.length()-1] == ')'))
      return 'L';
    else
      return 'C';
  }

  // returns the symbol following @ (for A commands)
  string symbol()
  {
    string tempSymbol;

    if (commandType() == 'L')
    {
      tempSymbol = currentLine.substr(1, currentLine.length()-2);
    }
    else
      tempSymbol = currentLine.substr(1);

    return tempSymbol;
  }

  // returns the computation part of a C command
  string comp()
  {
    string compCode = "";
    int positionEqual = currentLine.find('=');
    int positionSemicolon = currentLine.find(';');

    if (positionEqual>=0 && positionSemicolon>=0)
      compCode = currentLine.substr(positionEqual+1, positionSemicolon-(positionEqual+1));
    else if (positionEqual>=0)
      compCode = currentLine.substr(positionEqual+1);
    else if (positionSemicolon>=0)
      compCode = currentLine.substr(0, positionSemicolon);

    return compCode;
  }

  // returns the destination part of a C command
  string dest()
  {
    string destCode = "";
    int positionEqual = currentLine.find('=');

    if (positionEqual>=0)
      destCode = currentLine.substr(0, positionEqual);

    return destCode;
  }

  // returns the jump part of a C command
  string jump()
  {
    string jumpCode = "";
    int positionSemicolon = currentLine.find(';');

    if (positionSemicolon>=0)
      jumpCode = currentLine.substr(positionSemicolon+1);

    return jumpCode;
  }

  void reset()
  {
    file.clear();
    file.seekg(0);
  }

  void end()
  {
    file.close();
  }

};

class Code
{
public:

  string symbolBin(int addy)
  {
    string address = bitset<15>(addy).to_string();
    return address;
  }

  string compBin(string compCode)
  {
    unordered_map<string, string> compMap;

    compMap["0"]    = "0101010";
    compMap["1"]    = "0111111";
    compMap["-1"]   = "0111010";

    compMap["D"]    = "0001100";
    compMap["A"]    = "0110000";
    compMap["!D"]   = "0001101";
    compMap["!A"]   = "0110001";
    compMap["-D"]   = "0001111";
    compMap["-A"]   = "0110011";

    compMap["D+1"]  = "0011111";
    compMap["A+1"]  = "0110111";
    compMap["D-1"]  = "0001110";
    compMap["A-1"]  = "0110010";
    compMap["D+A"]  = "0000010";
    compMap["D-A"]  = "0010011";
    compMap["A-D"]  = "0000111";
    compMap["D&A"]  = "0000000";
    compMap["D|A"]  = "0010101";

    compMap["M"]    = "1110000";
    compMap["!M"]   = "1110001";
    compMap["-M"]   = "1110011";

    compMap["M+1"]  = "1110111";
    compMap["M-1"]  = "1110010";
    compMap["D+M"]  = "1000010";
    compMap["D-M"]  = "1010011";
    compMap["M-D"]  = "1000111";
    compMap["D&M"]  = "1000000";
    compMap["D|M"]  = "1010101";

    return compMap[compCode];
  }

  string destBin(string destCode)
  {
    unordered_map<string, string> destMap;

    destMap[""]     = "000";
    destMap["M"]    = "001";
    destMap["D"]    = "010";
    destMap["MD"]   = "011";
    destMap["A"]    = "100";
    destMap["AM"]   = "101";
    destMap["AD"]   = "110";
    destMap["AMD"]  = "111";

    return destMap[destCode];
  }

  string jumpBin(string jumpCode)
  {
    unordered_map<string, string> jumpMap;

    jumpMap[""]     = "000";
    jumpMap["JGT"]  = "001";
    jumpMap["JEQ"]  = "010";
    jumpMap["JGE"]  = "011";
    jumpMap["JLT"]  = "100";
    jumpMap["JNE"]  = "101";
    jumpMap["JLE"]  = "110";
    jumpMap["JMP"]  = "111";

    return jumpMap[jumpCode];
  }

};

class SymbolTable
{
public:
  unordered_map<string, int> symbolMap;

  SymbolTable()
  {
    symbolMap["SP"]     = 0;
    symbolMap["LCL"]    = 1;
    symbolMap["ARG"]    = 2;
    symbolMap["THIS"]   = 3;
    symbolMap["THAT"]   = 4;

    symbolMap["R0"]     = 0;
    symbolMap["R1"]     = 1;
    symbolMap["R2"]     = 2;
    symbolMap["R3"]     = 3;
    symbolMap["R4"]     = 4;
    symbolMap["R5"]     = 5;
    symbolMap["R6"]     = 6;
    symbolMap["R7"]     = 7;
    symbolMap["R8"]     = 8;
    symbolMap["R9"]     = 9;
    symbolMap["R10"]    = 10;
    symbolMap["R11"]    = 11;
    symbolMap["R12"]    = 12;
    symbolMap["R13"]    = 13;
    symbolMap["R14"]    = 14;
    symbolMap["R15"]    = 15;

    symbolMap["SCREEN"] = 16384;
    symbolMap["KBD"]    = 24576;
  }

  void addEntry(string symbol, int address)
  {
    symbolMap[symbol] = address;
  }

  bool contains(string symbol)
  {
    return symbolMap.count(symbol);
  }

  int getAddress(string symbol)
  {
    return symbolMap[symbol];
  }

  bool isNumber(string symbol)
  {
    return (symbol.find_first_not_of("0123456789") == -1);
  }

};

int main()
{
  string asmFile;
  string hackFile;
  cout << ".asm file:   ";
  cin >> asmFile;
  cout << ".hack file:  ";
  cin >> hackFile;

  Parser parse(asmFile);
  Code encode;
  SymbolTable table;
  ofstream outFile;
  outFile.open(hackFile);

  string mCode;
  string symbol;
  int lCounter = 0;
  int aCounter = 16;
  int address;

  // first pass through, creates the labels needed for the next run through
  while(parse.hasMoreCommands())
  {
    parse.advance();

    if(parse.commandType() == 'L')
    {
      symbol = parse.symbol();
      table.addEntry(symbol, lCounter);
    }
    else if (parse.commandType() != 'Z')
      lCounter++;

  }

  parse.reset();

  // second pass through, actually generates the hack code
  while(parse.hasMoreCommands())
  {
    parse.advance();

    if (parse.commandType() == 'A')
    {
      symbol = parse.symbol();

      if(table.isNumber(symbol))
      {
        address = stoi(symbol);
      }
      else
      {
        if (!table.contains(symbol))
        {
          table.addEntry(symbol, aCounter);
          aCounter++;
        }

        address = table.getAddress(symbol);
      }

      mCode = "0" + encode.symbolBin(address);
    }

    else if (parse.commandType() == 'C')
    {
      mCode = "111" + encode.compBin(parse.comp())
        + encode.destBin(parse.dest())
        + encode.jumpBin(parse.jump());
    }

    else
      continue;

    outFile << mCode << endl;
  }

  parse.end();
  outFile.close();

  return 0;
};
