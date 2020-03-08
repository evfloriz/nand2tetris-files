#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <experimental/filesystem>
#include <sstream>

using namespace std;
namespace fs = experimental::filesystem;

/*
  implemented:
  version 0:  made Sys.init a proper function call (passes StaticsTest)
  version 1:  stored temporary return values in R14 and R15 instead of R5 and R6
              (passes NestedCall)
  version 2:  made each function call create a unique label for the return address
              (passes FibonacciElement)

  planned:
  version 3:  improved comments (ability to identify and erase /* ... *\/ over multiple lines)
*/

const string vmVersion = "2";

// parses the vm code and provides an easy way to access the important parts
class Parser
{
public:

  ifstream file;              // input stream
  char type;                  // type (A or C)
  string currentLine;         // current line in the file
  string firstPart;
  string secondPart;
  string thirdPart;
  string currentFile;
  vector<string> fileVector;
  string directory = "";

  // constructor to create vector of strings containing .vm files
  Parser(string fileName)
  {
    if (fileName.find(".vm") != -1)         // if .vm is found
    {
      fileVector.push_back(fileName);       // vector will have 1 item (not that efficient but whatever)
    }
    else                                    // if .vm is not found, assume it's a directory
    {
      for (auto & p : fs::directory_iterator(fileName))
      {
        ostringstream ossPath;
        ossPath << p;
        string tempPath = ossPath.str();        // streams into ostringstream

        if (tempPath.find(".vm") != -1)             // checks if its a vm file
        {
            fileVector.push_back(tempPath);        // pushes back for each valid .vm file
        }
      }

      directory = fileName + "\\";
    }
  }

  string getDirectory()
  {
    return directory;
  }

  // if its just one, return 1 file, and open that one
  // if not, get number of files
  //  create array of strings thats equal to the number of files
  //  iterate through and open, read, and close each file in a for loop
  // void fileOpen(int fileNumber)

  void fileOpen(int fileNumber)
  {
    file.open(fileVector[fileNumber]);
  }

  string currentVMFile(int fileNumber)
  {
    string staticName = fileVector[fileNumber];

    int slashLocation = staticName.find("\\");            // erases the directory location from the filename
    if (slashLocation != -1)
      staticName.erase(0, slashLocation+1);

    int vmLocation = staticName.find(".vm");           // erases the .vm from the filename
    if (vmLocation != -1)
      staticName.erase(vmLocation);

    return staticName;
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

      format();
    }
  }

  /*
    returns type of command
    Arithmetic    A
    Push          X
    Pop           Y
    Label         L
    Goto          G
    If            I
    Function      F
    Return        R
    Call          C
    null          Z
  */
  char commandType()
  {
    char comType;

    if (firstPart == "add" || firstPart == "sub" || firstPart == "neg" ||
      firstPart == "eq" || firstPart == "gt" || firstPart == "lt" ||
      firstPart == "and" || firstPart == "or" || firstPart == "not")
      comType = 'A';
    else if (firstPart == "push")
      comType = 'X';
    else if (firstPart == "pop")
      comType = 'Y';
    else if (firstPart == "label")
      comType = 'L';
    else if (firstPart == "goto")
      comType = 'G';
    else if (firstPart == "if-goto")
      comType = 'I';
    else if (firstPart == "function")
      comType = 'F';
    else if (firstPart == "return")
      comType = 'R';
    else if (firstPart == "call")
      comType = 'C';
    else
      comType = 'Z';

    return comType;
  }


  // returns first argument
  string arg1()
  {
    string tempArg;

    if (commandType() == 'A')
      tempArg = firstPart;
    else
      tempArg = secondPart;

    return tempArg;
  }

  int arg2()
  {
    return stoi(thirdPart);
  }

  // formats currentLine and splits it into up to 3 parts
  void format()
  {
    if (currentLine.find("//") != -1)
      currentLine.erase(currentLine.find("//"));

    if (currentLine.find_first_not_of(' ') == -1)
      currentLine.erase(remove(currentLine.begin(), currentLine.end(), ' '), currentLine.end());


    firstPart = "";
    secondPart = "";
    thirdPart = "";

    int letter1 = currentLine.find_first_not_of(' ');
    int space1 = currentLine.find_first_of(' ', letter1);
    if (letter1 != -1)
      firstPart = currentLine.substr(letter1, space1-letter1);

    int letter2 = currentLine.find_first_not_of(' ', space1);
    int space2 = currentLine.find_first_of(' ', letter2);
    if (letter2 != -1)
      secondPart = currentLine.substr(letter2, space2-letter2);

    int letter3 = currentLine.find_first_not_of(' ', space2);
    int space3 = currentLine.find_first_of(' ', letter3);
    if (space3 == -1)
      space3 = currentLine.length();
    if (letter3 != -1)
      thirdPart = currentLine.substr(letter3, space3-letter3);
  }

  void fileClose()
  {
    file.close();
  }
};


