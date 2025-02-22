#ifndef VM_H_
#define VM_H_
#include "plutonium.h"
typedef PltObject *(*Nativefunc)(PltObject *, int, PltObject *);
using namespace std;
struct ByteSrc
{
  short fileIndex; // index of filename in files vector
  size_t ln;
}; // source of byte i.e it's filename and line number

inline bool isNumeric(char t)
{
  if (t == 'i' || t == 'f' || t == 'l')
    return true;
  return false;
}
inline void PromoteType(PltObject &a, char t)
{
  if (a.type == 'i')
  {
    if (t == 'l') // promote to long long int
    {
      a.type = 'l';
      a.l = (long long int)a.i;
    }
    else if (t == 'f')
    {
      a.type = 'f';
      a.f = (double)a.i;
    }
  }
  else if (a.type == 'f')
  {
    if (t == 'l') // promote to long long int
    {
      a.type = 'l';
      a.l = (long long int)a.f;
    }
    else if (t == 'i')
    {
      a.type = 'i';
      a.f = (int)a.f;
    }
    else if (t == 'f')
      return;
  }
  else if (a.type == 'l')
  {
    if (t == 'f') // only this one is needed
    {
      a.type = 'f';
      a.f = (double)a.l;
    }
  }
}
string fullform(char t)
{
  if (t == 'i')
    return "Integer 32 bit";
  else if (t == 'l')
    return "Integer 64 bit";
  else if (t == 'f')
    return "Float";
  else if (t == 's')
    return "String";
  else if (t == 'j')
    return "List";
  else if (t == 'm')
    return "Byte";
  else if (t == 'b')
    return "Boolean";
  else if (t == 'u')
    return "File Stream";
  else if (t == 'a')
    return "Dictionary";
  else if (t == 'q')
    return "Module";
  else if (t == 'w')
    return "Function";
  else if (t == 'v')
    return "Class";
  else if (t == 'o')
    return "Class Instance";
  else if (t == 'n')
    return "nil";
  else if (t == 'y')
    return "Native Function";
  else if (t == PLT_POINTER)
    return "Native Pointer";
  else if (t == 'e')
    return "Error Object";
  else if (t == 'g')
    return "Coroutine";
  else if (t == 'z')
    return "Coroutine Object";
  else
    return "Unknown";
}

struct MemInfo
{
  char type;
  bool isMarked;
};

const char *ErrNames[] = {"TypeError", "ValueError", "MathError", "NameError", "IndexError", "ArgumentError", "UnknownError", "FileIOError", "KeyError", "OverflowError", "FileOpenError", "FileSeekError", "ImportError", "ThrowError", "MaxRecursionError", "AccessError"};
// Allocator functions
// these are visible to loaded native modules
ErrObject *allocErrObject();
Klass *allocKlass();
KlassInstance *allocKlassInstance();
PltList *allocList();
Dictionary *allocDict();
string *allocString();
FunObject *allocFunObject();
Coroutine *allocCoroutine();
FileObject *allocFileObject();
Module *allocModule();
NativeFunction *allocNativeFun();
extern bool REPL_MODE;
void REPL();
class VM
{
private:
  vector<unsigned char *> callstack;
  unsigned char *program = NULL;
  unsigned int program_size;
  // Some registers
  int i1;
  int i2;
  int i3;
  long long int l1;
  double f1;
  unsigned char m1;
  PltObject p1;
  PltObject p2;
  PltObject p3;
  PltObject p4;
  string s1;
  string s2;
  char c1;
  char c2;
  PltList pl1;      // plutonium list 1
  PltList *pl_ptr1; // plutonium list pointer 1
  Dictionary pd1;
  Dictionary *pd_ptr1;
  unsigned char *k;
  vector<size_t> tryStackCleanup;
  vector<int> frames = {0}; // stores starting stack indexes of all stack frames
  size_t orgk = 0;
  vector<unsigned char *> except_targ;
  vector<size_t> tryLimitCleanup;
  // int Gc_Cycles = 0;
#ifdef BUILD_FOR_WINDOWS
  vector<HINSTANCE> moduleHandles;
#endif // BUILD_FOR_WINDOWS
#ifdef BUILD_FOR_LINUX
  vector<void *> moduleHandles;
#endif
  vector<FunObject *> executing = {NULL}; // pointer to plutonium function object we are executing,NULL means control is not in a function
  
  vector<BuiltinFunc> builtin; // addresses of builtin native functions
  // referenced in bytecode
  string magicName = ".destroy";
  size_t GC_THRESHHOLD;
  std::unordered_map<size_t, ByteSrc> *LineNumberTable;
  vector<string> *files;
  vector<string> *sources;
  PltList aux; // auxiliary space for markV2
  vector<PltObject> STACK;
public:
  friend class Compiler;
  std::unordered_map<void *, MemInfo> memory;
  size_t allocated = 0;
  size_t GC_Cycles = 0;
  vector<string> strings; // string constants used in bytecode
  PltObject *constants = NULL;
  int total_constants = 0; // total constants stored in the array constants
  
  void load(vector<unsigned char> b, std::unordered_map<size_t, ByteSrc> *ltable, vector<string> *a, vector<string> *c)
  {
    if (program)
      delete[] program;
    program = new unsigned char[b.size()];
    for (size_t k = 0; k < b.size(); k += 1)
    {
      program[k] = b[k];
    }
    program_size = b.size();
    GC_THRESHHOLD = 4196;
    LineNumberTable = ltable;
    files = a;
    sources = c;
  }
  void spitErr(ErrCode e, string msg) // used to show a runtime error
  {
    if (except_targ.size() != 0)
    {
      size_t T = STACK.size() - tryStackCleanup.back();
      STACK.erase(STACK.end() - T, STACK.end());
      ErrObject *E = allocErrObject();
      E->code = (int)e;
      E->des = msg;
      p1.type = 'e';
      p1.ptr = (void *)E;
      STACK.push_back(p1);
      T = frames.size() - tryLimitCleanup.back();
      frames.erase(frames.end() - T, frames.end());
      k = except_targ.back();
      except_targ.pop_back();
      tryStackCleanup.pop_back();
      tryLimitCleanup.pop_back();
      return;
    }
    size_t line_num = (*LineNumberTable)[orgk].ln;
    string &filename = (*files)[(*LineNumberTable)[orgk].fileIndex];
    string type = "UnknownError";
    if (e >= 1 && e <= 16)
      type = ErrNames[(int)e - 1];
    printf("\nFile %s\n", filename.c_str());
    printf("%s at line %ld\n", type.c_str(), line_num);
    auto it = std::find(files->begin(), files->end(), filename);
    size_t i = it - files->begin();
    string source_code = (*sources)[i];
    size_t l = 1;
    string line = "";
    size_t k = 0;

    while (l <= line_num)
    {
      if (source_code[k] == '\n')
        l += 1;
      else if (l == line_num)
        line += source_code[k];
      k += 1;
      if (k >= source_code.length())
        break;
    }
    printf("%s\n", lstrip(line).c_str());
    printf("%s\n", msg.c_str());
    if (callstack.size() != 0 && e != MAX_RECURSION_ERROR) // printing stack trace for max recursion is stupid
    {
      printf("<stack-trace>\n");
      while (callstack.size() != 0) // print stack trace
      {
        size_t L = callstack.back() - program;
        L -= 1;
        while (LineNumberTable->find(L) == LineNumberTable->end())
          L -= 1; // L is now the index of CALLUDF opcode now
        // which is ofcourse present in the LineNumberTable
        printf("  --by %s line %ld\n", (*files)[(*LineNumberTable)[L].fileIndex].c_str(), (*LineNumberTable)[L].ln);
        callstack.pop_back();
      }
    }
    if (REPL_MODE)
      REPL(); // start another REPL and exit after it finishes
    exit(0);
  }
  inline void DoThreshholdBusiness()
  {
    if (allocated > GC_THRESHHOLD)
    {
      mark();
      collectGarbage();
    }
  }

