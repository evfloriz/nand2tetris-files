#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <experimental/filesystem>
#include <sstream>

using namespace std;
namespace fs = experimental::filesystem;

/*
  version 0: implemented basic SymbolTable
*/

const string compilerVersion = "0";

/*
  opens a file and stores each line in currentLine
  formats currentLine to remove comments and whitespace
  turns formatted text into seperate tokens

  currently: prints to XML
  to implement: passes values into Parser
*/
class Tokenizer
{
public:
  ifstream file;              // input stream
  string currentLine;         // current line in the file
  string currentFile;
  vector<string> fileVector;
  string directory = "";
  vector<string> tokenVector;
  int tokenCounter = 0;

  Tokenizer(string fileName)
  {
    if (fileName.find(".jack") != -1)         // if .jack is found
    {
      fileVector.push_back(fileName);       // vector will have 1 item (not that efficient but whatever)
    }
    else                                    // if .jack is not found, assume it's a directory
    {
      for (auto & p : fs::directory_iterator(fileName))
      {
        ostringstream ossPath;
        ossPath << p;
        string tempPath = ossPath.str();        // streams into ostringstream

        if (tempPath.find(".jack") != -1)             // checks if its a .jack file
        {
            fileVector.push_back(tempPath);        // pushes back for each valid .jack file
        }
      }

      directory = fileName + "\\";
    }
  }

  string getDirectory()
  {
    return directory;
  }

  /*
    if its just one, return 1 file, and open that one.
    if not, get number of files.
    create array of strings thats equal to the number of files.
    iterate through and open, read, and close each file in a for loop.
    void fileOpen(int fileNumber).
  */

  void fileOpen(int fileNumber)
  {
    file.open(fileVector[fileNumber]);
  }

  int fileTotal()
  {
    return fileVector.size();
  }

  string getOutFileName(int fileNumber)
  {
    string tempFileName = fileVector[fileNumber];
    int jackPos = tempFileName.rfind(".jack");

    if (jackPos != -1)
    {
      tempFileName.erase(jackPos);
      tempFileName += ".xml";
      cout << tempFileName << endl;
    }
    else
    {
      cout << "error writing xml filename" << endl;
    }

    return tempFileName;

  }

  bool hasMoreTokens()
  {
    if (file.eof())
      return false;
    else
      return true;
  }

  // sets currentLine to the next line in the file
  void advance()
  {
    if (hasMoreTokens())
    {
      getline(file, currentLine);
      format();
    }
  }

  // gets rid of comments and lines what are just whitespace
  // WATCH OUT FOR TABS
  void format()
  {
    // knocks out lines that are commented by //
    if (currentLine.find("//") != -1)
    {
      currentLine.erase(currentLine.find("//"));
    }

    // knocks out lines that are commented by /**/
    int i = currentLine.find("/*");
    int j = currentLine.find("*/");
    if (i != -1)                      // if /* is found
    {

      if (j != -1)                    // if */ is found in the same line
        currentLine.erase(i, j-i+2);
      else
      {
        while (j == -1)                 // while */ is not found
        {
          getline(file, currentLine);   // skips to the next line
          j = currentLine.find("*/");   // updates j
        }

        currentLine.erase(0, j+2);
      }

    }

    // knocks out the whitespace at the end of lines
    int k = currentLine.find_last_not_of(" \t");
    int l = currentLine.length()-1;
    if (k < l)
      currentLine.erase(k+1, (l-k+1));

    if (currentLine.find_first_not_of(" \t") == -1)
    {
      advance();
    }

  }


