
#ifndef JSON_H_INCLUDED
#define JSON_H_INCLUDED

#include <string>

#include <squirrel/squirrel.h>

class JSonParser
{
  private:

  HSQUIRRELVM _vm;
  char* _not_found;
  //std::string _app_path;

  public:

  static void setPath(std::string*);

  int openFile(const char*);

  bool openTable(const char*);
  int openArray(const char*);
  void back();
  void reset();

  void beginIteration();
  void continueIteration();
  bool skipIterations(int);
  void endIteration();

  bool openNextTable(std::string* = nullptr);
  bool openNextArray(std::string* = nullptr);
  bool getNextI(SQInteger*, std::string* = nullptr);
  bool getNextF(SQFloat*, std::string* = nullptr);
  bool getNextS(std::string*, std::string* = nullptr);

  bool openNextTable(SQInteger*);
  bool openNextArray(SQInteger*);
  bool getNextI(SQInteger*, SQInteger*);
  bool getNextF(SQFloat*, SQInteger*);
  bool getNextS(std::string*, SQInteger*);

  SQInteger getElementI(const char*);
  SQFloat getElementF(const char*);
  std::string getElementS(const char*);

  const char* getNotFound();
  bool getError();
  void clearError();

  JSonParser();
  ~JSonParser();
};

#endif
