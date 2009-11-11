#include "../nrlib/iotools/fileio.hpp"
#include "../nrlib/tinyxml/tinyxml.h"

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

std::string 
FindPreceedingString(const std::string & file, std::string::size_type end)
{
  std::string::size_type start = end-1;
  while(start >= 0 && file[start] != ' ')
    start--;
  
  std::string name = file.substr(start+1,end-start);
  return(name);
}

void
CheckInputFiles(std::string & errTxt)
{
  std::ifstream infile;
  NRLib::OpenRead(infile,"src/inputfiles.cpp");
  std::string file1;
  std::string line;
  while(!infile.eof()) {
    getline(infile, line);
    file1 = file1+"\n"+line;
  }
  infile.close();

  infile.clear();
  std::string file2;
  NRLib::OpenRead(infile,"src/inputfiles.h");
  while(!infile.eof()) {
    getline(infile, line);
    file2 = file2+"\n"+line;
  }
  infile.close();

  std::string::size_type comStart = file1.find("addPathAndCheck");
  std::string::size_type comEnd   = file1.find("InputFiles::", comStart);
    
  std::string::size_type base = file2.find("private:");
  std::string::size_type end  = file2.find("_", base);
  
  while(end < file2.size()) {
    std::string variable = FindPreceedingString(file2, end);
    if(variable != "inputDirectory_") {
      if(file1.find(variable, comStart) > comEnd)
        errTxt = errTxt + "Error: Class variable "+variable+" is not checked for being an existing file in inputfiles.cpp\n";
    }
    end  = file2.find("_", end+1);
  }
}


std::string
GetPath(const TiXmlNode * node)
{
  std::string tree;
  if(node->ValueStr() != "crava") {
    tree = GetPath(node->Parent());
    tree = tree +"<"+node->ValueStr()+">";
  }
  return(tree);
}


TiXmlElement *
ProcessCodeLevel(const std::string & file, const std::string & command, std::string & errTxt)
{
  std::string::size_type init  = file.find(command);
  std::string::size_type term  = file.find("checkForJunk",init);
  std::string::size_type start = file.find("\"", init);
  std::string::size_type end   = file.find("\"", start+1);
  std::string name = file.substr(start+1,end-start-1);
  TiXmlElement * node = new TiXmlElement(name);
  TiXmlElement * child;

  std::string::size_type lStart = file.find("legalCommands", end+1);
  start = file.find("parse",end);
  std::vector<std::string> cList;
  while(lStart < start) {
    lStart = file.find("\"", lStart);
    end = file.find("\"", lStart+1);
    name = file.substr(lStart+1,end-lStart-1);
    cList.push_back(name);
    lStart = file.find("legalCommands", end+1);
  }

  bool stop = true;

  while(start < term) {
    end = file.find("(",start);
    name = file.substr(start,end-start+1);
    if(name == "parseValue(" || name == "parseFileName(" || name == "parseBool(" ||
       name == "parseVariogram(" || name == "parseTraceHeaderFormat(") {
      start = file.find("\"",end);
      end   = file.find("\"",start+1);
      name  = file.substr(start+1,end-start-1);
      child = new TiXmlElement(name);
    }
    else
      child = ProcessCodeLevel(file, "::"+name, errTxt);

    if(node->FirstChildElement(child->ValueStr()) != NULL)
      errTxt = errTxt+"Error: Command <"+node->ValueStr()+"><"+child->ValueStr()+"> is implemented twice.\n";
    else {
      node->LinkEndChild(child);
      std::vector<std::string>::iterator pos = std::find(cList.begin(), cList.end(),child->ValueStr());
      if(pos == cList.end())
        errTxt = errTxt+"Error: Command <"+node->ValueStr()+"><"+child->ValueStr()+"> is implemented, but not listed as legal.\n";
      else
        cList.erase(pos);
    }
    start = file.find("parse",end);
  }
  
  for(unsigned int i=0;i<cList.size();i++)
    errTxt = errTxt+"Error: Command <"+cList[i]+"> is listed as legal, but not implemented.\n";

  return(node);
}