  inline bool isHeapObj(const PltObject &obj)
  {
    if (obj.type != 'i' && obj.type != 'l' && obj.type != 'f' && obj.type != 'n' && obj.type != 'm' && obj.type != 'p' && obj.type != 'b')
      return true; // all other objects are on heap
    return false;
  }
  void markV2(const PltObject &obj)
  {
    aux.clear();
    std::unordered_map<void *, MemInfo>::iterator it;
    if (isHeapObj(obj) && (it = memory.find(obj.ptr)) != memory.end() && !(it->second.isMarked))
    {
      // mark object alive and push it
      it->second.isMarked = true;
      aux.push_back(obj);
    }
    // if an object is on aux then
    //  - it is a heap object
    //  - it was unmarked but marked before pushing
    //  - known to VM's memory pool i.e not from outside (any native module or something)

    // After following above constraints,aux will not have any duplicates or already marked objects
    PltObject curr;
    while (aux.size() != 0)
    {
      curr = aux.back();
      aux.pop_back();
      // for each curr object we get ,it was already marked alive before pushing onto aux
      // so we only process objects referenced by it and not curr itself
      if (curr.type == 'a')
      {
        Dictionary &d = *(Dictionary *)curr.ptr;
        for (auto e : d)
        {
          // if e.first or e.second is a heap object and known to our memory pool
          // add it to aux
          if (isHeapObj(e.first) && (it = memory.find(e.first.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.first);
          }
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
      }
      else if (curr.type == 'j')
      {
        PltList &d = *(PltList *)curr.ptr;
        for (const auto &e : d)
        {
          if (isHeapObj(e) && (it = memory.find(e.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e);
          }
        }
      }
      else if (curr.type == 'z') // coroutine object
      {
        Coroutine *g = (Coroutine *)curr.ptr;
        for (auto &e : g->locals)
        {
          if (isHeapObj(e) && (it = memory.find(e.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e);
          }
        }
        FunObject *gf = g->fun;
        it = memory.find((void *)gf);
        if (it != memory.end() && !(it->second.isMarked))
        {
          PltObject tmp;
          tmp.type = PLT_FUNC;
          tmp.ptr = (void *)gf;
          it->second.isMarked = true;
          aux.push_back(tmp);
        }
      }
      else if (curr.type == 'q')
      {
        Module *k = (Module *)curr.ptr;
        for (const auto &e : k->members)
        {
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
      }
      else if (curr.type == 'v')
      {
        Klass *k = (Klass *)curr.ptr;
        for (auto e : k->members)
        {
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
        for (auto e : k->privateMembers)
        {
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
      }
      else if (curr.type == 'y')
      {
        NativeFunction *fn = (NativeFunction *)curr.ptr;
        it = memory.find((void *)fn->klass);
        if (it != memory.end() && !(it->second.isMarked))
        {
          PltObject tmp;
          tmp.type = PLT_CLASS;
          tmp.ptr = (void *)fn->klass;
          it->second.isMarked = true;
          aux.push_back(tmp);
        }
      }
      else if (curr.type == 'o')
      {
        KlassInstance *k = (KlassInstance *)curr.ptr;
        Klass *kk = k->klass;
        it = memory.find((void *)kk);
        if (it != memory.end() && !(it->second.isMarked))
        {
          PltObject tmp;
          tmp.type = PLT_CLASS;
          tmp.ptr = (void *)kk;
          it->second.isMarked = true;
          aux.push_back(tmp);
        }
        for (auto e : k->members)
        {
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
        for (auto e : k->privateMembers)
        {
          if (isHeapObj(e.second) && (it = memory.find(e.second.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e.second);
          }
        }
      }
      else if (curr.type == PLT_FUNC)
      {
        Klass *k = ((FunObject *)curr.ptr)->klass;
        it = memory.find((void *)k);
        if (it != memory.end() && !(it->second.isMarked))
        {
          PltObject tmp;
          tmp.type = PLT_CLASS;
          tmp.ptr = (void *)k;
          it->second.isMarked = true;
          aux.push_back(tmp);
        }
        for (auto e : ((FunObject *)curr.ptr)->opt)
        {
          if (isHeapObj(e) && (it = memory.find(e.ptr)) != memory.end() && !(it->second.isMarked))
          {
            it->second.isMarked = true;
            aux.push_back(e);
          }
        }
      }
    } // end while loop
  }
  void mark()
  {
    for (auto e : STACK)
      markV2(e);
  }

  void collectGarbage()
  {
    size_t pre = allocated;
    vector<void *> toFree;
    for (auto e : memory)
    {
      MemInfo m = e.second;
      if (m.isMarked)
        memory[e.first].isMarked = false;
      else
      {
        toFree.push_back(e.first);
      }
    }

    for (auto e : toFree)
    {
      MemInfo m = memory[e];
      // if memory being deleted is an object
      // call native destructor
      if (m.type == 'o')
      {
        KlassInstance *obj = (KlassInstance *)e;
        PltObject dummy;
        dummy.type = 'o';
        dummy.ptr = e;
        if (obj->members.find(".destroy") != obj->members.end())
        {
          NativeFunction *p = (NativeFunction *)obj->members[".destroy"].ptr;
          PltObject rr;
          NativeFunPtr f = (NativeFunPtr)p->addr;
          f(&dummy, 1, &rr);
        }
      }

      if (m.type == 'j')
      {
        delete (PltList *)e;
        allocated -= sizeof(PltList);
      }
      else if (m.type == 'q')
      {
        delete (Module *)e;
        allocated -= sizeof(Module);
      }
      else if (m.type == 'v')
      {
        delete (Klass *)e;
        allocated -= sizeof(Klass);
      }
      else if (m.type == 'o')
      {
        delete (KlassInstance *)e;
        allocated -= sizeof(KlassInstance);
      }
      else if (m.type == 'a')
      {
        delete (Dictionary *)e;
        allocated -= sizeof(Dictionary);
      }
      else if (m.type == 'u')
      {
        delete (FileObject *)e;
        allocated -= sizeof(FileObject);
      }
      else if (m.type == 'w')
      {
        delete (FunObject *)e;
        allocated -= sizeof(FunObject);
      }
      else if (m.type == 'z')
      {
        delete (Coroutine *)e;
        allocated -= sizeof(Coroutine);
      }
      else if (m.type == 'e')
      {
        delete (ErrObject *)e;
        allocated -= sizeof(ErrObject);
      }
      else if (m.type == 's')
      {
        delete (string *)e;
        allocated -= sizeof(string);
      }
      else if (m.type == 'y')
      {
        delete (NativeFunction *)e;
        allocated -= sizeof(NativeFunction);
      }
      memory.erase(e);
    }
    size_t recycled = pre - allocated;
    // printf("recycled %lld bytes\n",recycled);
    if (recycled < 4196) // a gc cycle is expensive so running it to collect not even 4196 bytes is useless
    {
      // if this is the case we update the threshhold so that next time we collect more bytes
      // the number 4196 was picked randomly and it seems to work well
      GC_THRESHHOLD *= 2;
    }
  }
  inline bool invokeOperator(string meth, PltObject A, size_t args, string op, PltObject *rhs = NULL, bool raiseErrOnNF = true) // check if the object has the specified operator overloaded and prepares to call it by updating callstack and frames
  {
    KlassInstance *obj = (KlassInstance *)A.ptr;
    auto it = obj->members.find(meth);
    if (it != obj->members.end())
    {
      p3 = it->second;
      if (p3.type == 'w')
      {
        FunObject *fn = (FunObject *)p3.ptr;
        if (fn->opt.size() != 0)
          spitErr(ARGUMENT_ERROR, "Optional parameters not allowed for operator functions!");
        if (fn->args == args)
        {
          callstack.push_back(k + 1);
          if (callstack.size() >= 1000)
          {
            spitErr(MAX_RECURSION_ERROR, "Error max recursion limit 1000 reached.");
            return false;
          }
          executing.push_back(fn);
          frames.push_back(STACK.size());
          if (rhs != NULL)
            STACK.push_back(*rhs);
          STACK.push_back(A);
          k = program + fn->i;
          return true;
        }
      }
      else if (p3.type == PLT_NATIVE_FUNC)
      {
        NativeFunction *fn = (NativeFunction *)p3.ptr;
        NativeFunPtr M = fn->addr;
        PltObject rr;
        PltObject nil;
        PltObject argArr[2] = {A, (!rhs) ? nil : *rhs};
        M(argArr, args, &rr);
        if (rr.type == 'e')
        {
          ErrObject *E = (ErrObject *)rr.ptr;
          int eCode = E->code;
          s1 = meth + "():  " + E->des;
          spitErr((ErrCode)eCode, s1);
          return false;
        }
        STACK.push_back(rr);
        k++;
        return true;
      }
    }
    if (!raiseErrOnNF)
      return false;
    if (args == 2)
      spitErr(TYPE_ERROR, "Error operator '" + op + "' unsupported for types " + fullform(A.type) + " and " + fullform(rhs->type));
    else
      spitErr(TYPE_ERROR, "Error operator '" + op + "' unsupported for type " + fullform(A.type));
    return false;
  }
  void interpret(size_t offset = 0, bool panic = true) // panic if stack is not empty when finished
  {
    allocFuncions allocators;
    allocators.a1 = &allocList;
    allocators.a2 = &allocDict;
    allocators.a3 = &allocString;
    allocators.a4 = &allocErrObject;
    allocators.a5 = &allocFileObject;
    allocators.a6 = &allocKlass;
    allocators.a7 = &allocKlassInstance;
    allocators.a8 = &allocNativeFun;
    allocators.a9 = &allocModule;

    api_setup(&allocators);//for native modules
    srand(time(0));
    k = program + offset;

    unsigned char inst;
    while (*k != OP_EXIT)
    {
      inst = *k;
      switch (inst)
      {
      case LOAD_GLOBAL:
      {
        k += 1;
        memcpy(&i1, k, 4);
        STACK.push_back(STACK[i1]);
        k += 3;
        break;
      }
      case INC_GLOBAL:
      {
        orgk = k - program;
        k += 1;
        memcpy(&i1, k, 4);
        k += 3;
        c1 = STACK[i1].type;
        ;
        if (c1 == 'i')
        {
          if (STACK[i1].i == INT_MAX)
          {
            STACK[i1].l = (long long int)INT_MAX + 1;
            STACK[i1].type = 'l';
          }
          else
            STACK[i1].i += 1;
        }
        else if (c1 == 'l')
        {
          if (STACK[i1].l == LLONG_MAX)
          {
            spitErr(OVERFLOW_ERROR, "Error numeric overflow");
            continue;
          }
          STACK[i1].l += 1;
        }
        else if (c1 == 'f')
        {
          if (STACK[i1].f == FLT_MAX)
          {
            spitErr(OVERFLOW_ERROR, "Error numeric overflow");
            continue;
          }
          STACK[i1].f += 1;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error cannot add numeric constant to type " + fullform(c1));
          continue;
        }
        break;
      }
      case LOAD:
      {
        orgk = k - program;
        k += 1;
        c1 = *k;
        k += 1;
        if (c1 == 'j')
        {
          int listSize;
          memcpy(&listSize, k, sizeof(int));
          k += 3;
          PltObject a;
          PltList *p = allocList();
          *p = {STACK.end() - listSize, STACK.end()};
          STACK.erase(STACK.end() - listSize, STACK.end());
          a.type = 'j';
          a.ptr = (void *)p;
          STACK.push_back(a);
          DoThreshholdBusiness();
        }
        else if (c1 == 'a')
        {
          int DictSize;
          memcpy(&DictSize, k, sizeof(int));
          k += 3;
          Dictionary dict;
          while (DictSize != 0)
          {
            PltObject val = STACK[STACK.size() - 1];
            STACK.pop_back();
            PltObject key = STACK[STACK.size() - 1];
            STACK.pop_back();
            if(key.type!='i' && key.type!='l' && key.type!='f' && key.type!='s' && key.type!='m' && key.type!='b')
            {
              spitErr(TYPE_ERROR,"Error key of type "+fullform(key.type)+" not allowed.");
              continue;
            }
            if (dict.find(key) != dict.end())
            {
              spitErr(VALUE_ERROR, "Error duplicate keys in dictionary!");
              continue;
            }
            dict.emplace(key, val);
            DictSize -= 1;
          }
          PltObject a;
          Dictionary *p = allocDict();

          *p = dict;
          a.ptr = (void *)p;
          a.type = 'a';
          STACK.push_back(a);
          DoThreshholdBusiness();
        }
        else if (c1 == 'v')
        {
          memcpy(&i1, k, sizeof(int));
          k += 3;
          STACK.push_back(STACK[frames.back() + i1]);
        }

        break;
      }
      case LOAD_CONST:
      {
        k += 1;
        memcpy(&i1, k, 4);
        k += 3;
        STACK.push_back(constants[i1]);
        break;
      }
      case ASSIGN:
      {
        k += 1;
        memcpy(&i1, k, 4);
        k += 3;
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        STACK[frames.back() + i1] = p1;
        break;
      }
      case ASSIGN_GLOBAL:
      {
        k += 1;
        memcpy(&i1, k, 4);
        k += 3;
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        STACK[i1] = p1;
        break;
      }
      case DUP:
      {
        STACK.push_back(STACK.back());
        break;
      }
      case ASSIGNINDEX:
      {
        orgk = k - program;
        p1 = STACK.back(); // the index
        STACK.pop_back();
        p2 = STACK.back(); // the val i.e rhs
        STACK.pop_back();
        p3 = STACK.back();
        STACK.pop_back();
        if (p3.type == 'j')
        {
          pl_ptr1 = (PltList *)p3.ptr;
          long long int idx = 0;
          if (p1.type == 'i')
            idx = p1.i;
          else if (p1.type == 'l')
            idx = p1.l;
          else
          {
            spitErr(TYPE_ERROR, "Error type " + fullform(p1.type) + " cannot be used to index list!");
            continue;
          }
          if (idx < 0 || idx > (long long int)pl_ptr1->size())
          {
            spitErr(VALUE_ERROR, "Error index " + PltObjectToStr(p1) + " out of range for list of size " + to_string(pl_ptr1->size()));
            continue;
          }
          (*pl_ptr1)[idx] = p2;
        }
        else if (p3.type == 'a')
        {
          pd_ptr1 = (Dictionary *)p3.ptr;
          if (pd_ptr1->find(p1) == pd_ptr1->end())
          {
            spitErr(VALUE_ERROR, "Error given key is not present in dictionary!");
            continue;
          }
          (*pd_ptr1)[p1] = p2;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error indexed assignment unsupported for type " + fullform(p3.type));
          continue;
        }

        break;
      }
      case CALL:
      {
        orgk = k - program;
        k += 1;
        memcpy(&i1, k, 4);
        k += 4;
        int howmany = *k;
        builtin[i1](&STACK[STACK.size() - howmany], howmany);
        if (p1.type == 'e')
        {
          ErrObject *E = (ErrObject *)p1.ptr;
          spitErr((ErrCode)E->code, E->des);
          continue;
        }
        STACK.erase(STACK.end() - howmany, STACK.end());
        break;
      }
      case CALLFORVAL:
      {
        orgk = k - program;
        k += 1;
        memcpy(&i1, k, 4);
        k += 4;
        i2 = *k;
        p1 = builtin[i1](&STACK[STACK.size() - i2], i2);
        if (p1.type == 'e')
        {
          ErrObject *E = (ErrObject *)p1.ptr;
          spitErr((ErrCode)E->code, E->des);
          continue;
        }
        STACK.erase(STACK.end() - i2, STACK.end());
        STACK.push_back(p1);
        break;
      }
      case CALLMETHOD:
      {
        orgk = k - program;
        k++;
        memcpy(&i1, k, 4);
        k += 4;
        string method_name = strings[i1];
        i2 = *k;
        pl1 = {STACK.end() - i2, STACK.end()}; // args
        STACK.erase(STACK.end() - i2, STACK.end());
        p1 = STACK.back(); // Parent object from which member is being called
        STACK.pop_back();
        if (p1.type == 'q')
        {
          Module *m = (Module *)p1.ptr;
          if (m->members.find(method_name) == m->members.end())
          {
            spitErr(NAME_ERROR, "Error the module has no member " + method_name + "!");
            continue;
          }
          p3 = m->members[method_name];
          if (p3.type == 'y')
          {
            NativeFunction *fn = (NativeFunction *)p3.ptr;
            NativeFunPtr f = fn->addr;
            PltObject *argArr = NULL;
            if (pl1.size() != 0)
              argArr = &pl1[0];
            p4.type = 'n';
            f(argArr, i2, &p4);

            if (p4.type == 'e')
            {
              // The module raised an error
              ErrObject *E = (ErrObject *)p4.ptr;
              int eCode = E->code;
              s1 = method_name + "():  " + E->des;
              spitErr((ErrCode)eCode, s1);
              continue;
            }
            if (fullform(p4.type) == "Unknown" && p4.type != 'n')
            {
              spitErr(VALUE_ERROR, "Error invalid response from module!");
              continue;
            }
            STACK.push_back(p4);
          }
          else if (p3.type == 'v')
          {
            KlassInstance *obj = allocKlassInstance(); // instance of class
            obj->members = ((Klass *)p3.ptr)->members;
            obj->privateMembers = ((Klass *)p3.ptr)->privateMembers;

            obj->klass = (Klass *)p3.ptr;
            PltObject r;
            r.type = 'o';
            r.ptr = (void *)obj;
            if (obj->members.find("__construct__") != obj->members.end())
            {
              PltObject construct = obj->members["__construct__"];
              if (construct.type != 'y')
              {
                spitErr(TYPE_ERROR, "Error constructor of module's class " + ((Klass *)p3.ptr)->name + " is not a native function!");
                continue;
              }
              NativeFunction *fn = (NativeFunction *)construct.ptr;
              NativeFunPtr p = fn->addr;
              pl1.insert(pl1.begin(), r);
              PltObject *args = &pl1[0];
              p(args, i2 + 1, &p1);
              if (p1.type == 'e')
              {
                // The module raised an error
                ErrObject *E = (ErrObject *)p1.ptr;
                int eCode = E->code;
                s1 = method_name + "():  " + E->des;
                spitErr((ErrCode)eCode, s1);
                continue;
              }
            }
            STACK.push_back(r);
            DoThreshholdBusiness();
          }
          else // that's it modules cannot have plutonium code functions (at least not right now)
          {
            spitErr(TYPE_ERROR, "Error member of module is not a function so cannot be called.");
            continue;
          }
        }
        else if (p1.type == 'j' || p1.type == 'a')
        {
          pl1.insert(pl1.begin(), p1);
          PltObject callmethod(string, PltObject *, int);
          p3 = callmethod(method_name, &pl1[0], i2 + 1);
          if (p3.type == 'e')
          {
            spitErr((ErrCode)p3.i, *(string *)p3.ptr);
            continue;
          }
          STACK.push_back(p3);
        }
        else if (p1.type == 'o')
        {
          KlassInstance *obj = (KlassInstance *)p1.ptr;
          if (obj->members.find(method_name) == obj->members.end())
          {
            if (obj->privateMembers.find(method_name) != obj->privateMembers.end())
            {
              FunObject *p = executing.back();
              if (p == NULL)
              {
                spitErr(NAME_ERROR, "Error " + method_name + " is private member of class instance!");
                continue;
              }
              if (p->klass == obj->klass)
              {
                p4 = obj->privateMembers[method_name];
              }
              else
              {
                spitErr(NAME_ERROR, "Error " + method_name + " is private member of class instance!");
                continue;
              }
            }
            else
            {
              spitErr(NAME_ERROR, "Error class instance has no member " + method_name);
              continue;
            }
          }
          else
            p4 = obj->members[method_name];
          if (p4.type == 'w')
          {
            FunObject *memFun = (FunObject *)p4.ptr;
            if ((size_t)i2 + 1 + memFun->opt.size() < memFun->args || (size_t)i2 + 1 > memFun->args)
            {
              spitErr(ARGUMENT_ERROR, "Error function " + memFun->name + " takes " + to_string(memFun->args - 1) + " arguments," + to_string(i2) + " given!");
              continue;
            }
            callstack.push_back(k + 1);
            if (callstack.size() >= 1000)
            {
              spitErr(MAX_RECURSION_ERROR, "Error max recursion limit 1000 reached.");
              continue;
            }
            executing.push_back(memFun);
            frames.push_back(STACK.size());
            STACK.insert(STACK.end(), pl1.begin(), pl1.end());
            // add default arguments
            for (size_t i = memFun->opt.size() - (memFun->args - 1 - (size_t)i2); i < (memFun->opt.size()); i++)
            {
              STACK.push_back(memFun->opt[i]);
            }
            //
            STACK.push_back(p1);
            k = program + memFun->i;
            continue;
          }
          else if (p4.type == 'y')
          {
            NativeFunction *fn = (NativeFunction *)p4.ptr;
            NativeFunPtr R = fn->addr;
            pl1.insert(pl1.begin(), p1);
            PltObject *args = &pl1[0];
            PltObject rr;
            R(args, i2 + 1, &rr);
            if (rr.type == 'e')
            {
              ErrObject *E = (ErrObject *)rr.ptr;
              spitErr((ErrCode)E->code, fn->name + ": " + E->des);
              continue;
            }
            STACK.push_back(rr);
          }
          else
          {
            spitErr(NAME_ERROR, "Error member " + method_name + " of object is not callable!");
            continue;
          }
        }
        else if (p1.type == 'z')
        {
          Coroutine *g = (Coroutine *)p1.ptr;
          if (method_name == "isAlive")
          {
            PltObject isAlive;
            isAlive.type = 'b';
            isAlive.i = (g->state != STOPPED);
            STACK.push_back(isAlive);
            k++;
            continue;
          }
          if (method_name != "resume")
          {
            spitErr(NAME_ERROR, "Error couroutine object has no member " + method_name);
            continue;
          }

          if (g->state == STOPPED)
          {
            spitErr(VALUE_ERROR, "Error the generator has terminated cannot resume it!");
            continue;
          }

          if (g->state == RUNNING)
          {
            spitErr(VALUE_ERROR, "Error the coroutine already running cannot resume it!");
            continue;
          }
          bool push = false;
          if (g->giveValOnResume)
          {
            if (pl1.size() != 1)
            {
              spitErr(VALUE_ERROR, "Error the coroutine expects one value to be resumed!");
              continue;
            }
            push = true;
          }
          else
          {
            if (pl1.size() != 0)
            {
              spitErr(VALUE_ERROR, "Error the coroutine does not expect any value to resume it!");
              continue;
            }
          }
          callstack.push_back(k + 1);
          if (callstack.size() >= 1000)
          {
            spitErr(MAX_RECURSION_ERROR, "Error max recursion limit 1000 reached.");
            continue;
          }
          executing.push_back(NULL);

          STACK.push_back(p1);
          frames.push_back(STACK.size());
          STACK.insert(STACK.end(), g->locals.begin(), g->locals.end());
          if (push)
            STACK.push_back(pl1[0]);
          g->state = RUNNING;
          g->giveValOnResume = false;
          k = program + g->curr;
          continue;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error member operator '.' not supported for type " + fullform(p1.type));
          continue;
        }
        break;
      }
      case ASSIGNMEMB:
      {
        orgk = k - program;
        PltObject val = STACK.back();
        STACK.pop_back();
        PltObject Parent = STACK.back();
        STACK.pop_back();
        string mname = "";
        k++;
        while (*k != 0)
        {
          mname += (char)*k;
          k++;
        }
        // k++;

        if (Parent.type != 'o')
        {
          spitErr(TYPE_ERROR, "Error member assignment unsupported for " + fullform(Parent.type));
          continue;
        }
        KlassInstance *ptr = (KlassInstance *)Parent.ptr;
        if (ptr->members.find(mname) == ptr->members.end())
        {
          if (Parent.type == 'o')
          {
            if (ptr->privateMembers.find(mname) != ptr->privateMembers.end())
            {
              // private member
              FunObject *A = executing.back();
              if (A == NULL)
              {
                spitErr(ACCESS_ERROR, "Error cannot access private member " + mname + " of class " + ptr->klass->name + "'s object!");
                continue;
              }
              if (ptr->klass != A->klass)
              {
                spitErr(ACCESS_ERROR, "Error cannot access private member " + mname + " of class " + ptr->klass->name + "'s object!");
                continue;
              }
              ptr->privateMembers[mname] = val;
              k += 1;
              continue;
            }
            else
            {
              spitErr(NAME_ERROR, "Error the object has no member named " + mname);
              continue;
            }
          }
          else
          {
            spitErr(NAME_ERROR, "Error the object has no member named " + mname);
            continue;
          }
        }

        ptr->members[mname] = val;
        break;
      }
      case IMPORT:
      {
        orgk = k - program;
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        string name = strings[i1];
        typedef void (*mfunc)(PltObject *);
        typedef void (*api)(allocFuncions *);
#ifdef BUILD_FOR_WINDOWS
        name = "C:\\plutonium\\modules\\" + name + ".dll";
        HINSTANCE module = LoadLibraryA(name.c_str());
        api a = (api)GetProcAddress(module, "api_setup");
        mfunc f = (mfunc)GetProcAddress(module, "init");
        if (!module)
        {
          spitErr(IMPORT_ERROR, "Error importing module " + to_string(GetLastError()));
          continue;
        }
#endif
#ifdef BUILD_FOR_LINUX
        name = "/opt/plutonium/modules/" + name + ".so";
        void *module = dlopen(name.c_str(), RTLD_LAZY);
        if (!module)
        {
          spitErr(IMPORT_ERROR, "Error importing module " + (std::string)(dlerror()));
          continue;
        }
        mfunc f = (mfunc)dlsym(module, "init");
        api a = (api)dlsym(module, "api_setup");
#endif
        if (!f)
        {
          spitErr(IMPORT_ERROR, "Error init() function not found in the module");
          continue;
        }
        if (!a)
        {
          spitErr(IMPORT_ERROR, "Error api_setup() function not found in the module");
          continue;
        }
        a(&allocators);
        PltObject Q;
        f(&Q);
        if (Q.type != 'q')
        {
          spitErr(VALUE_ERROR, "Error module's init() should return a module object!");
          continue;
        }
        moduleHandles.push_back(module);
        STACK.push_back(Q);

        break;
      }
      case RETURN:
      {
        k = callstack[callstack.size() - 1] - 1;
        callstack.pop_back();
        executing.pop_back();
        PltObject val = STACK[STACK.size() - 1];
        while (STACK.size() != (size_t)frames.back())
        {
          STACK.pop_back();
        }
        frames.pop_back();
        STACK.push_back(val);
        break;
      }
      case YIELD:
      {
        executing.pop_back();
        PltObject val = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltList locals = {STACK.end() - (STACK.size() - frames.back()), STACK.end()};
        STACK.erase(STACK.end() - (STACK.size() - frames.back()), STACK.end());
        PltObject genObj = STACK.back();
        STACK.pop_back();
        Coroutine *g = (Coroutine *)genObj.ptr;
        g->locals = locals;
        g->curr = k - program + 1;
        g->state = SUSPENDED;
        g->giveValOnResume = false;
        frames.pop_back();
        STACK.push_back(val);
        k = callstack[callstack.size() - 1] - 1;
        callstack.pop_back();
        break;
      }
      case YIELD_AND_EXPECTVAL:
      {
        executing.pop_back();
        PltObject val = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltList locals = {STACK.end() - (STACK.size() - frames.back()), STACK.end()};
        STACK.erase(STACK.end() - (STACK.size() - frames.back()), STACK.end());
        PltObject genObj = STACK.back();
        STACK.pop_back();
        Coroutine *g = (Coroutine *)genObj.ptr;
        g->locals = locals;
        g->curr = k - program + 1;
        g->state = SUSPENDED;
        g->giveValOnResume = true;
        frames.pop_back();
        STACK.push_back(val);
        k = callstack[callstack.size() - 1] - 1;
        callstack.pop_back();
        break;
      }
      case CO_STOP:
      {
        executing.pop_back();
        PltObject val = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltList locals = {STACK.end() - (STACK.size() - frames.back()), STACK.end()};
        STACK.erase(STACK.end() - (STACK.size() - frames.back()), STACK.end());
        PltObject genObj = STACK.back();
        STACK.pop_back();
        Coroutine *g = (Coroutine *)genObj.ptr;
        g->locals = locals;
        g->curr = k - program + 1;
        g->state = STOPPED;
        frames.pop_back();
        STACK.push_back(val);
        k = callstack[callstack.size() - 1] - 1;
        callstack.pop_back();
        break;
      }
      case POP_STACK:
      {
        STACK.pop_back();
        break;
      }
      case NPOP_STACK:
      {
        k += 1;
        memcpy(&i1, k, 4);
        k += 3;
        STACK.erase(STACK.end() - i1, STACK.end());
        break;
      }
      case GOTO_CALLSTACK:
      {
        k = callstack[callstack.size() - 1] - 1;
        callstack.pop_back();
        executing.pop_back();
        STACK.erase(STACK.begin() + frames.back(), STACK.end());
        frames.pop_back();
        break;
      }
      case LSHIFT:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__lshift__", p1, 2, "<<", &p2))
            continue;
        }
        if (p1.type != p2.type)
        {
          spitErr(TYPE_ERROR, "Error operands should have same type for '<<' ");
          continue;
        }
        p3.type = p1.type;
        if (p1.type == 'i')
        {
          p3.i = p1.i << p2.i;
        }
        else if (p1.type == 'l')
        {
          p3.l = p1.l << p2.l;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator << unsupported for type " + fullform(p1.type));
          continue;
        }
        STACK.push_back(p3);
        break;
      }
      case RSHIFT:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__rshift__", p1, 2, ">>", &p2))
            continue;
        }
        if (p1.type != p2.type)
        {
          spitErr(TYPE_ERROR, "Error operands should have same type for '>>' ");
          continue;
        }
        p3.type = p1.type;
        if (p1.type == 'i')
        {
          p3.i = p1.i >> p2.i;
        }
        else if (p1.type == 'l')
        {
          p3.l = p1.l >> p2.l;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator >> unsupported for type " + fullform(p1.type));
          continue;
        }
        STACK.push_back(p3);
        break;
      }
      case BITWISE_AND:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__bitwiseand__", p1, 2, "&", &p2))
            continue;
        }
        if (p1.type != p2.type)
        {
          spitErr(TYPE_ERROR, "Error operands should have same type for '&' ");
          continue;
        }
        p3.type = p1.type;
        if (p1.type == 'i')
        {
          p3.i = p1.i & p2.i;
        }
        else if (p1.type == 'l')
        {
          p3.l = p1.l & p2.l;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator & unsupported for type " + fullform(p1.type));
          continue;
        }
        STACK.push_back(p3);
        break;
      }
      case BITWISE_OR:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__bitwiseor__", p1, 2, "|", &p2))
            continue;
        }
        if (p1.type != p2.type)
        {
          spitErr(TYPE_ERROR, "Error operands should have same type for '|' ");
          continue;
        }
        p3.type = p1.type;
        if (p1.type == 'i')
        {
          p3.i = p1.i | p2.i;
        }
        else if (p1.type == 'l')
        {
          p3.l = p1.l | p2.l;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator | unsupported for type " + fullform(p1.type));
          continue;
        }
        STACK.push_back(p3);
        break;
      }
      case COMPLEMENT:
      {
        orgk = k - program;
        p1 = STACK.back();
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__complement__", p1, 1, "~"))
            continue;
        }
        if (p1.type == 'i')
        {
          p2.type = 'i';
          p2.i = ~p1.i;
          STACK.push_back(p2);
        }
        else if (p1.type == 'l')
        {
          p2.type = 'l';
          p2.l = ~p1.l;
          STACK.push_back(p2);
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '~' unsupported for type " + fullform(p1.type));
          continue;
        }
        break;
      }
      case XOR:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__xor__", p1, 2, "^", &p2))
            continue;
        }
        if (p1.type != p2.type)
        {
          spitErr(TYPE_ERROR, "Error operands should have same type for operator '^' ");
          continue;
        }
        p3.type = p1.type;
        if (p1.type == 'i')
        {
          p3.i = p1.i ^ p2.i;
        }
        else if (p1.type == 'l')
        {
          p3.l = p1.l ^ p2.l;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '^' unsupported for type " + fullform(p1.type));
          continue;
        }
        STACK.push_back(p3);
        break;
      }
      case ADD:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__add__", p1, 2, "+", &p2))
            continue;
        }
        if (isNumeric(p1.type) && isNumeric(p2.type))
        {
          if (p1.type == 'f' || p2.type == 'f')
            c1 = 'f';
          else if (p1.type == 'l' || p2.type == 'l')
            c1 = 'l';
          else if (p1.type == 'i' || p2.type == 'i')
            c1 = 'i';
          PromoteType(p1, c1);
          PromoteType(p2, c1);
        }
        else if (p1.type == p2.type)
          c1 = p1.type;
        else
        {
          spitErr(TYPE_ERROR, "Error operator '+' unsupported for " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        if (c1 == 'j')
        {
          p3.type = 'j';
          PltList res;
          PltList e = *(PltList *)p2.ptr;
          res = *(PltList *)p1.ptr;
          res.insert(res.end(), e.begin(), e.end());
          PltList *p = allocList();
          *p = res;
          p3.ptr = (void *)p;
          STACK.push_back(p3);
          // DoThreshholdBusiness();
        }
        else if (c1 == 's')
        {
          string *p = allocString();

          *p = *(string *)p1.ptr + *(string *)p2.ptr;
          p3 = PltObjectFromStringPtr(p);
          STACK.push_back(p3);
          DoThreshholdBusiness();
        }
        else if (c1 == 'i')
        {
          p3.type = 'i';
          if (!addition_overflows(p1.i, p1.i))
          {
            p3.i = p1.i + p2.i;
            STACK.push_back(p3);
            break;
          }
          if (addition_overflows((long long int)p1.i, (long long int)p2.i))
          {
            spitErr(OVERFLOW_ERROR, "Integer Overflow occurred");
            continue;
          }
          p3.type = 'l';
          p3.l = (long long int)(p1.i) + (long long int)(p2.i);
          STACK.push_back(p3);
        }
        else if (c1 == 'f')
        {
          if (addition_overflows(p1.f, p2.f))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Floating point overflow during addition");
            continue;
          }
          p3.type = 'f';
          p3.f = p1.f + p2.f;
          STACK.push_back(p3);
        }
        else if (c1 == 'l')
        {
          if (addition_overflows(p1.l, p2.l))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error overflow during solving expression.");
            continue;
          }
          p3.type = 'l';
          p3.l = p1.l + p2.l;
          STACK.push_back(p3);
          break;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '+' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        break;
      }
      case SMALLERTHAN:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (isNumeric(p1.type) && isNumeric(p2.type))
        {
          if (p1.type == 'f' || p2.type == 'f')
            c1 = 'f';
          else if (p1.type == 'l' || p2.type == 'l')
            c1 = 'l';
          else if (p1.type == 'i' || p2.type == 'i')
            c1 = 'i';
          PromoteType(p1, c1);
          PromoteType(p2, c1);
        }
        else if (p1.type == 'o')
        {
          if (invokeOperator("__smallerthan__", p1, 2, "<", &p2))
            continue;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '<' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        p3.type = 'b';
        if (p1.type == 'i')
          p3.i = (bool)(p1.i < p2.i);
        else if (p1.type == 'l')
          p3.i = (bool)(p1.l < p2.l);
        else if (p1.type == 'f')
          p3.i = (bool)(p1.f < p2.f);
        STACK.push_back(p3);
        break;
      }
      case GREATERTHAN:
      {
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (isNumeric(p1.type) && isNumeric(p2.type))
        {
          if (p1.type == 'f' || p2.type == 'f')
            c1 = 'f';
          else if (p1.type == 'l' || p2.type == 'l')
            c1 = 'l';
          else if (p1.type == 'i' || p2.type == 'i')
            c1 = 'i';
          PromoteType(p1, c1);
          PromoteType(p2, c1);
        }
        else if (p1.type == 'o')
        {
          if (invokeOperator("__greaterthan__", p1, 2, ">", &p2))
            continue;
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '>' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        p3.type = 'b';
        if (p1.type == 'i')
          p3.i = (bool)(p1.i > p2.i);
        else if (p1.type == 'l')
          p3.i = (bool)(p1.l > p2.l);
        else if (p1.type == 'f')
          p3.i = (bool)(p1.f > p2.f);
        STACK.push_back(p3);
        // exit(0);
        break;
      }
      case SMOREQ:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (isNumeric(p1.type) && isNumeric(p2.type))
        {
          if (p1.type == 'f' || p2.type == 'f')
            c1 = 'f';
          else if (p1.type == 'l' || p2.type == 'l')
            c1 = 'l';
          else if (p1.type == 'i' || p2.type == 'i')
            c1 = 'i';
          PromoteType(p1, c1);
          PromoteType(p2, c1);
        }
        else if (p1.type == 'o')
        {
          if (invokeOperator("__smallerthaneq__", p1, 2, "<=", &p2))
            continue;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '<=' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        p3.type = 'b';
        if (p1.type == 'i')
          p3.i = (bool)(p1.i <= p2.i);
        else if (p1.type == 'l')
          p3.i = (bool)(p1.l <= p2.l);
        else if (p1.type == 'f')
          p3.i = (bool)(p1.f <= p2.f);
        STACK.push_back(p3);
        break;
      }
      case GROREQ:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (isNumeric(p1.type) && isNumeric(p2.type))
        {
          if (p1.type == 'f' || p2.type == 'f')
            c1 = 'f';
          else if (p1.type == 'l' || p2.type == 'l')
            c1 = 'l';
          else if (p1.type == 'i' || p2.type == 'i')
            c1 = 'i';
          PromoteType(p1, c1);
          PromoteType(p2, c1);
        }
        else if (p1.type == 'o')
        {
          if (invokeOperator("__greaterthaneq__", p1, 2, ">=", &p2))
            continue;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error operator '>=' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        p3.type = 'b';
        if (p1.type == 'i')
          p3.i = (bool)(p1.i >= p2.i);
        else if (p1.type == 'l')
          p3.i = (bool)(p1.l >= p2.l);
        else if (p1.type == 'f')
          p3.i = (bool)(p1.f >= p2.f);
        STACK.push_back(p3);
        // exit(0);
        break;
      }
      case EQ:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'i' && p2.type == 'l')
          PromoteType(p1, 'l');
        else if (p1.type == 'l' && p2.type == 'i')
          PromoteType(p2, 'l');
        if (p1.type == 'o' && p2.type != 'n')
        {
          if (invokeOperator("__eq__", p1, 2, "==", &p2, 0))
            continue;
        }
        p3.type = 'b';
        p3.i = (bool)(p1 == p2);
        STACK.push_back(p3);
        break;
      }
      case NOT:
      {
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__not__", a, 1, "!"))
            continue;
        }
        if (a.type != 'b')
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '!' unsupported for type " + fullform(a.type));
          continue;
        }
        a.i = (bool)!(a.i);
        STACK.push_back(a);
        break;
      }
      case NEG:
      {

        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__neg__", a, 1, "-"))
            continue;
        }
        if (!isNumeric(a.type))
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error unary operator '-' unsupported for type " + fullform(a.type));
          exit(0);
        };
        if (a.type == 'i')
        {
          if (a.i != INT_MIN)
            a.i = -(a.i);
          else
          {
            a.type = 'l';
            a.l = -(long long int)(INT_MIN);
          }
        }
        else if (a.type == 'l')
        {
          if (a.l == LLONG_MIN) // taking negative of LLONG_MIN results in LLONG_MAX+1 so we have to avoid it
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error negation of INT64_MIN causes overflow!");
            continue;
          }
          else if (-a.l == INT_MIN)
          {
            a.type = 'i';
            a.i = INT_MIN;
          }
          else
            a.l = -a.l;
        }
        else if (a.type == 'f')
        {
          a.f = -a.f;
        }
        STACK.push_back(a);
        break;
      }
      case INDEX:
      {
        PltObject i = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject val = STACK[STACK.size() - 1];
        STACK.pop_back();

        if (val.type == 'j')
        {
          if (i.type != 'i' && i.type != 'l')
          {
            orgk = k - program;
            spitErr(TYPE_ERROR, "Error index should be integer!");
            continue;
          }
          PromoteType(i, 'l');
          if (i.l < 0)
          {
            orgk = k - program;
            spitErr(VALUE_ERROR, "Error index cannot be negative!");
            continue;
          }

          PltList l = *(PltList *)val.ptr;
          if ((size_t)i.l >= l.size())
          {
            orgk = k - program;
            spitErr(VALUE_ERROR, "Error index is out of range!");
            continue;
          }
          STACK.push_back(l[i.i]);
          break;
        }
        else if (val.type == 'a')
        {
          Dictionary d = *(Dictionary *)val.ptr;
          if (d.find(i) == d.end())
          {
            orgk = k - program;
            spitErr(KEY_ERROR, "Error key " + PltObjectToStr(i) + " not found in the dictionary!");
            continue;
          }
          PltObject res = d[i];
          STACK.push_back(res);
        }
        else if (val.type == 's')
        {
          if (i.type != 'i' && i.type != 'l')
          {
            orgk = k - program;
            spitErr(TYPE_ERROR, "Error index should be integer!");
            continue;
          }
          PromoteType(i, 'l');
          if (i.l < 0)
          {
            orgk = k - program;
            spitErr(VALUE_ERROR, "Error index cannot be negative!");
            continue;
          }
          string s = *(string *)val.ptr;
          if ((size_t)i.l >= s.length())
          {
            orgk = k - program;
            spitErr(VALUE_ERROR, "Error index is out of range!");
          }
          char c = s[i.l];
          PltObject a;
          string *p = allocString();
          *p += c;
          a = PltObjectFromStringPtr(p);
          STACK.push_back(a);
          DoThreshholdBusiness();
        }
        else if (val.type == 'o')
        {
          if (invokeOperator("__index__", val, 2, "[]", &i))
            continue;
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '[]' unsupported for type " + fullform(val.type));
          continue;
        }
        break;
      }
      case NOTEQ:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();

        if (a.type == 'o' && b.type != 'n')
        {
          if (invokeOperator("__noteq__", a, 2, "!=", &b, 0))
            continue;
        }
        PltObject c;
        c.type = 'b';
        if (a.type == 'i' && b.type == 'l')
          PromoteType(a, 'l');
        else if (a.type == 'l' && b.type == 'i')
          PromoteType(b, 'l');
        c.i = (bool)!(a == b);
        STACK.push_back(c);
        break;
      }
      case AND:
      {
        orgk = k - program;
        p2 = STACK[STACK.size() - 1];
        STACK.pop_back();
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'o')
        {
          if (invokeOperator("__and__", p1, 2, "and", &p2))
            continue;
        }

        if (p1.type != 'b' || p2.type != 'b')
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator 'and' unsupported for types " + fullform(p1.type) + " and " + fullform(p2.type));
          continue;
        }
        p3.type = 'b';
        p3.i = (bool)(p1.i && p2.i);
        STACK.push_back(p3);
        break;
      }
      case IS:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject c;
        if ((a.type != 'a' && a.type != 'v' && a.type != 'j' && a.type != 'o' && a.type != 's' && a.type != 'q' && a.type != 'w') || (a.type != 'v' && b.type != 'a' && b.type != 's' && b.type != 'w' && b.type != 'j' && b.type != 'o' && b.type != 'q'))
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator 'is' unsupported for types " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }
        c.type = 'b';
        c.i = (bool)(a.ptr == b.ptr);
        STACK.push_back(c);
        break;
      }
      case OR:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__or__", a, 2, "or", &b))
            continue;
        }
        if (a.type != 'b' || b.type != 'b')
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator 'or' unsupported for types " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }
        PltObject c;
        c.type = 'b';
        c.i = (bool)(a.i || b.i);
        STACK.push_back(c);
        break;
        // exit(0);
      }
      case MUL:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__mul__", a, 2, "*", &b))
            continue;
        }
        PltObject c;
        char t;
        if (isNumeric(a.type) && isNumeric(b.type))
        {
          if (a.type == 'f' || b.type == 'f')
            t = 'f';
          else if (a.type == 'l' || b.type == 'l')
            t = 'l';
          else if (a.type == 'i' || b.type == 'i')
            t = 'i';
          PromoteType(a, t);
          PromoteType(b, t);
        }
        else if(a.type=='j' && b.type=='i')
        {
          PltList* src = (PltList*)a.ptr;
          PltList* res = allocList();
          if(src->size()!=0)
          {
          for(int i=b.i;i;--i)
            res->insert(res->end(),src->begin(),src->end());
          }
          p1.type = 'j';
          p1.ptr = (void*)res;
          STACK.push_back(p1);
          DoThreshholdBusiness();
          ++k;
          continue;
        }
        else if(a.type=='s' && b.type=='i')
        {
          string* src = (string*)a.ptr;
          string* res = allocString();
          if(src->size()!=0)
          {
            for(int i=b.i;i;--i)
              res->insert(res->end(),src->begin(),src->end());
          }
          p1.type = 's';
          p1.ptr = (void*)res;
          STACK.push_back(p1);
          DoThreshholdBusiness();
          ++k;
          continue;
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '*' unsupported for " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }

        if (t == 'i')
        {
          c.type = 'i';
          if (!multiplication_overflows(a.i, b.i))
          {
            c.i = a.i * b.i;
            STACK.push_back(c);
            break;
          }
          orgk = k - program;
          if (multiplication_overflows((long long int)a.i, (long long int)b.i))
          {
            spitErr(OVERFLOW_ERROR, "Overflow occurred");
            continue;
          }
          c.type = 'l';
          c.l = (long long int)(a.i) * (long long int)(b.i);
          STACK.push_back(c);
        }
        else if (t == 'f')
        {
          if (multiplication_overflows(a.f, b.f))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Floating point overflow during multiplication");
            continue;
          }
          c.type = 'f';
          c.f = a.f * b.f;
          STACK.push_back(c);
        }
        else if (t == 'l')
        {
          if (multiplication_overflows(a.l, b.l))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error overflow during solving expression.");
            continue;
          }
          c.type = 'l';
          c.l = a.l * b.l;
          STACK.push_back(c);
        }
        break;
      }
      case MEMB:
      {
        orgk = k - program;
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        ++k;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        string& mname = strings[i1];
        if (a.type == 'e')
        {
          ErrObject *E = (ErrObject *)a.ptr;
          if (mname == "code")
          {
            p1.type = 'i';
            p1.i = E->code;
          }
          else if (mname == "msg")
          {
            string *p = allocString();
            *p = E->des;
            p1 = PltObjectFromStringPtr(p);
          }
          else if (mname == "name")
          {
            p1.type = 's';
            if (a.i >= 1 && a.i <= 16)
              *(string *)p1.ptr = ErrNames[a.i - 1];
            else
              *(string *)p1.ptr = "UnknownError";
          }
          else
          {
            spitErr(NAME_ERROR, "Error object has no member named " + mname);
            continue;
          }
          STACK.push_back(p1);
          k++;
          continue;
        }
        else if (a.type == 'q')
        {
          Module *m = (Module *)a.ptr;

          if (m->members.find(mname) == m->members.end())
          {
            spitErr(NAME_ERROR, "Error module object has no member named '" + mname + "' ");
            continue;
          }
          STACK.push_back(m->members[mname]);
          ++k;
          continue;
        }
        else if(a.type == 'o')
        {
          KlassInstance *ptr = (KlassInstance *)a.ptr;
          if (ptr->members.find(mname) == ptr->members.end())
          {
            if (a.type == 'o')
            {
              if (ptr->privateMembers.find(mname) != ptr->privateMembers.end())
              {
                FunObject *A = executing.back();
                if (A == NULL)
                {
                  spitErr(ACCESS_ERROR, "Error cannot access private member " + mname + " of class " + ptr->klass->name + "'s object!");
                  continue;
                }
                if (ptr->klass != A->klass)
                {
                  spitErr(ACCESS_ERROR, "Error cannot access private member " + mname + " of class " + ptr->klass->name + "'s object!");
                  continue;
                }
                STACK.push_back(ptr->privateMembers[mname]);
                k += 1;
                continue;
              }
              else
              {
                spitErr(NAME_ERROR, "Error object has no member named " + mname);
                continue;
              }
            }
            else
            {
              spitErr(NAME_ERROR, "Error object has no member named " + mname);
              continue;
            }
          }
          PltObject ret = ptr->members[mname];
          STACK.push_back(ret);
        }
        else
        {
          spitErr(TYPE_ERROR, "Error member operator only supported for objects!");
          continue;
        }
        break;
      }
      case LOAD_FUNC:
      {
        k += 1;
        int p;
        memcpy(&p, k, sizeof(int));
        k += 4;
        int idx;
        memcpy(&idx, k, sizeof(int));
        k += 4;
        FunObject *fn = allocFunObject();
        fn->i = p;
        fn->args = *k;
        fn->name = strings[idx];
        p1.type = PLT_FUNC;
        p1.ptr = (void *)fn;
        k++;
        i2 = (int)*k;
        fn->opt.insert(fn->opt.end(), STACK.end() - i2, STACK.end());
        STACK.erase(STACK.end() - i2, STACK.end());
        STACK.push_back(p1);
        DoThreshholdBusiness();
        break;
      }
      case LOAD_CO:
      {
        k += 1;
        int p;
        memcpy(&p, k, sizeof(int));
        k += 4;
        int idx;
        memcpy(&idx, k, sizeof(int));
        k += 4;
        PltObject co;
        co.type = 'g';
        co.i = p;
        co.extra = *k;
        STACK.push_back(co);
        break;
      }
      case BUILD_CLASS:
      {
        k += 1;
        int N;
        memcpy(&N, k, sizeof(int));
        k += 4;
        int idx;
        memcpy(&idx, k, sizeof(int));
        k += 3;
        string name = strings[idx];
        PltObject klass;
        klass.type = 'v';
        Klass *obj = allocKlass();
        obj->name = name;
        vector<PltObject> values;
        vector<PltObject> names;
        for (int i = 1; i <= N; i++)
        {
          p1 = STACK.back();
          if (p1.type == PLT_FUNC)
          {
            FunObject *ptr = (FunObject *)p1.ptr;
            ptr->klass = obj;
          }
          values.push_back(p1);
          STACK.pop_back();
        }
        for (int i = 1; i <= N; i++)
        {
          names.push_back(STACK.back());
          STACK.pop_back();
        }
        for (int i = 0; i < N; i += 1)
        {
          s1 = *(string *)names[i].ptr;
          if (s1[0] == '@')
          {
            s1.erase(s1.begin());
            obj->privateMembers.emplace(s1, values[i]);
          }
          else
            obj->members.emplace(s1, values[i]);
        }
        klass.ptr = (void *)obj;
        STACK.push_back(klass);
        break;
      }
      case BUILD_DERIVED_CLASS:
      {
        orgk = k - program;
        k += 1;
        int N;
        memcpy(&N, k, sizeof(int));
        k += 4;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        // N is total new class members
        // i1 is idx of class name in strings array
        string name = strings[i1];
        PltObject klass;
        klass.type = 'v';
        Klass *d = allocKlass();
        d->name = name;
        vector<PltObject> values;
        vector<PltObject> names;
        for (int i = 1; i <= N; i++)
        {
          p1 = STACK.back();
          if (p1.type == PLT_FUNC)
          {
            FunObject *ptr = (FunObject *)p1.ptr;
            ptr->klass = d;
          }
          values.push_back(p1);
          STACK.pop_back();
        }
        for (int i = 1; i <= N; i++)
        {
          names.push_back(STACK.back());
          STACK.pop_back();
        }
        PltObject baseClass = STACK.back();
        STACK.pop_back();
        if (baseClass.type != 'v')
        {
          spitErr(TYPE_ERROR, "Error class can not be derived from object of non class type!");
          continue;
        }
        Klass *Base = (Klass *)baseClass.ptr;

        for (int i = 0; i < N; i += 1)
        {
          s1 = *(string *)names[i].ptr;
          if (s1[0] == '@')
          {
            s1.erase(s1.begin());
            d->privateMembers.emplace(s1, values[i]);
          }
          else
            d->members.emplace(s1, values[i]);
        }
        string n;
        for (auto e : Base->members)
        {
          const string &n = e.first;
          if (n == "super")//do not add base class's super to this class
            continue;
          if (d->members.find(n) == d->members.end())
          {
            if (d->privateMembers.find(n) == d->privateMembers.end())
            {
              p1 = e.second;
              if (p1.type == PLT_FUNC)
              {
                FunObject *p = (FunObject *)p1.ptr;
                FunObject *rep = allocFunObject();
                *rep = *p;
                rep->klass = d;
                p1.type = 'w';
                p1.ptr = (void *)rep;
              }
              d->members.emplace(e.first, p1);
            }
          }
        }
        for (auto e : Base->privateMembers)
        {
          const string &n = e.first;
          if (d->privateMembers.find(n) == d->privateMembers.end())
          {
            if (d->members.find(n) == d->members.end())
            {
              p1 = e.second;
              if (p1.type == PLT_FUNC)
              {
                FunObject *p = (FunObject *)p1.ptr;
                FunObject *rep = allocFunObject();
                *rep = *p;
                rep->klass = d;
                p1.type = 'w';
                p1.ptr = (void *)rep;
              }
              d->privateMembers.emplace(n, p1);
            }
          }
        }
        PltObject parent;
        parent.type = 'o';
        KlassInstance *super = allocKlassInstance();
        super->klass = Base;
        super->members = Base->members;
        super->privateMembers = Base->privateMembers;
        parent.ptr = (void *)super;
        d->members.emplace(strings[0], parent);
        klass.ptr = (void *)d;
        STACK.push_back(klass);
        break;
      }
      case LOAD_NAME:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        PltObject ret;
        ret = PltObjectFromStringPtr(&(strings[i1])); // load as shallow copy
        STACK.push_back(ret);
        break;
      }
      case LOAD_STR:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        p1.type = 's';
        p1.ptr = (void *)&strings[i1];
        STACK.push_back(p1);

        break;
      }
      case CALLUDF:
      {
        orgk = k - program;
        PltObject fn = STACK.back();
        if (fn.type != 'v' && fn.type != 'w' && fn.type != 'g' && fn.type != 'y')
        {
          spitErr(TYPE_ERROR, "Error type " + fullform(fn.type) + " not callable!");
          continue;
        }
        STACK.pop_back();
        k += 1;
        int N = *k;
        if (fn.type == 'w')
        {
          FunObject *obj = (FunObject *)fn.ptr;
          if ((size_t)N + obj->opt.size() < obj->args || (size_t)N > obj->args)
          {
            spitErr(ARGUMENT_ERROR, "Error function " + obj->name + " takes " + to_string(obj->args) + " arguments," + to_string(N) + " given!");
            continue;
          }
          callstack.push_back(k + 1);
          if (callstack.size() >= 1000)
          {
            spitErr(MAX_RECURSION_ERROR, "Error max recursion limit 1000 reached.");
            continue;
          }
          for (size_t i = obj->opt.size() - (obj->args - N); i < obj->opt.size(); i++)
            STACK.push_back(obj->opt[i]);
          executing.push_back(obj);
          frames.push_back(STACK.size() - obj->args);
          k = program + obj->i;
          continue;
        }
        else if (fn.type == 'y')
        {
          NativeFunction *A = (NativeFunction *)fn.ptr;
          NativeFunPtr f = A->addr;
          p4.type = 'n';
          f(&(STACK[STACK.size() - N]), N, &p4);
          if (p4.type == 'e')
          {
            int eCode = p4.i;
            s1 = "Native Function:  " + *(string *)p4.ptr;
            spitErr((ErrCode)eCode, s1);
            continue;
          }
          if (fullform(p4.type) == "Unknown" && p4.type != 'n')
          {
            spitErr(VALUE_ERROR, "Error invalid response from module!");
            continue;
          }
          STACK.erase(STACK.end() - N, STACK.end());
          STACK.push_back(p4);
        }
        else if (fn.type == 'v')
        {

          KlassInstance *obj = allocKlassInstance(); // instance of class
          obj->members = ((Klass *)fn.ptr)->members;
          if (obj->members.find("super") != obj->members.end())
          {
            KlassInstance *newSuper = allocKlassInstance();
            KlassInstance* super = (KlassInstance*)obj->members["super"].ptr;
            newSuper->klass = super->klass;
            newSuper->members = super->members;
            newSuper->privateMembers = super->privateMembers;
            obj->members["super"] = PltObjectFromKlassInstance(newSuper);
          }
          obj->privateMembers = ((Klass *)fn.ptr)->privateMembers;

          obj->klass = (Klass *)fn.ptr;
          if (obj->members.find("__construct__") != obj->members.end())
          {
            PltObject construct = obj->members["__construct__"];
            if (construct.type == 'w')
            {
              FunObject *p = (FunObject *)construct.ptr;
              if ((size_t)N + p->opt.size() + 1 < p->args || (size_t)N + 1 > p->args)
              {
                spitErr(ARGUMENT_ERROR, "Error constructor of class " + ((Klass *)fn.ptr)->name + " takes " + to_string(p->args - 1) + " arguments," + to_string(N) + " given!");
                continue;
              }
              PltObject r;
              r.type = 'o';
              r.ptr = (void *)obj;
              callstack.push_back(k + 1);

              for (size_t i = p->opt.size() - (p->args - 1 - N); i < p->opt.size(); i++)
              {
                STACK.push_back(p->opt[i]);
              }
              STACK.push_back(r);
              k = program + p->i;
              executing.push_back(p);
              frames.push_back(STACK.size() - (p->args));
              DoThreshholdBusiness();
              continue;
            }
            else if (construct.type == 'y')
            {
              NativeFunction *M = (NativeFunction *)construct.ptr;
              PltObject *args = NULL;
              PltObject r;
              r.type = 'o';
              r.ptr = (void *)obj;
              STACK.insert(STACK.end() - N, r);
              args = &STACK[STACK.size() - (N + 1)];
              M->addr(args, N + 1, &p4);
              STACK.erase(STACK.end() - (N + 1), STACK.end());
              if (p4.type == 'e')
              {
                ErrObject *E = (ErrObject *)p4.ptr;
                spitErr((ErrCode)E->code, ((Klass *)fn.ptr)->name + "." + "__construct__:  " + E->des);
                continue;
              }
              STACK.push_back(r);
              DoThreshholdBusiness();
              k++;
              continue;
            }
            else
            {
              spitErr(TYPE_ERROR, "Error constructor of class " + ((Klass *)fn.ptr)->name + " is not a function!");
              continue;
            }
          }
          else
          {
            if (N != 0)
            {
              spitErr(ARGUMENT_ERROR, "Error constructor of class " + ((Klass *)fn.ptr)->name + " takes 0 arguments!");
              continue;
            }
          }
          PltObject r;
          r.type = 'o';
          r.ptr = (void *)obj;
          STACK.push_back(r);
          DoThreshholdBusiness();
        }
        else if (fn.type == 'g')
        {
          if ((size_t)N != fn.extra)
          {
            spitErr(ARGUMENT_ERROR, "Error coroutine " + *(string *)fn.ptr + " takes " + to_string(fn.extra) + " arguments," + to_string(N) + " given!");
            continue;
          }
          Coroutine *g = allocCoroutine();
          g->curr = fn.i;
          g->state = SUSPENDED;
          vector<PltObject> locals = {STACK.end() - N, STACK.end()};
          STACK.erase(STACK.end() - N, STACK.end());
          g->locals = locals;
          g->giveValOnResume = false;
          PltObject T;
          T.type = 'z';
          T.ptr = g;
          STACK.push_back(T);
          DoThreshholdBusiness();
        }
        break;
      }
      case MOD:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__mod__", a, 2, "%", &b))
            continue;
        }
        PltObject c;
        char t;
        if (isNumeric(a.type) && isNumeric(b.type))
        {
          if (a.type == 'f' || b.type == 'f')
          {
            orgk = k - program;
            spitErr(TYPE_ERROR, "Error modulo operator % unsupported for floats!");
            continue;
          }
          else if (a.type == 'l' || b.type == 'l')
          {
            t = 'l';
          }
          else if (a.type == 'i' || b.type == 'i')
            t = 'i';
          PromoteType(a, t);
          PromoteType(b, t);
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '%' unsupported for " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }
        //

        if (t == 'i')
        {
          c.type = 'i';
          if (b.i == 0)
          {
            orgk = k - program;
            spitErr(MATH_ERROR, "Error modulo by zero");
            continue;
          }
          if ((a.i == INT_MIN) && (b.i == -1))
          {
            /* handle error condition */
            c.type = 'l';
            c.l = (long long int)a.i % (long long int)b.i;
            STACK.push_back(c);
          }
          c.i = a.i % b.i;
          STACK.push_back(c);
        }
        else if (t == 'l')
        {
          c.type = 'l';
          if (b.l == 0)
          {
            orgk = k - program;
            spitErr(MATH_ERROR, "Error modulo by zero");
            continue;
          }
          if ((a.l == LLONG_MIN) && (b.l == -1))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error modulo of INT32_MIN by -1 causes overflow!");
            continue;
          }
          c.i = a.i % b.i;
          STACK.push_back(c);
        }
        break;
      }
      case INPLACE_INC:
      {
        orgk = k - program;
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        char t = STACK[frames.back() + i1].type;
        if (t == 'i')
        {
          if (STACK[frames.back() + i1].i == INT_MAX)
          {
            STACK[frames.back() + i1].l = (long long int)INT_MAX + 1;
            STACK[frames.back() + i1].type = 'l';
          }
          else
            STACK[frames.back() + i1].i += 1;
        }
        else if (t == 'l')
        {
          if (STACK[frames.back() + i1].l == LLONG_MAX)
          {
            spitErr(OVERFLOW_ERROR, "Error numeric overflow");
            continue;
          }
          STACK[frames.back() + i1].l += 1;
        }
        else if (t == 'f')
        {
          if (STACK[frames.back() + i1].f == FLT_MAX)
          {
            spitErr(OVERFLOW_ERROR, "Error numeric overflow");
            continue;
          }
          STACK[frames.back() + i1].f += 1;
        }
        else
        {
          spitErr(TYPE_ERROR, "Error cannot add numeric constant to type " + fullform(t));
          continue;
        }
        break;
      }
      case SUB:
      {

        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__sub__", a, 2, "-", &b))
            continue;
        }
        PltObject c;
        char t;
        if (isNumeric(a.type) && isNumeric(b.type))
        {
          if (a.type == 'f' || b.type == 'f')
            t = 'f';
          else if (a.type == 'l' || b.type == 'l')
            t = 'l';
          else if (a.type == 'i' || b.type == 'i')
            t = 'i';
          PromoteType(a, t);
          PromoteType(b, t);
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '-' unsupported for " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }

        //
        if (t == 'i')
        {
          c.type = 'i';
          if (!subtraction_overflows(a.i, b.i))
          {
            c.i = a.i - b.i;
            STACK.push_back(c);
            break;
          }
          if (subtraction_overflows((long long int)a.i, (long long int)b.i))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Overflow occurred");
          }
          c.type = 'l';
          c.l = (long long int)(a.i) - (long long int)(b.i);
          STACK.push_back(c);
        }
        else if (t == 'f')
        {
          if (subtraction_overflows(a.f, b.f))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Floating point overflow during subtraction");
            continue;
          }
          c.type = 'f';
          c.f = a.f - b.f;
          STACK.push_back(c);
        }
        else if (t == 'l')
        {
          if (subtraction_overflows(a.l, b.l))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error overflow during solving expression.");
            continue;
          }
          c.type = 'l';
          c.l = a.l - b.l;
          STACK.push_back(c);
        }
        break;
      }
      case DIV:
      {
        PltObject b = STACK[STACK.size() - 1];
        STACK.pop_back();
        PltObject a = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (a.type == 'o')
        {
          if (invokeOperator("__div__", a, 2, "/", &b))
            continue;
        }
        PltObject c;
        char t;
        if (isNumeric(a.type) && isNumeric(b.type))
        {
          if (a.type == 'f' || b.type == 'f')
            t = 'f';
          else if (a.type == 'l' || b.type == 'l')
            t = 'l';
          else if (a.type == 'i' || b.type == 'i')
            t = 'i';
          PromoteType(a, t);
          PromoteType(b, t);
        }
        else
        {
          orgk = k - program;
          spitErr(TYPE_ERROR, "Error operator '/' unsupported for " + fullform(a.type) + " and " + fullform(b.type));
          continue;
        }

        if (t == 'i')
        {
          if (b.i == 0)
          {
            orgk = k - program;
            spitErr(MATH_ERROR, "Error division by zero");
            continue;
          }
          c.type = 'i';
          if (!division_overflows(a.i, b.i))
          {
            c.i = a.i / b.i;
            STACK.push_back(c);
            break;
          }
          if (division_overflows((long long int)a.i, (long long int)b.i))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Overflow occurred");
            continue;
          }
          c.type = 'l';
          c.l = (long long int)(a.i) / (long long int)(b.i);
          STACK.push_back(c);
          break;
        }
        else if (t == 'f')
        {
          if (b.f == 0)
          {
            orgk = k - program;
            spitErr(MATH_ERROR, "Error division by zero");
            continue;
          }
          if (division_overflows(a.f, b.f))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Floating point overflow during division");
            continue;
          }
          c.type = 'f';
          c.f = a.f / b.f;
          STACK.push_back(c);
        }
        else if (t == 'l')
        {
          if (b.l == 0)
          {
            orgk = k - program;
            spitErr(MATH_ERROR, "Error division by zero");
            continue;
          }
          if (division_overflows(a.l, b.l))
          {
            orgk = k - program;
            spitErr(OVERFLOW_ERROR, "Error overflow during solving expression.");
            continue;
          }
          c.type = 'l';
          c.l = a.l / b.l;
          STACK.push_back(c);
        }
        break;
      }
      case JMP:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        int where = k - program + i1 + 1;
        k = program + where - 1;
        break;
      }
      case JMPNPOPSTACK:
      {
        k += 1;
        int N;
        memcpy(&N, k, sizeof(int));
        k += 4;
        STACK.erase(STACK.end() - N, STACK.end());
        memcpy(&i1, k, sizeof(int));
        k += 3;
        int where = k - program + i1;
        k = program + where;
        break;
      }
      case JMPIF:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        int where = k - program + i1 + 1;
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.i && p1.type == 'b')
          k = program + where - 1;
        else
          break;
        break;
      }
      case THROW:
      {
        orgk = k - program;
        p3 = STACK.back(); // err message
        STACK.pop_back();
        p2 = STACK.back(); // error code
        STACK.pop_back();
        p1.type = 'e';
        ErrObject *E = allocErrObject();
        E->des = *(string *)p3.ptr;
        E->code = p2.i;
        p1.ptr = (void *)E;
        if (except_targ.size() == 0)
        {
          spitErr((ErrCode)p2.i, *(string *)p3.ptr);
          continue;
        }
        k = except_targ.back();
        i1 = STACK.size() - tryStackCleanup.back();

        STACK.erase(STACK.end() - i1, STACK.end());
        i1 = frames.size() - tryLimitCleanup.back();
        frames.erase(frames.end() - i1, frames.end());
        STACK.push_back(p1);

        except_targ.pop_back();
        tryStackCleanup.pop_back();
        tryLimitCleanup.pop_back();
        continue;
        break;
      }
      case ONERR_GOTO:
      {
        k += 1;
        memcpy(&i1, k, 4);
        except_targ.push_back((unsigned char *)program + i1);
        tryStackCleanup.push_back(STACK.size());
        tryLimitCleanup.push_back(frames.size());
        k += 3;
        break;
      }
      case POP_EXCEP_TARG:
      {
        except_targ.pop_back();
        tryStackCleanup.pop_back();
        break;
      }
      case GOTO:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k = program + i1 - 1;
        break;
      }
      case BREAK:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 4;
        int p = i1;
        memcpy(&i1, k, sizeof(int));
        STACK.erase(STACK.end() - i1, STACK.end());
        k = program + p;
        p1.type = 'n';
        STACK.push_back(p1);
        continue;
      }
      case CONT:
      {
        k += 1;
        int p;
        memcpy(&p, k, sizeof(int));
        k += 4;
        memcpy(&i1, k, sizeof(int));
        STACK.erase(STACK.end() - i1, STACK.end());
        k = program + p;
        continue;
      }
      case GOTONPSTACK:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 4;
        STACK.erase(STACK.end() - i1, STACK.end());
        memcpy(&i1, k, sizeof(int));
        k = program + i1;
        continue;
      }
      case JMPIFFALSE:
      {
        k += 1;
        memcpy(&i1, k, sizeof(int));
        k += 3;
        p1 = STACK[STACK.size() - 1];
        STACK.pop_back();
        if (p1.type == 'n' || (p1.type == 'b' && p1.i == 0))
        {
          k = k + i1 + 1;
          continue;
        }
        break;
      }
      case GC:
      {
        mark();
        collectGarbage();
        break;
      }
      default:
      {
        // unknown opcode
        // Faulty bytecode
        printf("An InternalError occurred.Error Code: 14\n");
        exit(0);
        break;
      }

      } // end switch statement
      k += 1;

    } // end while loop
    if (STACK.size() != 0 && panic)
    {
      printf("An InternalError occurred.Error Code: 15\n");
      exit(0);
    }
  } // end function interpret
  ~VM()
  {

    delete[] program;
    delete[] constants;
    STACK.clear();

    vector<void *> toerase;
    for (auto e : memory)
    {
      MemInfo m = e.second;

      if (m.type == 'o')
      {
        KlassInstance *obj = (KlassInstance *)e.first;
        PltObject dummy;
        dummy.type = 'o';
        dummy.ptr = e.first;
        if (obj->members.find(".destroy") != obj->members.end())
        {
          NativeFunction *ptr = (NativeFunction *)obj->members[".destroy"].ptr;
          PltObject rr;
          ptr->addr(&dummy, 1, &rr);
        }
        delete (KlassInstance *)e.first;
        toerase.push_back(e.first);
      }
    }
    for (auto e : toerase)
      memory.erase(e);
    mark(); // clearing the STACK and marking objects will result in all objects being deleted
    // which is what we want
    collectGarbage();
    typedef void (*unload)(void);
    for (auto e : moduleHandles)
    {
#ifdef BUILD_FOR_WINDOWS
      unload ufn = (unload)GetProcAddress(e, "unload");
      if (ufn)
        ufn();
      FreeLibrary(e);

#endif
#ifdef BUILD_FOR_LINUX
      unload ufn = (unload)dlsym(e, "unload");
      if (ufn)
        ufn();
      dlclose(e);
#endif
    }
  }
} vm;
PltList *allocList()
{
  PltList *p = new PltList;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(PltList);
  MemInfo m;
  m.type = 'j';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
ErrObject *allocErrObject()
{
  ErrObject *p = new ErrObject;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(ErrObject);
  MemInfo m;
  m.type = 'e';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
string *allocString()
{

  string *p = new string;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(string);
  MemInfo m;
  m.type = 's';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
Klass *allocKlass()
{
  Klass *p = new Klass;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(Klass);
  MemInfo m;
  m.type = 'v';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
Module *allocModule()
{
  Module *p = new Module;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(Module);
  MemInfo m;
  m.type = 'q';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
KlassInstance *allocKlassInstance()
{
  KlassInstance *p = new KlassInstance;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(KlassInstance);
  MemInfo m;
  m.type = 'o';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
Coroutine *allocCoroutine()
{
  Coroutine *p = new Coroutine;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(Coroutine);
  MemInfo m;
  m.type = 'z';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
FunObject *allocFunObject()
{
  FunObject *p = new FunObject;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  p->klass = NULL;
  vm.allocated += sizeof(FunObject);
  MemInfo m;
  m.type = PLT_FUNC;
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
FileObject *allocFileObject()
{

  FileObject *p = new FileObject;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(PltList);
  MemInfo m;
  m.type = 'u';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
Dictionary *allocDict()
{
  Dictionary *p = new Dictionary;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(Dictionary);
  MemInfo m;
  m.type = 'a';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}
NativeFunction *allocNativeFun()
{
  NativeFunction *p = new NativeFunction;
  if (!p)
  {
    printf("error allocating memory!\n");
    exit(0);
  }
  vm.allocated += sizeof(NativeFunction);
  MemInfo m;
  m.type = 'y';
  m.isMarked = false;
  vm.memory.emplace((void *)p, m);
  return p;
}

#endif
