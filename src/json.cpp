#include "json.h"

#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <squirrel/sqstdaux.h>

namespace
{
  std::string* _app_path = nullptr;

  void squ_print(HSQUIRRELVM v, const char* text, ...)
  {
    std::va_list list;
    va_start(list, text);

    std::vprintf(text, list);
    std::fflush(stdout);

    va_end(list);

    return;
  }
}

//static functions
void JSonParser::setPath(std::string* path)
{
  _app_path = path;
}

//parsing functions
int JSonParser::openFile(const char* filename)
{
  std::string path = "";
  std::FILE* file;
  char* buffer = nullptr;
  int size = 0;

  path += *_app_path;
  path += filename;

  file = fopen(path.c_str(), "rb");
  if(file == nullptr)
  {
    std::printf("Unable to open file: %s.\n", path.c_str());
    return 1;
  }

  std::fseek(file, 0, SEEK_END);
  size = (int)std::ftell(file);
  std::fseek(file, 0, SEEK_SET);
  size += 9;

  buffer = new char[size];
  std::strncpy(buffer, "return (", 8);
  std::fread(buffer + 8, 1, size, file);
  buffer[size - 1] = ')';

  sq_settop(_vm, 0);

  //squirrel stuff
  if(SQ_FAILED(sq_compilebuffer(_vm, buffer, size, filename, true)))
  {
    delete[] buffer;
    std::fclose(file);
    printf("Compilation of file \"%s\" failed.\n", path.c_str());
    std::fflush(stdout);
    return 2;
  }

  sq_pushroottable(_vm);
  sq_call(_vm, 1, true, true);

  std::fclose(file);
  delete[] buffer;

  std::fflush(stdout);

  return 0;
}

bool JSonParser::openTable(const char* table)
{
  sq_pushstring(_vm, table, -1);
  if(SQ_FAILED(sq_get(_vm, -2)))
    return false;

  if(sq_gettype(_vm, -1) != OT_TABLE)
  {
    sq_pop(_vm, 1);
    return false;
  }

  return true;
}

int JSonParser::openArray(const char* array)
{
  sq_pushstring(_vm, array, -1);
  if(SQ_FAILED(sq_get(_vm, -2)))
    return -1;

  if(sq_gettype(_vm, -1) != OT_ARRAY)
  {
    sq_pop(_vm, 1);
    return -1;
  }

  return sq_getsize(_vm, -1);
}

void JSonParser::back()
{
  sq_pop(_vm, 1);
}

void JSonParser::reset()
{
  sq_settop(_vm, 2);
}

///iteration
void JSonParser::beginIteration()
{
  sq_pushnull(_vm);
}

void JSonParser::continueIteration()
{
  sq_pop(_vm, 2);
}

bool JSonParser::skipIterations(int itrs)
{
  while(itrs > 0)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;
  }
  return true;
}

void JSonParser::endIteration()
{
  sq_pop(_vm, 1);
}

//iteration access
bool JSonParser::openNextTable(SQInteger* key)
{
  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_TABLE)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
      sq_getinteger(_vm, -2, key);

    break;
  }

  return true;
}

bool JSonParser::openNextArray(SQInteger* key)
{
  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_ARRAY)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
      sq_getinteger(_vm, -2, key);

    break;
  }

  return true;
}

bool JSonParser::getNextI(SQInteger* val, SQInteger* key)
{
  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_INTEGER)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
      sq_getinteger(_vm, -2, key);

    break;
  }

  sq_getinteger(_vm, -1, val);
  sq_pop(_vm, 2);

  return true;
}

bool JSonParser::getNextF(SQFloat* val, SQInteger* key)
{
  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_FLOAT)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
      sq_getinteger(_vm, -2, key);

    break;
  }

  sq_getfloat(_vm, -1, val);
  sq_pop(_vm, 2);

  return true;
}

bool JSonParser::getNextS(std::string* val, SQInteger* key)
{
  const char* str;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_STRING)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
      sq_getinteger(_vm, -2, key);

    break;
  }

  sq_getstring(_vm, -1, &str);
  *val = str;
  sq_pop(_vm, 2);

  return true;
}