  // gets the next token
  // WARNING this is the most hacked together piece of shit, good luck debugging this future evan
  void computeLine()
  {
    advance();
    string symbol = "{}()[].,;+-*/&|<>=~";
    // tokenVector.erase(tokenVector.begin(), tokenVector.end());

    int letter;
    int sym;
    int space;
    int oldSpace = 0;
    int quote1;
    int quote2;

    while (true)
    {

      letter = currentLine.find_first_not_of(" \t\""+symbol, oldSpace);
      sym = currentLine.find_first_of(symbol, oldSpace);
      space = currentLine.find_first_of(" \t", letter);

      // this hopefully fixes the issue with strings ("test test")
      quote1 = currentLine.find_first_of("\"", oldSpace);
      quote2 = currentLine.find_first_of("\"", quote1+1);
      if ((quote1 != -1 && quote2 != -1) && (quote1 < sym) && (quote1 < letter))
      {
        tokenVector.push_back(currentLine.substr(quote1, quote2-quote1+1));
        oldSpace = quote2+1;
        if (quote2 == currentLine.length()-1)
          break;
        continue;
      }

      if (letter != -1)
      {
        if (sym != -1)
        {
          if (letter < sym)
          {
            if (space != -1)
            {
              if (sym<space)
              {
                tokenVector.push_back(currentLine.substr(letter, sym-letter));
                tokenVector.push_back(currentLine.substr(sym, 1));
                oldSpace = sym+1;
                if(sym == currentLine.length()-1)
                  break;
              }
              else
              {
                tokenVector.push_back(currentLine.substr(letter, space-letter));
                oldSpace = space;
              }
            }
            else
            {
              tokenVector.push_back(currentLine.substr(letter, sym-letter));
              tokenVector.push_back(currentLine.substr(sym, 1));
              oldSpace = sym+1;
              if(sym == currentLine.length()-1)
                break;
            }
          }
          else
          {
            tokenVector.push_back(currentLine.substr(sym, 1));
            oldSpace = sym+1;
            if(sym == currentLine.length()-1)
              break;
          }
        }
        else
        {
          if (space != -1)
          {
            tokenVector.push_back(currentLine.substr(letter, space-letter));
            oldSpace = space;
          }
          else
          {
            tokenVector.push_back(currentLine.substr(letter, currentLine.length()-letter));
            oldSpace = space;
            break;
          }
        }
      }

      else if (sym != -1)
      {
        tokenVector.push_back(currentLine.substr(sym, 1));
        oldSpace = sym+1;
        if(sym == currentLine.length()-1)
          break;
      }

      else
        break;

    }
  }

  // creates the master vector of all the tokens
  void buildTokenVector()
  {
    tokenVector.erase(tokenVector.begin(), tokenVector.end());

    while (hasMoreTokens())
    {
      computeLine();
    }
  }

  int tokenTotal()
  {
    return tokenVector.size();
  }


  /*
  // returns the next token in the prebuilt tokenVector
  // will this get used?  i have no clue
  string getNextToken()
  {
    string temp = tokenVector[tokenCounter];        // creates temporary string from tokenVector
    tokenCounter++;                                 // increments tokenCounter
    return temp;
  }
  */

  void fileClose()
  {
    file.close();
  }

};



/*
  receives tokens as input and parses them
  decides what operation to operate next based on the token received

  prints to XML
*/
class Parser
{
public:
ofstream file;
vector<string> tokenVector;
int tokenCounter = 0;
int tabCounter = 0;

  Parser(string fileName, vector<string> &temp)
  {
    file.open(fileName);
    tokenVector = temp;
  }

  // returns correctly formatted token within tokenVector at location i
  string tokenize(int i)
  {
    string tempTokenType;
    string tempToken;

    tempTokenType = tokenType(i);

    tempToken = tokenVector[i];
    if (tempTokenType == "stringConstant")
        tempToken = tokenVector[i].substr(1, tokenVector[i].length()-2);
    else if (tempTokenType == "symbol")
    {
      if (tokenVector[i] == "<")
        tempToken = "&lt;";
      else if (tokenVector[i] == ">")
        tempToken = "&gt;";
      else if (tokenVector[i] == "&")
        tempToken = "&amp;";
    }

    return tempToken;
  }

