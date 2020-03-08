#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;

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

  void close()
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
string staticName;


  CodeWriter(string fileName)
  {
    file.open(fileName);
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

      file << "@SP"   << endl;          // pushes the value of D to the stack
      file << "A=M"   << endl;
      file << "M=D"   << endl;
      file << "@SP"   << endl;          // increases the value of SP
      file << "M=M+1" << endl;
    }
    else if (command == 'Y')
    {
      writeSegment(segment, index);      //AD = location to store data

      file << "@R13"    << endl;
      file << "M=D"     << endl;      //M(R13) = where to store the data from the top of the stack

      file << "@SP"     << endl;        // decreases the value of SP
      file << "AM=M-1"  << endl;
      file << "D=M"     << endl;        // sets the value of D to be the value at the top of the stack

      file << "@R13"    << endl;
      file << "A=M"     << endl;

      file << "M=D"     << endl;

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

  void currentVMFile(string tempStr)
  {
    staticName = tempStr;
    int i = staticName.find(".vm");
    if (i != -1)
      staticName.erase(i);
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
  cout << ".vm file:  ";
  cin >> vmFile;
  cout << ".asm File: ";
  cin >> asmFile;

  Parser parse(vmFile);
  CodeWriter code(asmFile);

  code.currentVMFile(vmFile);

  while(parse.hasMoreCommands())
  {
    parse.advance();

    if (parse.commandType() == 'A')
      code.writeArithmetic(parse.arg1());
    else if (parse.commandType() == 'X' || parse.commandType() == 'Y')
      code.writePushPop(parse.commandType(), parse.arg1(), parse.arg2());

  }

  parse.close();
  code.close();

  return 0;
}