class CodeWriter
{
public:
ofstream file;
int i1 = 0;                    // counters for the comparison labels of writeArithmetic
int i2 = 0;
int i3 = 0;
int iReturn = 0;               // counter for return labels generated in writeCall
string staticName;
string currentFunction;


  CodeWriter(string fileName)
  {
    file.open(fileName);
    writeInit();
  }

  void writeArithmetic (string command)
  {

    if (command == "add")
    {
      arithmeticPushPop(true);
      file << "M=D+M" << endl;
    }
    else if (command == "sub")
    {
      arithmeticPushPop(true);
      file << "M=M-D" << endl;
    }
    else if (command == "neg")
    {
      arithmeticPushPop(false);
      file << "M=-M" << endl;
    }
    else if (command == "eq")
    {
      arithmeticPushPop(true);                      //x = M(256), y = M(257)
      file << "D=M-D"               << endl;        //D=x-y
      file << "@EQ" << i1           << endl;
      file << "D;JEQ"               << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=0"                 << endl;
      file << "@ENDEQ" << i1        << endl;
      file << "0;JMP"               << endl;
      file << "(EQ" << i1 << ")"    << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=-1"                << endl;
      file << "(ENDEQ" << i1 << ")" << endl;
      i1++;
    }
    else if (command == "gt")
    {
      arithmeticPushPop(true);
      file << "D=M-D"               << endl;        //D=x-y
      file << "@GT" << i2           << endl;
      file << "D;JGT"               << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=0"                << endl;
      file << "@ENDGT" << i2        << endl;
      file << "0;JMP"               << endl;
      file << "(GT" << i2 << ")"    << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=-1"                 << endl;
      file << "(ENDGT" << i2 << ")" << endl;
      i2++;
    }
    else if (command == "lt")
    {
      arithmeticPushPop(true);
      file << "D=M-D"               << endl;        //D=x-y
      file << "@LT" << i3           << endl;
      file << "D;JLT"               << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=0"                << endl;
      file << "@ENDLT" << i3        << endl;
      file << "0;JMP"               << endl;
      file << "(LT" << i3 << ")"    << endl;
      file << "@SP"                 << endl;
      file << "A=M-1"               << endl;
      file << "M=-1"                 << endl;
      file << "(ENDLT" << i3 << ")" << endl;
      i3++;
    }
    else if (command == "and")
    {
      arithmeticPushPop(true);
      file << "M=D&M" << endl;
    }
    else if (command == "or")
    {
      arithmeticPushPop(true);
      file << "M=D|M" << endl;
    }
    else if (command == "not")
    {
      arithmeticPushPop(false);
      file << "M=!M" << endl;
    }

  }