bool JSonParser::openNextTable(std::string* key)
{
  const char* _key;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_TABLE)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
    {
      sq_getstring(_vm, -2, &_key);
      *key = _key;
    }

    break;
  }

  return true;
}

bool JSonParser::openNextArray(std::string* key)
{
  const char* _key;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_ARRAY)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
    {
      sq_getstring(_vm, -2, &_key);
      *key = _key;
    }

    break;
  }

  return true;
}

bool JSonParser::getNextI(SQInteger* val, std::string* key)
{
  const char* _key;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_INTEGER)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
    {
      sq_getstring(_vm, -2, &_key);
      *key = _key;
    }

    break;
  }

  sq_getinteger(_vm, -1, val);
  sq_pop(_vm, 2);

  return true;
}

bool JSonParser::getNextF(SQFloat* val, std::string* key)
{
  const char* _key;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_FLOAT)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
    {
      sq_getstring(_vm, -2, &_key);
      *key = _key;
    }

    break;
  }

  sq_getfloat(_vm, -1, val);
  sq_pop(_vm, 2);

  return true;
}

bool JSonParser::getNextS(std::string* val, std::string* key)
{
  const char* str;
  const char* _key;

  while(true)
  {
    if(SQ_FAILED(sq_next(_vm, -2)))
      return false;

    if(sq_gettype(_vm, -1) != OT_STRING)
    {
      sq_pop(_vm, 2);
      continue;
    }

    if(key != nullptr)
    {
      sq_getstring(_vm, -2, &_key);
      *key = _key;
    }

    break;
  }

  sq_getstring(_vm, -1, &str);
  *val = str;
  sq_pop(_vm, 2);

  return true;
}

///element access
SQInteger JSonParser::getElementI(const char* element)
{
  int top = sq_gettop(_vm);

  sq_pushstring(_vm, element, -1);
  if(SQ_FAILED(sq_get(_vm, -2)))
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return 0;
  }

  if(sq_gettype(_vm, -1) != OT_INTEGER)
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return 0;
  }

  SQInteger result = 0;

  sq_getinteger(_vm, -1, &result);

  sq_settop(_vm, top);

  return result;
}

SQFloat JSonParser::getElementF(const char* element)
{
  int top = sq_gettop(_vm);

  sq_pushstring(_vm, element, -1);
  if(SQ_FAILED(sq_get(_vm, -2)))
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return 0.0;
  }

  if(sq_gettype(_vm, -1) != OT_FLOAT)
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return 0.0;
  }

  SQFloat result = 0.0;

  sq_getfloat(_vm, -1, &result);

  sq_settop(_vm, top);

  return result;
}

std::string JSonParser::getElementS(const char* element)
{
  int top = sq_gettop(_vm);

  sq_pushstring(_vm, element, -1);
  if(SQ_FAILED(sq_get(_vm, -2)))
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return "";
  }

  const char* result;
  std::string ret;

  if(sq_gettype(_vm, -1) != OT_STRING)
  {
    if(_not_found == nullptr)
    {
      _not_found = new char[strlen(element) + 1];
      strcpy(_not_found, element);
    }
    return "";
  }

  sq_getstring(_vm, -1, &result);
  ret = result;

  sq_settop(_vm, top);

  return ret;
}

///error handling
bool JSonParser::getError()
{
  return _not_found != nullptr;
}

void JSonParser::clearError()
{
  delete[] _not_found;
  _not_found = nullptr;
}

const char* JSonParser::getNotFound()
{
  return _not_found;
}

///constructor/destructor
JSonParser::JSonParser()
{
  _vm = sq_open(1024);

  sqstd_seterrorhandlers(_vm);
  sq_setprintfunc(_vm, squ_print, squ_print);

  _not_found = nullptr;
}

JSonParser::~JSonParser()
{
  sq_close(_vm);

  delete[] _not_found;
}
