//These header contains some builtin functions which are embed into the interpreter
//the prototype of these functions is different from the ones in the native modules

#ifndef BUILTIN_FUNC_H_
#define BUILTIN_FUNC_H_
#include "vm.h"
#include "plutonium.h"
using namespace std;

std::unordered_map<string,BuiltinFunc> funcs;

int f;
string fullform(char);
#define PltArgs const vector<PltObject>&
void PromoteType(PltObject&,char);

PltList ListToByteList(PltList l)
{
   size_t k = 0;
        PltObject A;
        A.type = 'm';
        PltList res;
        while(k<l.size())
        {
            A = l[k];
            if(A.type=='i')
            {
               FOO.x = A.i;
               A.type = 'm';
               int j = 0;
               while(j<4)
               {
                   A.i = (unsigned char)(FOO.bytes[j]);
                   res.push_back(A);
                   j+=1;
               }
            }
            else if(A.type=='b')
            {
               A.type = 'm';
               res.push_back(A);
            }
            else if(A.type=='l')
            {
               FOO1.l = A.l;
               A.type = 'm';
               int j = 0;
               while(j<8)
               {
                   A.i = (unsigned char)(FOO1.bytes[j]);

                   res.push_back(A);
                   j+=1;
               }
            }
            else if(A.type=='s')
            {
                string s = *(string*)A.ptr;
                size_t j = 0;
                A.type = 'm';
                while(j<s.length())
                {
                    A.i = (unsigned char)(s[j]);
                    res.push_back(A);
                    j+=1;
                }
                A.i = (unsigned char)(0);
                res.push_back(A);
            }
            else if(A.type=='f')
            {
               FOO2.f = A.f;
               A.type = 'm';
               size_t j = 0;
               while(j<4)
               {
                   A.i = (unsigned char)(FOO2.bytes[j]);
                   res.push_back(A);
                   j+=1;
               }
            }
            else
            {

            }
            k+=1;
        }
        return res;
}

bool validateArgTypes(string name,string e,PltObject* args,int argc,PltObject& x)
{
    if(e.length()!=(size_t)argc)
    {
      x = Plt_Err(ARGUMENT_ERROR,"Error "+name+"() takes "+str((long long int)e.length())+" arguments!");
      return false;
    }
    for(int f=1;f<=argc;f++)
    {
        PltObject k = args[f-1];
        if(k.type!=e[f-1])
            {
           x = Plt_Err(TYPE_ERROR,"Error argument "+str(f)+" of "+name+"() should be of type "+fullform(e[f-1]));
            return false;
            }
        
    }
    return true;
}
PltObject ISALPHA(PltObject* args,int argc)
{
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error isalpha() takes one argument!");
  if(args[0].type!='s')
    return Plt_Err(TYPE_ERROR,"Error isalpha() takes a string argument!");
  string s = *(string*)args[0].ptr;
  PltObject ret;
  ret.type = 'b';
  ret.i = 0;
  for(auto e: s)
  {
    if(!isalpha(e))
      return ret;
  }
  ret.i = 1;
  return ret;
}
PltObject ADDR(PltObject* args,int argc)
{
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error addr() takes one argument!");
  if(args[0].type!='s' && args[0].type!='o' && args[0].type!='y' && args[0].type!='v' && args[0].type!='w')
    return Plt_Err(TYPE_ERROR,"Error addr() function not suppported for type "+fullform(args[0].type));
  printf("%p\n",args[0].ptr);
  PltObject ret;
  return ret;
}
PltObject ASCII(PltObject* args,int argc)
{
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error ascii() takes one argument!");
  if(args[0].type!='s')
    return Plt_Err(TYPE_ERROR,"Error ascii() takes a string argument!");
  string s = *(string*)args[0].ptr;
  if(s.length()!=1)
      return Plt_Err(VALUE_ERROR,"Error ascii() takes a string argument of length 1!");
 
  PltObject ret;
  ret.type = 'i';
  ret.i = s[0];
  return ret;
}
PltObject TOCHAR(PltObject* args,int argc)
{
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error char() takes one argument!");
  if(args[0].type!='i')
    return Plt_Err(TYPE_ERROR,"Error char() takes an integer argument!");
  char ch = args[0].i;
  string s;
  s+=ch;
  PltObject ret;
  string* p = allocString();
  *p = s;
  return PltObjectFromStringPtr(p);
}
string unescape(string);
void printDictionary(Dictionary* l,vector<void*> seen = {})
{
  if(std::find(seen.begin(),seen.end(),(void*)l)!=seen.end())
  {
    printf("{...}");
    return;
  }
  seen.push_back((void*)l);
  Dictionary d = *l;
  PltObject val;
  size_t k = l->size();
  size_t i = 0;
  printf("{");
  for(auto e: d)
  {
    void printList(PltList*,vector<void*> = {});

    if(e.first.type=='j')
      printList((PltList*)e.first.ptr,seen);
    else if(e.first.type=='a')
      printDictionary((Dictionary*)e.first.ptr,seen);
    else if(e.first.type=='s')
    {
      printf("\"%s\"",unescape(*(string*)e.first.ptr).c_str());
    }
    else
      printf("%s",PltObjectToStr(e.first).c_str());
    printf(" : ");
    //
    if(e.second.type=='j')
      printList((PltList*)e.second.ptr,seen);
    else if(e.second.type=='a')
      printDictionary((Dictionary*)e.second.ptr,seen);
    else if(e.second.type=='s')
    {
      printf("\"%s\"",unescape(*(string*)e.second.ptr).c_str());
    }
    else
      printf("%s",PltObjectToStr(e.second).c_str());
    if(i!=k-1)
      printf(",");
    i+=1;
  }
  printf("}");
}
void printList(PltList* l,vector<void*> seen = {})
{
  if(std::find(seen.begin(),seen.end(),(void*)l)!=seen.end())
  {
    printf("[...]");
    return;
  }
  seen.push_back((void*)l);
  size_t k = l->size();
  PltObject val;
  printf("[");
  for(size_t i=0;i<k;i+=1)
  {
    val = l->at(i);
    if(val.type=='j')
      printList((PltList*)val.ptr,seen);
    else if(val.type=='a')
      printDictionary((Dictionary*)val.ptr,seen);
    else if(val.type=='s')
    {
      printf("\"%s\"",unescape(*(string*)val.ptr).c_str());
    }
    else
    {
      printf("%s",PltObjectToStr(val).c_str());
    }
    if(i!=k-1)
      printf(",");
  }
  printf("]");
}
PltObject print(PltObject* args,int argc)
{
    int k = 0;
    while(k<argc)
    {
        if(args[k].type=='j')
           printList((PltList*)args[k].ptr);
        else if(args[k].type=='a')
           printDictionary((Dictionary*)args[k].ptr);
        else
          printf("%s",PltObjectToStr(args[k]).c_str());
        k+=1;
    }
    PltObject ret;
    ret.type = 'n';
    return ret;
}
PltObject fninfo(PltObject* args,int argc) //for debugging purposes
{
    if(argc!=1)
    {
      return Plt_Err(ARGUMENT_ERROR,"1 argument needed!");
    }
    if(args[0].type=='y')
    {
      NativeFunction* p = (NativeFunction*)args[0].ptr;
      printf("name: %s\n",p->name.c_str());
      if(p->klass!=NULL)
      printf("binded to class %s\n",p->klass->name.c_str());
    }
    else if(args[0].type=='w')
    {
      FunObject* p = (FunObject*)args[0].ptr;
           printf("name: %s\n",p->name.c_str());
      if(p->klass!=NULL)
      printf("binded to class %s\n",p->klass->name.c_str());
    }
    PltObject ret;
    ret.type = 'n';
    return ret;
}
void printList(PltList);
PltObject println(PltObject* args,int argc)
{
    int k = 0;
    while(k<argc)
    {
        if(args[k].type=='j')
          printList((PltList*)args[k].ptr);
        else if(args[k].type=='a')
          printDictionary((Dictionary*)args[k].ptr);
        else
          printf("%s",PltObjectToStr(args[k]).c_str());
        k+=1;
    }
    printf("\n");
    PltObject ret;
    return ret;
}