  void writePushPop (char command, string segment, int index)
  {
    // push command, adds to stack and increases SP
    if (command == 'X')
    {
      // pushes constant 'index' to location M(SP)
      writeSegment(segment, index);

      if (segment != "constant")
        file << "D=M"   << endl;          // D becomes the value of local 0 in "push local 0" (eg)

      stackPushPop(true);
    }
    else if (command == 'Y')
    {
      writeSegment(segment, index);      //AD = location to store data

      stackPushPop(false);

    }
  }

  void writeSegment(string segment, int index)
  {
    if (segment == "constant")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
    }
    else if (segment == "local")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@LCL"        << endl;
      file << "AD=M+D"      << endl;

    }
    else if (segment == "argument")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@ARG"        << endl;
      file << "AD=M+D"      << endl;
    }
    else if (segment == "this")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@THIS"       << endl;
      file << "AD=M+D"      << endl;
    }
    else if (segment == "that")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@THAT"       << endl;
      file << "AD=M+D"      << endl;
    }
    else if (segment == "pointer")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@3"          << endl;
      file << "AD=A+D"      << endl;
    }
    else if (segment == "temp")
    {
      file << "@" << index  << endl;
      file << "D=A"         << endl;
      file << "@5"          << endl;
      file << "AD=A+D"      << endl;
    }
    else if (segment == "static")
    {
      string name = staticName + "." + to_string(index);

      file << "@" << name << endl;
      file << "D=A"             << endl;

    }
  }


  void arithmeticPushPop(bool binary)
  {

    // ends up with SP at 1 less than original,
    // the higher value in the stack (y) in D and
    // the lower value in the stack (x) in M(SP-1).
    // initialized to overwrite the lower value M(SP-1) in the stack with the new value
    if (binary)
    {
      file << "@SP"     << endl;          //SP = 258
      file << "AM=M-1"  << endl;          //SP = 257, A=257
      file << "D=M"     << endl;          //D = M(257)
      file << "@SP"     << endl;          //SP = 257
      file << "A=M-1"   << endl;          //SP = 257, A=256
    }

    // ends up with SP same as original
    // initialized to overwrite the old value M(SP-1) in the stack with the new value
    else
    {
      file << "@SP"   << endl;          //SP = 258
      file << "A=M-1" << endl;          //SP = 258, A=257
    }
  }

  void writeInit()
  {
    file << "// Created with VM.cpp version " << vmVersion << endl;
    file << "@256"  << endl;
    file << "D=A"   << endl;

    file << "@SP"   << endl;
    file << "M=D"   << endl;         // initialize SP

    writeCall("Sys.init", 0);
  }

  void writeLabel(string label)
  {
    file << "(" << currentFunction << "$" << label <<  ")" << endl;
  }

  void writeGoto(string label)
  {
    file << "@" << currentFunction << "$" << label << endl;   // load the specified label
    file << "0;JMP"                                << endl;   // always jump
  }

  void writeIf(string label)
  {
    file << "@SP"    << endl;
    file << "AM=M-1" << endl;
    file << "D=M"    << endl;                                 // pop the top of the stack into D

    file << "@" << currentFunction << "$" << label << endl;   // load the specified label
    file << "D;JNE"                                << endl;   // jump if D != 0
  }

  void writeCall(string functionName, int numArgs)
  {
    // push return-address
    file << "@" << functionName << "$$RETURN" << iReturn << endl;        // load return address
    file << "D=A" << endl;                                    // stores return address in D (A or M?????)
    stackPushPop(true);

    // push LCL
    file << "@LCL" << endl;
    file << "D=M" << endl;
    stackPushPop(true);

    // push ARG
    file << "@ARG" << endl;
    file << "D=M" << endl;
    stackPushPop(true);

    // push THIS
    file << "@THIS" << endl;
    file << "D=M" << endl;
    stackPushPop(true);

    // push THAT
    file << "@THAT" << endl;
    file << "D=M" << endl;
    stackPushPop(true);

    // ARG = SP-n-5
    file << "@" << numArgs  << endl;
    file << "D=A"           << endl;                // D = numArgs
    file << "@R14"          << endl;
    file << "M=D"           << endl;                // M(R14) = numArgs
    file << "@5"            << endl;
    file << "D=A"           << endl;
    file << "@R14"          << endl;
    file << "M=M+D"         << endl;              // M(14) = numArgs + 5

    file << "@SP"   << endl;
    file << "D=M"   << endl;                // D = *SP
    file << "@R14"  << endl;
    file << "D=D-M" << endl;              // D = SP - (numArgs + 5)

    file << "@ARG"  << endl;
    file << "M=D"   << endl;                // M(ARG) = SP - numArgs - 5s

    // LCL = SP
    file << "@SP"   << endl;
    file << "D=M"   << endl;                // D = SP
    file << "@LCL"  << endl;
    file << "M=D"   << endl;                // LCL = SP

    // goto functionName
    file << "@" << functionName << endl;
    file << "0;JMP"             << endl;

    // (return-address)
    file << "(" << functionName << "$$RETURN" << iReturn << ")" << endl;       // declare return address label

    iReturn++;
  }

  void writeReturn()
  {
    // FRAME = LCL (frame is temp variable, R14)
    file << "@LCL"  << endl;
    file << "D=M"   << endl;    // LCL stored in D
    file << "@R14"   << endl;
    file << "M=D"   << endl;    // LCL stored in R14 (frame)

    // RET = *(FRAME-5) (ret is temp variable, R15)
    file << "@5"    << endl;
    file << "D=A"   << endl;        // D=5
    file << "@R14"   << endl;
    file << "A=M-D" << endl;
    file << "D=M"   << endl;      // D = *(frame-5)
    file << "@R15"   << endl;
    file << "M=D"   << endl;        // M(R15) = *(frame-5)

    // *ARG = pop()
    file << "@ARG"  << endl;
    file << "D=M"   << endl;        // D set to the value of the memory location pointed to by ARG
    stackPushPop(false);            // top of stack stored in D (*ARG)

    // SP = ARG+1
    file << "@ARG"  << endl;
    file << "D=M"   << endl;        // ARG stored in D
    file << "@SP"   << endl;
    file << "M=D+1" << endl;      // SP set to ARG location (doesnt rely on last value of SP)

    // THAT = *(FRAME-1)
    file << "@1"    << endl;
    file << "D=A"   << endl;        // D=5
    file << "@R14"   << endl;
    file << "A=M-D" << endl;    // A set to location 5 below the location pointed to by FRAME (LCL)
    file << "D=M"   << endl;      // D = *(frame-5)
    file << "@THAT" << endl;
    file << "M=D"   << endl;        // M(R15) = *(frame-5)

    // THIS = *(FRAME-2)
    file << "@2"    << endl;
    file << "D=A"   << endl;        // D=5
    file << "@R14"   << endl;
    file << "A=M-D" << endl;
    file << "D=M"   << endl;      // D = *(frame-5)
    file << "@THIS" << endl;
    file << "M=D"   << endl;        // M(R15) = *(frame-5)

    // ARG = *(FRAME-3)
    file << "@3"    << endl;
    file << "D=A"   << endl;        // D=5
    file << "@R14"   << endl;
    file << "A=M-D" << endl;
    file << "D=M"   << endl;      // D = *(frame-5)
    file << "@ARG"  << endl;
    file << "M=D"   << endl;        // M(R15) = *(frame-5)

    // LCL = *(FRAME-4)
    file << "@4"    << endl;
    file << "D=A"   << endl;        // D=5
    file << "@R14"   << endl;
    file << "A=M-D" << endl;
    file << "D=M"   << endl;      // D = *(frame-5)
    file << "@LCL"  << endl;
    file << "M=D"   << endl;        // M(R15) = *(frame-5)

    // goto RET
    file << "@R15"   << endl;
    file << "A=M"   << endl;    // A set to location pointed to by RET (R15)
    file << "0;JMP" << endl;
  }

  void writeFunction(string functionName, int numLocals)
  {
    currentFunction = functionName;
    file << "(" << functionName << ")" << endl;         // function label

    file << "@" << numLocals << endl;                   // loads number of locals
    file << "D=A" << endl;                              // stores it in D

    file << "(" << functionName << "$$LOCALS)" << endl;   // label for looping to initialize all locals to 0

    file << "@" << functionName << "$$ENDLOCALS" << endl; // jumps to the end of the local declaration if D<=0
    file << "D;JLE"                            << endl;

    file << "@SP"   << endl;
    file << "A=M"   << endl;
    file << "M=0"   << endl;                            // initialize current location pointed by SP to 0
    file << "@SP"   << endl;
    file << "M=M+1" << endl;                            // increment SP

    file << "D=D-1"                         << endl;    // decrement D
    file << "@" << functionName << "$$LOCALS" << endl;    // jump to the beginning of the local declaration
    file << "0;JMP"                         << endl;

    file << "(" << functionName << "$$ENDLOCALS)" << endl;  // label for end of function
  }


  // writes the actual push or pop assembly code
  // push:  D must store the value to push to the top of the stack
  // pop:   D must store the location where value at the top of the stack will be stored
  //        R13 must be clear
  void stackPushPop(bool push)
  {
    if (push == true)
    {
      file << "@SP"   << endl;          // pushes the value of D to the stack
      file << "A=M"   << endl;
      file << "M=D"   << endl;
      file << "@SP"   << endl;          // increases the value of SP
      file << "M=M+1" << endl;
    }
    else
    {
      file << "@R13"    << endl;
      file << "M=D"     << endl;        //M(R13) = where to store the data from the top of the stack

      file << "@SP"     << endl;        // decreases the value of SP
      file << "AM=M-1"  << endl;
      file << "D=M"     << endl;        // sets the value of D to be the value at the top of the stack

      file << "@R13"    << endl;
      file << "A=M"     << endl;

      file << "M=D"     << endl;        // stores D in the place specified by R13
    }
  }

  void close()
  {
    file.close();
  }
};