  /*
    K = keyword
    S = symbol
    I = identifier
    N = int_constant
    W = string_constant

    returns the token type of the token within tokenVector at location i
  */
  string tokenType(int i)
  {
    string t = tokenVector[i];

    if (t == "class"        ||
        t == "method"       ||
        t == "function"     ||
        t == "constructor"  ||
        t == "int"          ||
        t == "boolean"      ||
        t == "char"         ||
        t == "void"         ||
        t == "var"          ||
        t == "static"       ||
        t == "field"        ||
        t == "let"          ||
        t == "do"           ||
        t == "if"           ||
        t == "else"         ||
        t == "while"        ||
        t == "return"       ||
        t == "true"         ||
        t == "false"        ||
        t == "null"         ||
        t == "this")
    {
      return "keyword";
    }
    else if ( t == "{" ||
              t == "}" ||
              t == "(" ||
              t == ")" ||
              t == "[" ||
              t == "]" ||
              t == "." ||
              t == "," ||
              t == ";" ||
              t == "+" ||
              t == "-" ||
              t == "*" ||
              t == "/" ||
              t == "&" ||
              t == "|" ||
              t == "<" ||
              t == ">" ||
              t == "=" ||
              t == "~")
    {
      return "symbol";
    }
    else if (t.find_first_not_of("0123456789") == -1)
    {
      return "integerConstant";
    }
    else if (t.find_first_of("\"") < t.find_last_of("\""))
    {
      return "stringConstant";
    }
    else
    {
      return "identifier";
    }
  }

  string tabs()
  {
    string tempTabs = "";

    for (int i=0; i<tabCounter; i++)
      tempTabs += "\t";

    return tempTabs;
  }

  void printXML()
  {
    file << tabs() << "<" << tokenType(tokenCounter) << "> ";
    file << tokenize(tokenCounter);
    file << " </" << tokenType(tokenCounter) << ">" << endl;
    tokenCounter++;
  }


  // creates the proper xml for the class compilation
  // class should be the first keyword right?
  void compileClass()
  {
    // class
    file << tabs() << "<class> " << endl;
    tabCounter++;

    // class name
    printXML();

    //identifier
    printXML();

    // {
    printXML();

    // compileClassVarDec()
    while (tokenize(tokenCounter) == "static" || tokenize(tokenCounter) == "field")
    {
      compileClassVarDec();
    }

    // subroutineDec()
    while ( tokenize(tokenCounter) == "constructor"  ||
            tokenize(tokenCounter) == "function"     ||
            tokenize(tokenCounter) == "method"       )
    {
      compileSubroutine();
    }

    // }
    printXML();

    tabCounter--;
    file << tabs() << "</class>" << endl;
  }

  void compileClassVarDec()
  {
    file << tabs() << "<classVarDec>" << endl;
    tabCounter++;


    // static or field
    printXML();

    // type
    printXML();

    // varName
    printXML();
    while (tokenize(tokenCounter) == ",")
    {
      // ,
      printXML();

      // varName
      printXML();
    }

    // ;
    printXML();

    tabCounter--;
    file << tabs() << "</classVarDec>" << endl;
  }

  void compileSubroutine()
  {
    file << tabs() << "<subroutineDec>" << endl;
    tabCounter++;

    // subroutine type
    printXML();

    // void or type
    printXML();

    // subroutine name
    printXML();

    // (
    printXML();

    // parameter list
    compileParameterList();

    // )
    printXML();

    /*
      SUBROUTINE BODY
    */
    file << tabs() << "<subroutineBody>" << endl;
    tabCounter++;

    // {
    printXML();

    if (tokenize(tokenCounter) != "}")
    {
      // varDec
      while (tokenize(tokenCounter) == "var")
        compileVarDec();

      // statements
      compileStatements();
    }

    // }
    printXML();

    // close subroutineBody
    tabCounter--;
    file << tabs() << "</subroutineBody>" << endl;

    // close subroutineDec
    tabCounter--;
    file << tabs() << "</subroutineDec>" << endl;

  }

  void compileParameterList()
  {
    // parameterList
    file << tabs() << "<parameterList>" << endl;
    tabCounter++;

    if (tokenize(tokenCounter) != ")")
    {
      // type
      printXML();

      // varName
      printXML();

      while (tokenize(tokenCounter) == ",")
      {
        // ,
        printXML();

        // type
        printXML();

        // varName
        printXML();
      }
    }

    // close parameterList
    tabCounter--;
    file << tabs() << "</parameterList>" << endl;

  }

  void compileVarDec()
  {
    // varDec
    file << tabs() << "<varDec>" << endl;
    tabCounter++;

    // var
    printXML();

    // type
    printXML();

    // varName
    printXML();
    while (tokenize(tokenCounter) == ",")
    {
      // ,
      printXML();

      // varName
      printXML();
    }

    // ;
    printXML();

    // close varDec
    tabCounter--;
    file << tabs() << "</varDec>" << endl;

  }