TiXmlElement *
ProcessDocLevel(const std::string & file, int level, const std::vector<std::string> & commands, 
                std::string::size_type first, std::string::size_type last)
{
  std::string::size_type init  = file.find(commands[level],first);
  std::string::size_type term  = file.find(commands[level],init+1);
  if(init > last)
    return(NULL);
  if(term > last)
    term = last;

  std::string::size_type start;
  std::string::size_type end;

  TiXmlElement * node;
  if(level == 0)
    node = new TiXmlElement("crava");

  else {
    start = file.find("newkw", init);  //Jump to newkw.
    start = file.find("{", start);      //Jump to position before command name.
    end   = file.find("}", start+1);
    std::string name = file.substr(start+1,end-start-1);
    if(name == "variogram-keyword")
      return(NULL);
    node = new TiXmlElement(name);
  }

  if(level < 6) {
    start = file.find(commands[level+1], init);  //Jump to newkw.
    while(start < term) {
      TiXmlElement * child = ProcessDocLevel(file, level+1, commands, start, term);
      if(child != NULL)
        node->LinkEndChild(child);
      start = file.find(commands[level+1], start+1); 
    }
  }
  return(node);
}

void
CompareBranches(const TiXmlNode * cRoot, const TiXmlNode * dRoot, std::string & errTxt)
{
  if(cRoot->ValueStr() == "segy-format") //Subcommands herre not caught by code parser.
    return;

  const TiXmlNode * cNode = cRoot->FirstChildElement();
  const TiXmlNode * dNode;
  while(cNode != NULL) {
    dNode = dRoot->FirstChildElement(cNode->ValueStr());
    if(dNode != NULL)
      CompareBranches(cNode, dNode, errTxt);
    else
      errTxt = errTxt+"Error: Keyword "+GetPath(cNode)+" in code is not documented.\n";
    cNode = cNode->NextSibling();
  }

  dNode = dRoot->FirstChildElement();
  while(dNode != NULL) {
    cNode = cRoot->FirstChildElement(dNode->ValueStr());
    if(cNode == NULL) //No need to compare here, done above.
      errTxt = errTxt+"Error: Keyword "+GetPath(dNode)+" in documentation is not implemented.\n";
    dNode = dNode->NextSibling();
  }
}



bool CompareTrees(const TiXmlDocument & code, const TiXmlDocument & doc, std::string & errTxt)
{
  const TiXmlNode * dNode = doc.FirstChildElement();
  const TiXmlNode * cNode = code.FirstChildElement();
  if(dNode->ValueStr() != "crava")
    errTxt = "Error: Documentation tree corrupt.\n";
  if(cNode->ValueStr() != "crava")
    errTxt = "Error: Code tree corrupt.\n";
  if(errTxt == "")
    CompareBranches(dNode, cNode, errTxt);

  return(errTxt == "");
}

int main()
{
  std::string errTxt;
  CheckInputFiles(errTxt);
  if(errTxt == "")
    std::cout << "All files are checked for existence.\n";
  
  std::ifstream infile;
  NRLib::OpenRead(infile,"doc/user_manual/referencemanual.tex");
  std::string file;
  std::string line;
  while(!infile.eof()) {
    getline(infile, line);
    file = file+"\n"+line;
  }
  infile.close();
  std::vector<std::string> secTab(7);
  secTab[0] = "\\chapter";
  secTab[1] = "\\section";
  secTab[2] = "\\subsection";
  secTab[3] = "\\subsubsection";
  secTab[4] = "\\paragraph";
  secTab[5] = "\\subparagraph";
  secTab[6] = "\\subsubparagraph";
  TiXmlElement * root = ProcessDocLevel(file,0,secTab,0,file.size()+1);
  TiXmlDocument doc;
  doc.LinkEndChild(root);
  doc.SaveFile("docgrammar.txt");

  infile.clear();
  NRLib::OpenRead(infile,"src/xmlmodelfile.cpp");
  file = "";
  while(!infile.eof()) {
    getline(infile, line);
    file = file+"\n"+line;
  }
  infile.close();

  root = ProcessCodeLevel(file, "::parseCrava(", errTxt);
  std::ofstream outfile;
  if(errTxt.size() > 0) {
    NRLib::OpenWrite(outfile, "legalerrors.txt");
    outfile << errTxt;
    outfile.close();
    std::cout << errTxt;
  }
  else
    std::cout << "Listing of legal commands is consistent with code.\n";

  TiXmlDocument code;
  code.LinkEndChild(root);
  code.SaveFile("codegrammar.txt");
  errTxt = "";
  if(CompareTrees(doc, code, errTxt) == false) {
    std::ofstream outfile("Grammarerrors.txt");
    outfile << errTxt;
    outfile.close();
    std::cout << errTxt;
  }
  else
    std::cout << "Code and documentation are consistent.\n";
}