PltObject input(PltObject* args,int argc)
{
    if(argc!=0 && argc!=1)
    {
        return Plt_Err(ARGUMENT_ERROR,"Error input() takes 1 or 0 arguments!");
    }
    if(argc==1)
    {
      if(args[0].type!='s')
        return Plt_Err(TYPE_ERROR,"Error input() takes a string argument!");
      string& prompt = *(string*)args[0].ptr;
      printf("%s",prompt.c_str());
    }
    PltObject ret;
    string s;
    char ch;
    while(true)
    {
        ch = fgetc(stdin);
        if(ch=='\n')
        {
            break;
        }

          s+=ch;
    }
    string* p = allocString();
    *p = s;
    return PltObjectFromStringPtr(p);
}
PltObject TYPEOF(PltObject* args,int argc)
{
  if(argc!=1)
  {
      return Plt_Err(TYPE_ERROR,"Error typeof() takes one argument only!");
  }
  string fullform(char);
  string* p = allocString();
  *p = fullform(args[0].type);
  return PltObjectFromStringPtr(p);
}
PltObject isInstanceOf(PltObject* args,int argc)
{
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error function isInstanceOf() takes 2 arguments!");
  if(args[1].type!='v')
    return Plt_Err(TYPE_ERROR,"Error second argument to  isInstanceOf() should be a class!");
  PltObject ret;
  ret.type = 'b';
  if(args[0].type!='o')
  {
    ret.i = 0;
    return ret;
  }
  KlassInstance* obj = (KlassInstance*)args[0].ptr;
  Klass* k = (Klass*)args[1].ptr;
  ret.i = obj->klass == k;
  return ret;
}
PltObject LEN(PltObject* args,int argc)
{
    if(argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error len() takes one argument!");

    PltObject ret;
    ret.type = 'i';
    if(args[0].type=='s')
    {
      string& s = *(string*)args[0].ptr;
      ret.i = s.length();
    }
    else if(args[0].type=='j')
        ret.i = ((PltList*)args[0].ptr)->size();
    else if(args[0].type=='a')
        ret.i = ((Dictionary*)args[0].ptr)->size();
    else
        return Plt_Err(TYPE_ERROR,"Error len() unsupported for type "+fullform(args[0].type));
    return ret;
}
//////////
PltObject OPEN(PltObject* args,int argc)
{
    string patt = "ss";
    PltObject ret;
    if(!validateArgTypes("open",patt,args,argc,ret))
      return ret;
    string& filename = *(string*)args[0].ptr;
    string& mode = *(string*)args[1].ptr;
    if(mode!="r" && mode!="w" && mode!="a" && mode!="rw" && mode!="rw+" && mode!="rb" && mode!="wb")
        return Plt_Err(VALUE_ERROR,"Error unknown mode: \""+mode+"\"");
    FILE* fp = fopen(filename.c_str(), mode.c_str());
    if(!fp)
    {

        return Plt_Err(FILE_OPEN_ERROR,strerror(errno));
    }
    FileObject* f = allocFileObject();
    f->fp = fp;
    f->open = true;
    ret.type = 'u';
    ret.ptr = (void*)f;
    return ret;
}
PltObject READ(PltObject* args,int argc)
{
    if(argc!=2 && argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error read() function takes one or two arguments!");
    if(args[0].type!='u')
        return Plt_Err(TYPE_ERROR,"Error read() needs a file stream to read from!");
    char delim = EOF;
    if(argc==2)
    {
      if(args[1].type!='s')
        return Plt_Err(TYPE_ERROR,"Error argument 2 to read() function should be of type string!");
      string& l = *(string*)args[1].ptr;
      if(l.length()!=1)
        return Plt_Err(VALUE_ERROR,"Error optional delimeter argument to read() should be string of length 1");
      delim = l[0];  
    }
    FileObject fobj = *(FileObject*)args[0].ptr;
    if(!fobj.open)
      return Plt_Err(VALUE_ERROR,"Error the file stream is closed!");
    FILE* fp = fobj.fp;
    if(!fp)
        return Plt_Err(FILEIO_ERROR,"Error can't read from a closed file stream!");
    char ch;
    string* p = allocString();
    while((ch = fgetc(fp))!=EOF)
    {
        if(ch==delim)
          break;
        (*p)+=ch;
    }
    return PltObjectFromStringPtr(p);
}
PltObject CLOSE(PltObject* args,int argc)
{
    if(argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error close() takes 1 argument!");
  //  printf("args[0].type = %c\n",args[0].type);
    if(args[0].type!='u')
        return Plt_Err(TYPE_ERROR,"Error close() takes a file stream as an argument!");
    FileObject* fobj = (FileObject*)args[0].ptr;
    if(!fobj->open)
    {
      return Plt_Err(VALUE_ERROR,"Error file already closed!");
    }
    FILE* f = fobj->fp;
    if(f==stdin || f==stdout)
      return Plt_Err(VALUE_ERROR,"Are you nuts?You should not close stdin or stdout!");
    fclose(f);
    PltObject ret;
    ret.type='n';
    fobj->open = false;
    return ret;
}
PltObject RAND(PltObject* args,int argc)
{
    if(argc!=0)
        return Plt_Err(ARGUMENT_ERROR,"Error rand() takes 0 arguments!");
    PltObject ret;
    ret.i = rand();
    ret.type= 'i';
    return ret;
}
//////////////


PltObject WRITE(PltObject* args,int argc)
{
  //printf("writing %s\n",args[0].s.c_str());
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error write() takes two arguments!");
  string patt = "su";
  PltObject ret;
  if(!validateArgTypes("write",patt,args,argc,ret))
    return ret;
  string& data = *(string*)args[0].ptr;
                  FileObject* p = (FileObject*)args[1].ptr;
                FILE* fp = p->fp;
  if(fputs(data.c_str(),fp)==EOF)
    return Plt_Err(FILEIO_ERROR,"Error while writing to file: "+(std::string)strerror(errno));
  ret.type = 'n';
  //printf("done\n");
  return ret;
}
PltObject EXIT(PltObject* args,int argc)
{
    if(argc!=0)
        return Plt_Err(ARGUMENT_ERROR,"Error exit() takes no arguments!");
    exit(0);

}
////////////////
PltObject REVERSE(PltObject* args,int argc)
{
    if(argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error reverse() takes 1 argument!");
    const PltObject& q  = args[0];
    if(q.type!='s' && q.type!='j')
        return Plt_Err(TYPE_ERROR,"Error reverse() takes a string or list argument!");
    if(q.type=='s')
    {
        string* l = allocString();
        string& data = *(string*)q.ptr;
        for(int k=data.length()-1;k>=0;k--)
        {
            *l+=data[k];
        }
        return PltObjectFromStringPtr(l);
    }
    PltList l;
    PltList currList = *(PltList*)q.ptr;
    for(int k =currList.size()-1;k>=0;k--)
    {
        l.push_back(currList[k]);
    }
    PltObject ret;// = l;
    PltList* p = allocList();
    *p = l;
    ret.type = 'j';
    ret.ptr = (void*)p;
    return ret;
}///////
/////////
PltObject BYTES(PltObject* args,int argc)
{
       if(argc!=1)
            return Plt_Err(ARGUMENT_ERROR,"Error toBytes() takes one argument!");
        PltList l;
        const PltObject& e = args[0];
        if(e.type=='i')
        {
            l.push_back(e);
            l = ListToByteList(l);
            PltObject ret;
          ret.type = 'j';
          PltList* p = allocList();
          *p = l;
          ret.ptr = (void*)p;
          return ret;
        }
        else if(e.type=='l')
        {
            l.push_back(e);
            l = ListToByteList(l);
            PltObject ret;
          ret.type = 'j';
          PltList* p = allocList();
          *p = l;
          ret.ptr = (void*)p;
            return ret;
        }
        else if(e.type=='f')
        {
            l.push_back(e);
            l = ListToByteList(l);
            PltObject ret;
           ret.type = 'j';
          PltList* p = allocList();
          *p = l;
          ret.ptr = (void*)p;
            return ret;
        }
        else if(e.type=='s')
        {
            l.push_back(e);
            l = ListToByteList(l);
            PltObject ret;
          ret.type = 'j';
          PltList* p = allocList();
          *p = l;
          ret.ptr = (void*)p;
          return ret;
        }
        else if(e.type=='b')
        {
            l.push_back(e);
            l = ListToByteList(l);
            PltObject ret;
          ret.type = 'j';
          PltList* p = allocList();
          *p = l;
          ret.ptr = (void*)p;
            return ret;
        }
        else
            {
                return Plt_Err(TYPE_ERROR,"Error cannot convert "+fullform(e.type)+" type to bytes!");
            }
}
///////////////
PltObject OBJINFO(PltObject* args,int argc)
{
    if(argc!=1)
      return Plt_Err(ARGUMENT_ERROR,"Error obj_info() takes 1 argument!");
    if( args[0].type=='o')
    {
      KlassInstance* k = (KlassInstance*)args[0].ptr;
      for(auto e: k->members)
      {
            printf("%s: %s\n",e.first.c_str(),fullform(e.second.type).c_str());
          
      }
      for(auto e: k->privateMembers)
      {
          printf("%s: %s\n",e.first.c_str(),fullform(e.second.type).c_str());
      }
      PltObject ret;
      return ret;
    }
    else
    {
        return Plt_Err(TYPE_ERROR,"Error argument is not an instance of any class!");
    }
}
PltObject makeList(PltObject* args,int argc)
{
        PltObject ret;
        if(!validateArgTypes("makeList","js",args,argc,ret))
          return ret;
        string& pattern = *(string*)args[1].ptr;
        size_t k = 0;
        PltList res;
        size_t i = 0;

        PltList currList = *(PltList*)args[0].ptr;
        while(k<currList.size())
        {
            if(i>pattern.length()-1)
            {
               return Plt_Err(VALUE_ERROR,"Error no pattern specified for the remaining bytes!");
            }
            PltObject l = currList[k];
            if(l.type!='m')
            {
                return Plt_Err(VALUE_ERROR,"Error list should only contain bytes!");
            }
            int b = l.i;
            if(pattern[i]=='i')
            {
             //  exit(0);
               FOO.bytes[0] = b;
               if(k+3 >=currList.size())
               {
                   return Plt_Err(VALUE_ERROR,"Error the list is missing some bytes!");
               }
               k+=1;
               l = currList[k];
               FOO.bytes[1] = l.i;
               k+=1;
               l = currList[k];
               FOO.bytes[1+1] = l.i;
               k+=1;
               l = currList[k];
               FOO.bytes[3] = l.i;
               PltObject e;
               e.type = 'i';
               e.i =(int) FOO.x;
               res.push_back(e);
            }
            else if(pattern[i]=='b')
            {

               PltObject e;
               e.type = 'b';
               e.i = b;
               res.push_back(e);
            }
            else if(pattern[i]=='l')
            {
               FOO1.bytes[0] = b;
               if(k+7 >=currList.size())
               {
                   return Plt_Err(VALUE_ERROR,"Error the list is missing some bytes!");
               }
               k+=1;
               l = currList[k];
               FOO1.bytes[1] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[1+1] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[3] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[4] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[5] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[6] = l.i;
               k+=1;
               l = currList[k];
               FOO1.bytes[7] = l.i;
               PltObject e;
               e.type = 'l';
               e.l = FOO1.l;
               res.push_back(e);
            }
            else if(pattern[i]=='s')
            {
                size_t j = k;
                PltObject e;
                string* f = allocString();
                bool terminated = false;
                while(true)
                {
                    if(j>=currList.size())
                    {
                        return Plt_Err(VALUE_ERROR,"Ran out of bytes!");
                    }
                    e = currList[j];
                    if(e.type!='m')
                        return Plt_Err(VALUE_ERROR,"Error the list should only contain bytes!");
                    if(e.i==0)
                    {
                        terminated = true;
                        break;
                    }
                    *f+=(char)e.i;
                    j+=1;
                }
                if(!terminated)
                {
                    return Plt_Err(VALUE_ERROR,"Error the bytes are invalid to be converted to string!");
                }
                res.push_back(PltObjectFromStringPtr(f));
                k  = j;
            }
            else if(pattern[i]=='f')
            {
               FOO2.bytes[0] = b;
               if(k+3 >=currList.size())
               {
                   return Plt_Err(VALUE_ERROR,"Error the list is missing some bytes!");
               }
               k+=1;
               l = currList[k];
               FOO2.bytes[1] = l.i;
               k+=1;
               l = currList[k];
               FOO2.bytes[1+1] = l.i;
               k+=1;
               l = currList[k];
               FOO2.bytes[3] = l.i;
               PltObject e;
               e.type = 'f';
               e.f = FOO2.f;
               res.push_back(e);
            }
            else
            {
                return Plt_Err(TYPE_ERROR,"Error unknown type used in pattern!");
            }
            i+=1;
            k+=1;
        }
        if(i!=pattern.length())
        {
            return Plt_Err(VALUE_ERROR,"Error the list does not have enough bytes to follow the pattern!");
        }

        PltList* p = allocList();
        *p =  (res);
        ret.ptr = (void*)p;
        ret.type = 'j';

        return ret;
}
///////
/////////

PltObject SUBSTR(PltObject* args,int argc)
{
    if(argc==3)
		{
			if(args[0].type!='i' && args[0].type!='l')
        return Plt_Err(TYPE_ERROR,"Error first argument of substr() should be an integer");
      if(args[1].type!='i' && args[1].type!='l')
        return Plt_Err(TYPE_ERROR,"Error second argument of substr() should be an integer");
			if(args[2].type=='s')
			{
        string* q = allocString();
        if(args[0].i<0 || args[1].i<0 )
        {
           return PltObjectFromStringPtr(q);
        }
        string& data = *(string*)args[2].ptr;
        *q  = substr(args[0].i,args[1].i,data);
        return PltObjectFromStringPtr(q);
			}
			else
      {
      return Plt_Err(TYPE_ERROR,"Error third argument of substr() should be a string!\n");
      }
		}
		return Plt_Err(ARGUMENT_ERROR,"Error substr() takes three arguments!");
}
PltObject getFileSize(PltObject* args,int argc)
{
     if(argc==1)
        {
            if(args[0].type!='u')
                return Plt_Err(TYPE_ERROR,"Error getFileSize() takes an open file as argument!");
          
            FileObject* p = (FileObject*)args[0].ptr;
            if(!p->open)
              return Plt_Err(FILEIO_ERROR,"Unable to get size of closed file!");
            FILE* currF = p->fp;
            fseek(currF,0,SEEK_END);
            long long int n = ftell(currF);
            rewind(currF);
            PltObject ret;
            ret.type = 'l';
            ret.l = (long long int)n;
            return ret;
        }
            return Plt_Err(ARGUMENT_ERROR,"Error getFileSize() takes 1 argument!");
}
PltObject FTELL(PltObject* args,int argc)
{
     if(argc==1)
        {
            if(args[0].type!='u')
                return Plt_Err(TYPE_ERROR,"Error ftell() takes an open file as argument!");
            FileObject* p = (FileObject*)args[0].ptr;
            if(!p->open)
              return Plt_Err(FILEIO_ERROR,"Error file is closed!");
            FILE* currF = p->fp;
            long long int n = ftell(currF);
            PltObject ret;
            ret.type = 'l';
            ret.l = (long long int)n;
            return ret;
        }
            return Plt_Err(ARGUMENT_ERROR,"Error ftell() takes 1 argument!");
}
PltObject REWIND(PltObject* args,int argc)
{
     if(argc==1)
        {
            if(args[0].type!='u')
                return Plt_Err(TYPE_ERROR,"Error rewind() takes an open file as argument!");
          
            FileObject* p = (FileObject*)args[0].ptr;
            if(!p->open)
              return Plt_Err(FILEIO_ERROR,"Unable to rewind closed file!");
            FILE* currF = p->fp;
            rewind(currF);
            PltObject ret;
            return ret;
        }
            return Plt_Err(ARGUMENT_ERROR,"Error rewind() takes 1 argument!");
}
PltObject SYSTEM(PltObject* args,int argc)
{
    if(argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error system() takes 1 argument!");
    if(args[0].type!='s')
        return Plt_Err(TYPE_ERROR,"Error system() takes a string argument!");
    string& command = *(string*)args[0].ptr;
    system(command.c_str());
    PltObject ret;
    return ret;
}
PltObject SPLIT(PltObject* args,int argc)
{
    if(argc==2)
		{
			if(args[0].type=='s' && args[1].type=='s')
			{
        string& data = *(string*)args[0].ptr;
        string& delim = *(string*)args[1].ptr;
				vector<string> list = split(data,delim);
				unsigned int  o = 0;
				PltList l;
				string* value;
				while(o<list.size())
        {
          value = allocString();
          *value = list[o];
          l.push_back(PltObjectFromStringPtr(value));
          o+=1;
        }
        PltList* p =new PltList(l);
        PltObject ret;
        ret.type = 'j';
        ret.ptr = p;
        return ret;
			}
			else
      {
        return Plt_Err(TYPE_ERROR,"Error split() takes both string arguments!\n");
        exit(0);
      }
		}
		return Plt_Err(ARGUMENT_ERROR,"Error split() takes two arguments!");
}
PltObject GETENV(PltObject* args,int argc)
{
        if(argc==1)
        {

            if(args[0].type=='s')
            {
              string& vname = *(string*)args[0].ptr;
              char* c = getenv(vname.c_str());
              if(!c)
              {
               //   return Plt_Err(NAME_ERROR,"Unknown environment variable!\n");
               PltObject ret;
               return ret;
              }
              string* s = allocString();
              *s = c;

              return PltObjectFromStringPtr(s);
            }
            else
            {
              return Plt_Err(TYPE_ERROR,"Error getenv() takes a string argument!");
            }
        }
        return Plt_Err(ARGUMENT_ERROR,"Error getenv() takes one argument!");
}
PltObject SHUFFLE(PltObject* args,int argc)
{
    if(argc==1)
		{
			if(args[0].type=='j')
			{
			    PltList a = *(PltList*)args[0].ptr;
				std::random_shuffle(a.begin(),a.end());
				PltObject ret;
				return ret;
			}
			else
      {
        return Plt_Err(TYPE_ERROR,"Error shuffle takes a list as an argument!");
        exit(0);
      }
		}
		return Plt_Err(ARGUMENT_ERROR,"Error shuffle() takes exactly one argument!");
}
PltObject STR(PltObject* args,int argc)
{
    	if(argc==1)
		{
			if(args[0].type=='i')
			{
                string* s = allocString();
                *s = str(args[0].i);
                return PltObjectFromStringPtr(s);
			}
			else if(args[0].type=='f')
			{
                string* s = allocString();
                *s = str(args[0].f);
                return PltObjectFromStringPtr(s);
			}
            else if(args[0].type=='l')
            {
                string* s = allocString();
                *s = str(args[0].l);
                return PltObjectFromStringPtr(s);
            }
            return Plt_Err(TYPE_ERROR,"Error str() takes a numeric argument!");
		}
        return Plt_Err(ARGUMENT_ERROR,"Error str() takes only one argument!");
		exit(0);
}
PltObject FIND(PltObject* args,int argc)
{
  PltObject ret;
  if(!validateArgTypes("find","ss",args,argc,ret))
    return ret;  
  ret.type = 'l';
  string& a = *(string*)args[0].ptr;
  string& b = *(string*)args[1].ptr;
  auto y = b.find(a); 
  if(y==std::string::npos)
  {
    ret.type = 'n';
    return ret;
  }
  else
    ret.l = static_cast<long long int>(y);
  return ret;
}
PltObject TOINT(PltObject* args,int argc)
{
    if(argc==1)
		{

			if(args[0].type=='s')
			{
			    string& q = *(string*)args[0].ptr;
			    PltObject ret;
                if(isnum(q))
                {
                    ret.i = Int(q);
                    ret.type = 'i';
                }
                else if(isInt64(q))
                {
                    ret.l = toInt64(q);
                    ret.type = 'l';
                }
                else if(isaFloat(q))
                {
                    q = q.substr(0,q.find('.'));
                    ret.i = (Int(q));
                    ret.type = 'i';
                }
                else
                    return Plt_Err(VALUE_ERROR,"Error the string "+q+" cannot be converted to an integer!");
				return ret;
			}
			else if(args[0].type=='f')
			{
               PltObject ret;
               double d = args[0].f;
               if(d<(double)LLONG_MIN)
                 ret.l = LLONG_MIN;
               else if(d>(double)LLONG_MAX)
                 ret.l = LLONG_MAX;
               else 
                 ret.l = static_cast<long long int>(args[0].f);
               ret.type = 'l';
               return ret;
			}
        return Plt_Err(TYPE_ERROR,"Error int() unsupported for type "+fullform(args[0].type));
		exit(0);
		}
		return Plt_Err(ARGUMENT_ERROR,"Error int() takes exactly one argument!");
		exit(0);
}
PltObject TOFLOAT(PltObject* args,int argc)
{
    if(argc==1)
		{
			if(args[0].type=='s')
			{
			    string& q = *(string*)args[0].ptr;
			    if(!isaFloat(q))
                {
                    return Plt_Err(VALUE_ERROR,"The string cannot be converted to a float!\n");
                }
                    PltObject ret;
                    ret.type = 'f';
                    ret.f = Float(q);
                    return ret;

			}
        return Plt_Err(TYPE_ERROR,"Error float() takes a string argument!");
		exit(0);
		}
		return Plt_Err(ARGUMENT_ERROR,"Error float() takes exactly one argument!");
		exit(0);
}
PltObject tonumeric(PltObject* args,int argc)
{
    if(argc==1)
    {
            if(args[0].type=='s')
            {
               string& q = *(string*)args[0].ptr;
               if(isnum(q))
               {
                   PltObject ret;
                   ret.type='i';
                   ret.i = Int(q);
                   return ret;
               }
               else if(isInt64(q))
               {
                   PltObject ret;
                   ret.type = 'l';
                   ret.l = toInt64(q);
                   return ret;
               }
               else if(isaFloat(q))
               {
                   PltObject ret;
                   ret.type = 'f';
                   ret.f = Float(q);
                   return ret;
               }
               else
               {
                   return Plt_Err(VALUE_ERROR,"Error cannot convert the string \""+q+"\" to numeric type!");
               }
            }
            else
            {
                return Plt_Err(TYPE_ERROR,"Error tonumeric() takes a string argument!");
            }
		}
        return Plt_Err(ARGUMENT_ERROR,"Error tonumeric() takes one argument!");
}
PltObject isnumeric(PltObject* args,int argc)
{
  PltObject ret;
  ret.type = 'b';
  ret.i = 1;
    if(argc==1)
    {
            if(args[0].type=='s')
            {
               string& s = *(string*)args[0].ptr;
               if(isnum(s))
                   return ret;
               else if(isInt64(s))
                   return ret;
               else if(isaFloat(s))
                   return ret;
               else
               {
                 ret.i = 0;
                   return ret;
               }
            }
            else
            {
                return Plt_Err(TYPE_ERROR,"Error isnumeric() takes a string argument!");
            }
    }
    return Plt_Err(ARGUMENT_ERROR,"Error isnumeric() takes 1 argument!");
}
PltObject REPLACE(PltObject* args,int argc)
{
        if(argc==3)
        {
           if(args[0].type!='s')
               return Plt_Err(TYPE_ERROR,"Error first argument given to replace() must be a string!");
           if(args[1].type!='s')
               return Plt_Err(TYPE_ERROR,"Error second argument given to replace() must be a string!");
           if(args[2].type!='s')
               return Plt_Err(TYPE_ERROR,"Error third argument given to replace() must be a string!");
           string& a = *(string*)args[0].ptr;
           string& b = *(string*)args[1].ptr;
           string& c = *(string*)args[2].ptr;
           
           string* z = allocString();
           *z = replace_all(a,b,c);
           return PltObjectFromStringPtr(z);
        }
        else
        {
            return Plt_Err(ARGUMENT_ERROR,"Error replace() takes three arguments\n");
        }
}
PltObject REPLACE_ONCE(PltObject* args,int argc)
{
        if(argc==3)
        {
           if(args[0].type!='s')
               return Plt_Err(TYPE_ERROR,"Error first argument given to replace_once() must be a string!");
           if(args[1].type!='s')
               return Plt_Err(TYPE_ERROR,"Error second argument given to replace_once() must be a string!");
           if(args[2].type!='s')
               return Plt_Err(TYPE_ERROR,"Error third argument given to replace_once() must be a string!");
           string& a = *(string*)args[0].ptr;
           string& b = *(string*)args[1].ptr;
           string& c = *(string*)args[2].ptr;
           string* z = allocString();
           *z = replace(a,b,c);
           return PltObjectFromStringPtr(z);
        }
        else
        {
            return Plt_Err(ARGUMENT_ERROR,"Error replace_once() takes three arguments\n");
        }
}
PltObject SLEEP(PltObject* args,int argc)
{
    if(argc!=1)
        return Plt_Err(ARGUMENT_ERROR,"Error sleep() takes 1 argument!");
    if(args[0].type=='i' && args[0].type!='l')
    {
      PltObject r = args[0];
      PromoteType(r,'l');
      #ifdef BUILD_FOR_WINDOWS
      #include <windows.h>
         Sleep(r.l);
      #else
        usleep(r.l*1000);
      #endif
      PltObject ret;
      return ret;
    }
    else
    {
      return Plt_Err(TYPE_ERROR,"Error sleep() takes an integer argument!");
    }
}

PltObject datetime(PltObject* args,int argc)
{
    if(argc!=0)
        return Plt_Err(ARGUMENT_ERROR,"Error datetime() takes 0 arguments!");
    time_t now = time(0);
    string*  q = allocString();
    *q = ctime(&now);
    return PltObjectFromStringPtr(q);
}
PltObject ByteToInt(PltObject* args,int argc)
{
     if(argc!=1)
        {
            return Plt_Err(VALUE_ERROR,"Error ByteToInt() takes 1 argument!");
        }
        if(args[0].type!='m')
        {
            return Plt_Err(TYPE_ERROR,"First argument of ByteToInt() should be a byte!");
        }
        unsigned char x = args[0].i;
        PltObject ret;
        ret.type = 'i';
        ret.i = (int)x;
        return ret;
}
PltObject IntToByte(PltObject* args,int argc)
{
     if(argc!=1)
        {
            return Plt_Err(ARGUMENT_ERROR,"Error IntToByte() takes 1 argument!");
        }
        if(args[0].type!='i')
        {
            return Plt_Err(TYPE_ERROR,"First argument of byteToDecimal should be an integer!");
        }
        if(args[0].i>255 || args[0].i<0)
        {
            return Plt_Err(VALUE_ERROR,"The integer must be in range 0 to 255!");
        }
        PltObject ret;
        ret.type = 'm';
        ret.i = (unsigned char)args[0].i;
        return ret;
}
PltObject writelines(PltObject* args,int argc)
{
     if(argc==2)
        {
            if(args[0].type!='j')
                return Plt_Err(TYPE_ERROR,"Error first argument of writelines() should be a list!");
            if(args[1].type=='u')
            {
               // fputs(data.c_str(),currF);
                PltList lines = *(PltList*)args[0].ptr;
                unsigned int f = 0;
                string data = "";
                PltObject m;
                while(f<lines.size())
                {
                    m = (lines[f]);
                    if(m.type!='s')
                    {
                        return Plt_Err(VALUE_ERROR,"List provided to writelines should consist of string elements only!");
                    }
                    data+=*(string*)m.ptr;
                    if(f!=lines.size()-1)
                    {
                        data+="\n";
                    }
                    f+=1;
                }
                                FileObject* p = (FileObject*)args[1].ptr;
                FILE* currF = p->fp;
                fputs(data.c_str(),currF);
                PltObject ret;
                return ret;
            }
            return Plt_Err(ARGUMENT_ERROR,"Error writelines() needs a filestream to write!");
        }
        else
        {
            return Plt_Err(VALUE_ERROR,"Error writelines() takes two arguments!");
            exit(0);
        }
}
PltObject readlines(PltObject* args,int argc)
{
    if(argc==1)
    {
      if(args[0].type!='u')
          return Plt_Err(TYPE_ERROR,"Error first argument given to readlines() must be a filestream!");
      char ch;
      FileObject fobj = *(FileObject*)args[0].ptr;
      if(!fobj.open)
        return Plt_Err(VALUE_ERROR,"Error the file stream is closed!");
      FILE* currF = fobj.fp;
      // bool done;
      PltList lines;
      string* reg = allocString();
      lines.push_back(PltObjectFromStringPtr(reg));
      int k = 0;

      while(true)
      {
          // done =true;
          ch = fgetc(currF);
          if(ch==EOF)
          {
              break;
          }
          else if(ch=='\n')
          {
              k+=1;
              reg = allocString();
              lines.push_back(PltObjectFromStringPtr(reg));
          }
          else
          {
            *reg+=ch;
          }
      }
      PltObject ret;
      PltList* p = allocList();
      *p = lines;
      ret.type = 'j';
      ret.ptr = (void*)p;
      return ret;
    }
    else
      {
          return Plt_Err(ARGUMENT_ERROR,"Error readlines() takes one argument!");
          exit(0);
      }
}
void clean_stdin(void)
{
    int c = 0;
    while(c!='\n' && c!=EOF) 
    {
        c = getchar();
    }
}
PltObject FREAD(PltObject* args,int argc)
{
    if(argc!=2)
        return Plt_Err(ARGUMENT_ERROR,"Error fread() takes two arguments!");
    if(args[0].type!='i' && args[0].type!='l')
    {
        return Plt_Err(TYPE_ERROR,"Error first argument of fread() should be an integer!");
    }
    if(args[1].type!='u')
      return Plt_Err(TYPE_ERROR,"Error second argument of fread() should be a file stream!");
    PltObject a = args[0];
    PromoteType(a,'l');
    long long int e = a.l;
 //   printf("e = %lld\n",e);
    unsigned char* bytes = new unsigned char[e];
    FileObject fobj = *(FileObject*)args[1].ptr;
    if(!fobj.open)
      return Plt_Err(VALUE_ERROR,"Error the file stream is closed!");
    FILE* currF = fobj.fp;
    if(fread(bytes,sizeof(unsigned char),e,currF)!=(size_t)e)
        return Plt_Err(FILEIO_ERROR,"Error unable to read specified bytes from the file.");
    /*{
        printf("n = %ld\n",n);
        string what = strerror(errno);
        return Plt_Err("IOERROR",what);
    }*/
    PltList l;
    for(int k=0;k<e;k++)
    {
        PltObject m;
        m.type = 'm';
        m.i = (unsigned char)(bytes[k]);
        l.push_back(m);
    }
    PltObject ret;
    PltList* p = new PltList(l);
    ret.ptr = (void*)p;
    ret.type = 'j';

    delete[] bytes;
    if(currF == stdin)
      clean_stdin();
    return ret;
}
PltObject FWRITE(PltObject* args,int argc)
{
    PltObject ret;
    long long int S = 0;
    if(argc==2)
    {
    if(!validateArgTypes("fwrite","ju",args,argc,ret))
      return ret;
     S = ((PltList*)args[0].ptr)->size(); 
    }
    else if(argc==3)
    {
       if(args[0].type!='j' || args[1].type!='u' || (args[2].type!='i' && args[2].type!='l'))
         return Plt_Err(TYPE_ERROR,"Invalid Argument types");
       if(args[2].type=='i')
         S = args[2].i;
        else
         S = args[2].l;
    }
    else
    {
      return Plt_Err(ARGUMENT_ERROR,"Error fread takes either 2 or 3 arguments");
    }
    PltList l = *(PltList*)args[0].ptr;
    FileObject fobj = *(FileObject*)args[1].ptr;
    if(!fobj.open)
      return Plt_Err(VALUE_ERROR,"Error the file stream is closed!");
    FILE* currF = fobj.fp;
    unsigned char* bytes = new unsigned char[S];
    for(int k=0;k<S;k++)
    {
        PltObject m = l[k];
        if(m.type!='m')
            return Plt_Err(VALUE_ERROR,"Error the list should contain bytes only!");
        bytes[k] = m.i;
    }
    if(fwrite(bytes,sizeof(unsigned char),S,currF)!=(size_t)S)
    {
        string what = strerror(errno);
        return Plt_Err(FILEIO_ERROR,"Error unable to write the bytes to file!");

    }
    delete[] bytes;
    return ret;

}
PltObject FSEEK(PltObject* args,int argc)
{
    if(argc!=3)
        return Plt_Err(ARGUMENT_ERROR,"Error fseek() takes 3 arguments!");
    PltObject ret;
    if(!validateArgTypes("fseek","uii",args,argc,ret))
      return ret;
    int w = 0;
    int whence = args[2].i;
    FileObject fobj = *(FileObject*)args[0].ptr;
    if(!fobj.open)
      return Plt_Err(VALUE_ERROR,"Error the file stream is closed!");
    FILE* currF = fobj.fp;
    int where = args[1].i;
    if(whence==SEEK_SET)
    {
        w = SEEK_SET;
    }
    else if(whence==SEEK_END)
        w = SEEK_END;
    else if(whence==SEEK_CUR)
        w = SEEK_CUR;
    else
        return Plt_Err(VALUE_ERROR,"Error invalid option "+to_string(whence));
    if(fseek(currF,where,w)!=0)
      {
        string what = strerror(errno);
        return Plt_Err(FILE_SEEK_ERROR,what);
      }
    return ret;
}
//
PltList* makeListCopy(PltList);
Dictionary* makeDictCopy(Dictionary);
//
Dictionary* makeDictCopy(Dictionary v)
{
  Dictionary* d = allocDict();
  for(auto e: v)
  {
    PltObject key,val;
    key = e.first;
    val = e.second;
    d->emplace(key,val);
  }
  return d;
}
PltList* makeListCopy(PltList v)
{
  PltList* p = allocList();
  for(auto e: v)
  {
    PltObject elem;
    elem = e;
    p->push_back(elem);
  }
  return p;
}
PltObject COPY(PltObject* args,int argc)
{
  if(argc!=1)
      return Plt_Err(ARGUMENT_ERROR,"Error clone() takes one argument!");
  if(args[0].type=='a')
  {
     Dictionary v = *(Dictionary*)args[0].ptr;
     PltObject ret;
     ret.type = 'a';
     ret.ptr = (void*)makeDictCopy(v);
     return ret;

  }
  else if(args[0].type=='j')
  {
    PltList v = *(PltList*)args[0].ptr;
    PltObject ret;
    ret.type = 'j';
    ret.ptr = (void*)makeListCopy(v);
    return ret;
  }
  else
    return Plt_Err(TYPE_ERROR,"Error clone() takes a list or dictionary as an argument!");
}
PltObject POW(PltObject* args,int argc)
{
    if(argc!=2)
    {
        return Plt_Err(ARGUMENT_ERROR,"pow() takes 2 arguments!");
    }
    PltObject a = args[0];
    PltObject b = args[1];
    PltObject c;
    char t;
    if(isNumeric(a.type) && isNumeric(b.type))
    {
                if(a.type=='f' || b.type=='f')
                    t = 'f';
                else if(a.type=='l' || b.type=='l')
                    t = 'l';
                else if(a.type=='i' || b.type=='i')
                    t = 'i';
                PromoteType(a,t);
                PromoteType(b,t);
    }
    else
    {
                 return Plt_Err(TYPE_ERROR,"Error pow() unsupported for "+fullform(a.type)+" and "+fullform(b.type));
    }

          //
             if(t=='i')
             {
               c.type = 'i';
               if(!exponen_overflows(a.i,b.i))
               {
                 c.i = pow(a.i,b.i);
                 return c;
               }

               if(exponen_overflows((long long int)a.i,(long long int)b.i))
               {
                 return Plt_Err(OVERFLOW_ERROR,"Integer Overflow occurred in pow()");
               }
               c.type = 'l';
               c.l = pow((long long int)(a.i) , (long long int)(b.i));
               return c;
             }
             else if(t=='f')
             {
               if(exponen_overflows(a.f,b.f))
               {
                     return Plt_Err(OVERFLOW_ERROR,"Floating Point Overflow occurred in pow()");
               }
               c.type = 'f';
               c.f = pow(a.f,b.f);
               return c;
             }
             else if(t=='l')
             {
                 if(exponen_overflows(a.l,b.l))
                 {
                     return Plt_Err(OVERFLOW_ERROR,"Integer Overflow occurred in pow().");
                 }
                 c.type = 'l';
                 c.l = pow(a.l,b.l);
                 return c;
             }
        return c;//to avoid warning otherwise this stmt is never executed
}
PltObject CLOCK(PltObject* args,int argc)
{
  if(argc!=0)
    return Plt_Err(ARGUMENT_ERROR,"Error clock() takes 0 arguments!");
  PltObject ret;
  ret.type = 'f';
  ret.f = static_cast<double> (clock());
  return ret;
}
////////////////////
//Builtin Methods
//Methods work exactly like functions except that they except their first argument to
//be an object
PltObject POP(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named pop()");
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error method pop() takes 0 arguments!");
  PltList* p = (PltList*)args[0].ptr;
  PltObject ret;
  if(p->size()!=0)
  {
    ret = p->back();
    p->pop_back();
  }

  return ret;
}
PltObject CLEAR(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named clear()");
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error method clear() takes 0 arguments!");
  PltList* p = (PltList*)args[0].ptr;
  p->clear();
  PltObject ret;
  return ret;
}
PltObject PUSH(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named push()");
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error method push() takes 1 argument!");
  PltList* p = (PltList*)args[0].ptr;
  p->push_back(args[1]);
  PltObject ret;
  return ret;

}
PltObject RESERVE(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named push()");
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error method reserve() takes 1 argument!");
  if(args[1].type!='i' && args[1].type!='l')
    return Plt_Err(TYPE_ERROR,"Error reserve() takes an integer argument!");
  PltList* p = (PltList*)args[0].ptr;
  PltObject x = args[1];
  PromoteType(x,'l');
  p->reserve(x.l);
  PltObject ret;
  return ret;

}
PltObject FINDINLIST(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named find()");
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error method find() takes 1 arguments!");
  PltList* p = (PltList*)args[0].ptr;
  PltObject ret;
  for(size_t k=0;k<p->size();k+=1)
  {
    if((*p)[k]==args[1])
    {
      ret.type = 'i';
      ret.i = k;
      return ret;
    }
  }
  return ret;
}

PltObject INSERT(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named insert()");
  if(argc==3)
  {
    PltList* p = (PltList*)args[0].ptr;
    PltObject idx = args[1];
    PltObject val = args[2];
    if(idx.type!='i' && idx.type!='l')
      return Plt_Err(TYPE_ERROR,"Error method insert() expects an integer argument for position!");
    PromoteType(idx,'l');
    if(idx.l < 0)
      return Plt_Err(VALUE_ERROR,"Error insertion position is negative!");
    if((size_t)idx.l > p->size())
          return Plt_Err(VALUE_ERROR,"Error insertion position out of range!");
    if(val.type=='j')
    {
      PltList sublist = *(PltList*)val.ptr;
      p->insert(p->begin()+idx.l,sublist.begin(),sublist.end());
    }
    else
    {
      p->insert(p->begin()+idx.l,val);
    }
    PltObject ret;
    return ret;
  }
  else
    return Plt_Err(ARGUMENT_ERROR,"Error method insert() takes 2 arguments!");
}
PltObject ERASE(PltObject* args,int argc)
{
  if(args[0].type=='j')
  {
    if(argc!=2 && argc!=3)
      return Plt_Err(ARGUMENT_ERROR,"Error erase() takes 1 or 2 arguments!");
    PltList* p = (PltList*)args[0].ptr;
    PltObject idx1 = args[1];
    PltObject idx2;
    if(argc==3)
     idx2 = args[2];
    else
      idx2 = idx1;
    if(idx1.type!='i' && idx1.type!='l')
        return Plt_Err(TYPE_ERROR,"Error method erase() expects an integer argument for start!");
    if(idx2.type!='i' && idx2.type!='l')
        return Plt_Err(TYPE_ERROR,"Error method erase() expects an integer argument for start!");
    PromoteType(idx1,'l');
    PromoteType(idx2,'l');
    if(idx1.l < 0 || idx2.l < 0)
        return Plt_Err(VALUE_ERROR,"Error index is negative!");
    if((size_t)idx1.l >= p->size() || (size_t)idx2.l >= p->size())
        return Plt_Err(VALUE_ERROR,"Error index out of range!");
    p->erase(p->begin()+idx1.l,p->begin()+idx2.l+1);
    PltObject ret;
    return ret;
  }
  else if(args[0].type=='a')
  {
     if(argc!=2)
       return Plt_Err(ARGUMENT_ERROR,"Error dictionary method erase() takes 1 argument!");
    Dictionary* d = (Dictionary*)args[0].ptr;
    PltObject key = args[1];
    if(key.type!='i' && key.type!='l' && key.type!='f' && key.type!='s' && key.type!='m' && key.type!='b')
      return Plt_Err(TYPE_ERROR,"Error key of type "+fullform(key.type)+" not allowed.");
    if(d->find(key)==d->end())
      return Plt_Err(KEY_ERROR,"Error cannot erase value,key not found in the dictionary!");
    d->erase(key);
    PltObject ret;
    return ret;
  }
  else
  {
      return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no member named erase.");
  }
}
PltObject asMap(PltObject* args,int argc)
{
    if(args[0].type!='j')
      return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no member named asMap()");
    if(argc!=1)
      return Plt_Err(ARGUMENT_ERROR,"Error list member asMap() takes 0 arguments!");
    PltList l = *(PltList*)args[0].ptr;
    PltObject x;
    x.type = 'l';
    size_t i = 0;
    Dictionary* d = allocDict();

    for(;i<l.size();i++)
    {
       x.l = i;
       d->emplace(x,l[i]);
    }
    PltObject ret;
    ret.type = 'a';
    ret.ptr = (void*)d;
    return ret;
}
PltObject ASLIST(PltObject* args,int argc)
{
    if(args[0].type!='a')
      return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no member named asList()");
    if(argc!=1)
      return Plt_Err(ARGUMENT_ERROR,"Error dictionary member asList() takes 0 arguments!");
    Dictionary d = *(Dictionary*)args[0].ptr;
    PltList* list = allocList();
    for(auto e: d)
    {
      PltList* sub = allocList();
      sub->push_back(e.first);
      sub->push_back(e.second);
      PltObject x;
      x.type = 'j';
      x.ptr = (void*)sub;
      list->push_back(x);
    }
    PltObject ret;
    ret.type = 'j';
    ret.ptr = (void*)list;
    return ret;
}
PltObject REVERSELIST(PltObject* args,int argc)
{
  if(args[0].type!='j')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named reverse()");
  if(argc!=1)
    return Plt_Err(ARGUMENT_ERROR,"Error method reverse() takes 0 arguments!");
  PltList* p = (PltList*)args[0].ptr;
  PltList l = *p;
  std::reverse(l.begin(),l.end());
  *p = l;
  PltObject ret;
  return ret;
}
PltObject EMPLACE(PltObject* args,int argc)
{
  if(args[0].type!='a')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named emplace()");
  if(argc!=3)
    return Plt_Err(ARGUMENT_ERROR,"Error method emplace() takes 2 arguments!");
  Dictionary* p = (Dictionary*)args[0].ptr;
  PltObject& key = args[1];
  if(key.type!='i' && key.type!='l' && key.type!='f' && key.type!='s' && key.type!='m' && key.type!='b')
    return Plt_Err(TYPE_ERROR,"Error key of type "+fullform(key.type)+" not allowed.");
  p->emplace(key,args[2]);
  PltObject ret;
  return ret;

}
PltObject HASKEY(PltObject* args,int argc)
{
  if(args[0].type!='a')
         return Plt_Err(NAME_ERROR,"Error type "+fullform(args[0].type)+" has no method named hasKey()");
  if(argc!=2)
    return Plt_Err(ARGUMENT_ERROR,"Error method hasKey() takes 1 argument!");
  Dictionary* p = (Dictionary*)args[0].ptr;

  PltObject ret;
  ret.type= 'b';
  ret.i = (p->find(args[1])!=p->end());
  return ret;

}
////////////////////
///////////////
void initFunctions()
{
  funcs.emplace("print",&print);
  funcs.emplace("println",&println);
  funcs.emplace("input",&input);
  funcs.emplace("typeof",&TYPEOF);
  funcs.emplace("len",&LEN);
  funcs.emplace("open",&OPEN);
  funcs.emplace("read",&READ);
  funcs.emplace("close",&CLOSE);
  funcs.emplace("rand",&RAND);
  funcs.emplace("shuffle",&SHUFFLE);
  funcs.emplace("reverse",&REVERSE);
  funcs.emplace("getenv",&GETENV);
  funcs.emplace("system",&SYSTEM);
  funcs.emplace("readlines",&readlines);
  funcs.emplace("writelines",&writelines);
  funcs.emplace("write",&WRITE);
  funcs.emplace("getFileSize",getFileSize);
  funcs.emplace("fread",&FREAD);
  funcs.emplace("fwrite",&FWRITE);
  funcs.emplace("substr",&SUBSTR);
  funcs.emplace("find",&FIND);
  funcs.emplace("replace_once",&REPLACE_ONCE);
  funcs.emplace("replace",&REPLACE);
  funcs.emplace("sleep",&SLEEP);
  funcs.emplace("exit",&EXIT);
  funcs.emplace("split",&SPLIT);
  funcs.emplace("int",&TOINT);
  funcs.emplace("float",&TOFLOAT);
  funcs.emplace("tonumeric",&tonumeric);
  funcs.emplace("isnumeric",&isnumeric);
  funcs.emplace("datetime",&datetime);
  funcs.emplace("fseek",&FSEEK);
  funcs.emplace("bytes",&BYTES);
  funcs.emplace("ListFromBytes",&makeList);
  funcs.emplace("ByteToInt",&ByteToInt);
  funcs.emplace("str",&STR);
  funcs.emplace("IntToByte",&IntToByte);
  funcs.emplace("clone",&COPY);
  funcs.emplace("pow",&POW);
  funcs.emplace("obj_info",&OBJINFO);
  funcs.emplace("isalpha",&ISALPHA);
  funcs.emplace("ftell",&FTELL);
  funcs.emplace("rewind",&REWIND);
  funcs.emplace("clock",&CLOCK);
  funcs.emplace("isInstanceOf",&isInstanceOf);
  funcs.emplace("ascii",&ASCII);
  funcs.emplace("char",&TOCHAR);
  funcs.emplace("fninfo",&fninfo);
  funcs.emplace("addr",&ADDR);
}
std::unordered_map<string,BuiltinFunc> methods;
void initMethods()
{
  methods.emplace("push",&PUSH);
  methods.emplace("pop",&POP);
  methods.emplace("clear",&CLEAR);
  methods.emplace("insert",&INSERT);
  methods.emplace("find",&FINDINLIST);
  methods.emplace("asMap",&asMap);
  methods.emplace("reserve",&RESERVE);
  methods.emplace("erase",&ERASE);
  methods.emplace("reverse",&REVERSELIST);
  methods.emplace("emplace",&EMPLACE);
  methods.emplace("hasKey",&HASKEY);
  methods.emplace("asList",&ASLIST);

}
PltObject callmethod(string name,PltObject* args,int argc)
{
     if(methods.find(name)==methods.end())
       return Plt_Err(NAME_ERROR,"Error "+fullform(args[0].type)+" type has no method named "+name+"()");
     return methods[name](args,argc);
}
bool function_exists(string name)
{
  if(funcs.find(name)!=funcs.end())
    return true;
  return false;
}
#endif