  void compileStatements()
  {
    // statements
    file << tabs() << "<statements>" << endl;
    tabCounter++;

    while(true)
    {
      if (tokenize(tokenCounter) == "let")
        compileLet();
      else if (tokenize(tokenCounter) == "if")
        compileIf();
      else if (tokenize(tokenCounter) == "while")
        compileWhile();
      else if (tokenize(tokenCounter) == "do")
        compileDo();
      else if (tokenize(tokenCounter) == "return")
        compileReturn();
      else
        break;
    }

    tabCounter--;
    file << tabs() << "</statements>" << endl;
  }

  void compileDo()
  {
    file << tabs() << "<doStatement>" << endl;
    tabCounter++;

    // do
    printXML();

    // subroutineCall
    if (tokenize(tokenCounter+1) == ".")
    {
      // className/varName
      printXML();

      // .
      printXML();
    }

    // subroutineName
    printXML();
    // (
    printXML();

    // expressionList
    compileExpressionList();

    // )
    printXML();

    // ;
    printXML();

    tabCounter--;
    file << tabs() << "</doStatement>" << endl;
  }

  void compileLet()
  {
    file << tabs() << "<letStatement>" << endl;
    tabCounter++;

    // let
    printXML();

    // varName
    printXML();
    while (tokenize(tokenCounter) == ",")
    {
      // ,
      printXML();
      // varName
      printXML();
    }

    // [
    if (tokenize(tokenCounter) == "[")
    {
      // [
      printXML();

      // expression
      compileExpression();
      // ]
      printXML();
    }

    // =
    printXML();

    // expression
    compileExpression();

    // ;
    printXML();

    tabCounter--;
    file << tabs() << "</letStatement>" << endl;
  }

  void compileWhile()
  {
    file << tabs() << "<whileStatement>" << endl;
    tabCounter++;

    // while
    printXML();

    // (
    printXML();

    // expression
    compileExpression();

    // )
    printXML();

    // {
    printXML();

    // statements
    compileStatements();

    // }
    printXML();

    tabCounter--;
    file << tabs() << "</whileStatement>" << endl;
  }

  void compileReturn()
  {
    file << tabs() << "<returnStatement>" << endl;
    tabCounter++;

    // return
    printXML();

    // expression
    if (tokenize(tokenCounter) != ";")
      compileExpression();

    // ;
    printXML();

    tabCounter--;
    file << tabs() << "</returnStatement>" << endl;
  }

  void compileIf()
  {
    file << tabs() << "<ifStatement>" << endl;
    tabCounter++;

    // if
    printXML();

    // (
    printXML();

    // expression
    compileExpression();

    // )
    printXML();

    // {
    printXML();

    // statements (should i check for } )
    compileStatements();

    // }
    printXML();

    if (tokenize(tokenCounter) == "else")
    {
      // else
      printXML();

      // {
      printXML();

      // statements
      compileStatements();

      // }
      printXML();
    }

    tabCounter--;
    file << tabs() << "</ifStatement>" << endl;
  }

  void compileExpression()
  {
    file << tabs() << "<expression>" << endl;
    tabCounter++;

    // term
    compileTerm();

    // op
    while ( tokenize(tokenCounter) == "+"     ||
            tokenize(tokenCounter) == "-"     ||
            tokenize(tokenCounter) == "*"     ||
            tokenize(tokenCounter) == "/"     ||
            tokenize(tokenCounter) == "&amp;" ||
            tokenize(tokenCounter) == "|"     ||
            tokenize(tokenCounter) == "&lt;"  ||
            tokenize(tokenCounter) == "&gt;"  ||
            tokenize(tokenCounter) == "="     )
    {
      printXML();

      //term
      compileTerm();
    }

    tabCounter--;
    file <<tabs() << "</expression>" << endl;
  }