int main()
{
  string vmFile;
  string asmFile;
  string trueOutputFile;

  cout << ".vm file:  ";
  cin >> vmFile;
  cout << ".asm File: ";
  cin >> asmFile;

  Parser parse(vmFile);

  trueOutputFile = parse.getDirectory() + asmFile;

  CodeWriter code(trueOutputFile);

  for (int i=0; i<parse.fileVector.size(); i++)
  {

    // open the next file in the queue
    parse.fileOpen(i);

    // set the name of the current VM file
    code.staticName = parse.currentVMFile(i);

    while(parse.hasMoreCommands())
    {
      parse.advance();

      /*
        returns type of command
        Arithmetic    A
        Push          X
        Pop           Y
        Label         L
        Goto          G
        If            I
        Function      F
        Return        R
        Call          C
        null          Z
        */

      if (parse.commandType() == 'A')
        code.writeArithmetic(parse.arg1());
      else if (parse.commandType() == 'X' || parse.commandType() == 'Y')
        code.writePushPop(parse.commandType(), parse.arg1(), parse.arg2());
      else if (parse.commandType() == 'L')
        code.writeLabel(parse.arg1());
      else if (parse.commandType() == 'G')
        code.writeGoto(parse.arg1());
      else if (parse.commandType() == 'I')
        code.writeIf(parse.arg1());
      else if (parse.commandType() == 'F')
        code.writeFunction(parse.arg1(), parse.arg2());
      else if (parse.commandType() == 'R')
        code.writeReturn();
      else if (parse.commandType() == 'C')
        code.writeCall(parse.arg1(), parse.arg2());

    }

    parse.fileClose();
  }


  code.close();

  cout << "Done" << endl;

  return 0;
}