  void compileTerm()
  {
    file << tabs() << "<term>" << endl;
    tabCounter++;

    // varname [expression]
    if (tokenize(tokenCounter+1) == "[")
    {
      //varname
      printXML();

      // [
      printXML();

      // expression
      compileExpression();

      // ]
      printXML();
    }
    else if (tokenType(tokenCounter) == "symbol")
    {
      // (expression)
      if (tokenize(tokenCounter) == "(")
      {
        // (
        printXML();

        // expression
        compileExpression();

        // )
        printXML();
      }
      // unaryOp term
      else if (tokenize(tokenCounter) == "-" || tokenize(tokenCounter) == "~")        // conflicts with op (-) ????
      {
        // unaryOp
        printXML();
        // term
        compileTerm();            // recursive, be careful
      }
    }
    else
    {
      // subroutineCall
      if (tokenize(tokenCounter+1) == "(")
      {
        // subroutineName
        printXML();

        // (
        printXML();

        // expressionList
        compileExpressionList();

        // )
        printXML();
      }
      else if (tokenize(tokenCounter+1) == ".")
      {
        // classname or varname
        printXML();

        // .
        printXML();

        // subroutineName
        printXML();

        // (
        printXML();

        // expressionList
        compileExpressionList();

        // )
        printXML();
      }
      else
      {
        // int const/string const/keyword const/varname
        printXML();
      }

    }

    tabCounter--;
    file << tabs() << "</term>" << endl;
  }

  void compileExpressionList()
  {
    file << tabs() << "<expressionList>" << endl;
    tabCounter++;

    if (tokenize(tokenCounter) != ")")
    {
      // expression
      compileExpression();

      while (tokenize(tokenCounter) == ",")
      {
        // ,
        printXML();

        // expresssion
        compileExpression();
      }
    }

    tabCounter--;
    file << tabs() << "</expressionList>" << endl;
  }

  ~Parser()
  {
    file.close();
  }


};


class SymbolTable
{
public:
  unordered_map<string, int> name;
  // should i use vectors here or find a way to do it with arrays
  vector<string> type;
  vector<string> kind;

  int index = 0;

  SymbolTable()
  {
    // what goes here
  }

  void startSubroutine()
  {
    name.erase();
    type.erase();
    kind.erase();
    index = 0;
  }

  void define(string tempName, string tempType, string tempKind)
  {
    name[tempName] = index;     // eg name["id"] = 0
    type[index] = tempType;     // eg type[0] = "int"
    kind[index] = tempKind;     // eg kind[0] = "static"

    index++;
  }

  int varCount(string tempKind)
  {
    int counter = 0;

    for (int i=0; i<index; i++)
    {
      if (kind[i] == tempKind)
        counter++;
    }

    return counter;
  }

  string kindOf(string tempName)
  {
    return kind[name[tempName]];
  }

  string typeOf(string tempName)
  {
    return type[name[tempName]];
  }

  int indexOf(string tempName)
  {
    return name[tempName];
  }

};

class VMWriter
{
public:
ofstream file;


  VMWriter(string fileName)
  {
    file.open(fileName);
  }

  void writePush()
  {

  }

  void writePop()
  {

  }

  void writeArithmetic()
  {

  }

  void writeLabel()
  {

  }

  void writeGoto()
  {

  }

  void writeIf()
  {

  }

  void writeCall()
  {

  }

  void writeFunction()
  {

  }

  void writeReturn()
  {

  }

  ~VMWriter()
  {
    file.close();
  }


};


int main()
{
  string jackFile;
  string xmlFile;
  cout << ".jack file:  ";
  cin >> jackFile;

  Tokenizer token(jackFile);

  for (int i=0; i<token.fileTotal(); i++)
  {
    token.fileOpen(i);

    // creates a new vector every time
    token.buildTokenVector();

    xmlFile = token.getOutFileName(i);

    Parser parse(xmlFile, token.tokenVector);

    /*
    code for testing (lists all tokens and their types)
    for (int j=0; j<token.tokenVector.size(); j++)
    {
      cout << "\n" << token.tokenType(j) << endl;
      cout << token.tokenize(j) << endl;
    }
    */

    // code for compiling  - 'class' should be the first keyword in each file
    parse.compileClass();

    token.fileClose();
  }


  cout << "Done" << endl;
  string end;                               // stalls so program doesn't just quit immediately
  cin >> end;

  return 0;
}
