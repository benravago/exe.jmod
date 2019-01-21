
#include <dlfcn.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jni.h>

#define YES 1
#define NO 0

void say(const char* fmt, ...) {
  va_list vp;
  va_start(vp,fmt);
  vfprintf(stderr,fmt,vp);
  va_end(vp);
  fputc('\n',stderr);
}

void* alloc(size_t size) {
  void* p = malloc(size);
  if (p) return p;
  perror("out of memory");
  exit(1);
}

char* concat(int argc, ...) {
  int n = 0;
  va_list v;
  va_start(v,argc);
  for (int i = 0; i < argc; i++) {
    n += strlen(va_arg(v,char*));
  }
  char* p = alloc(n+1);
  n = 0;
  va_start(v,argc);
  for (int i = 0; i < argc; i++) {
    char* str = va_arg(v,char*);
    int len = strlen(str);
    memcpy(p+n,str,len);
    n += len;
  }
  va_end(v);
  p[n] = 0;
  return p;
}

char* realdir(char* path) {
  char s[strlen(path)+1];
  return realpath(dirname(strcpy(s,path)),NULL);
}

char* resolve(char* file) {
  char dir[FILENAME_MAX];
  return concat(3,getcwd(dir,sizeof(dir)),"/",file);
}

#define exists(p) (!access(p,R_OK|X_OK))

char* locate(char* cmd, char* path) {
  char s[strlen(path)+1];
  char* dir = strcpy(s,path);
  char* end = strchr(dir,0);
  char* r = NULL;
  while (dir < end) {
    char* p = strchr(dir,':');
    if (p) *p = 0;
    r = concat(3,dir,"/",cmd);
    if (exists(r)) break;
    free(r);
    if (!p) break;
    dir = p+1;
  }
  return r;
}

int inlist(char* list, char* str) {
  if (list && str) {
    char* p = strstr(list,str);
    if (p) {
      if (p == list || *(p-1) == ':') {
        int n = p[strlen(str)];
        if (n == 0 || n == ':') return YES;
      }
    }
  }
  return NO;
}

int addenv(char* name, char* dir) {
  char* p = getenv(name);
  if (p) {
    if (inlist(p,dir)) {
      return NO;
    }
    char s[strlen(dir)+1+strlen(p)+1];
    strcpy(s,dir);
    strcat(s,":");
    strcat(s,p);
    setenv(name,s,1);
  } else {
    setenv(name,dir,1);
  }
  return YES;
}

void restart(int argc, char* argv[]) {
  char* args[argc+1];
  for (int i = 0; i < argc; i++) {
    args[i] = argv[i];
  }
  args[argc] = NULL;
  say("restarting %s \n",args[0]);
  execv(args[0],args);
  perror("oops!");
}


jint (*_CreateJavaVM)(JavaVM**,void**,void*);

JavaVM *vm;
JNIEnv *env;

int VmArgc;
JavaVMOption *VmArgv;

void *VMLib;

static char libjvm_so[] = "lib/server/libjvm.so";

static char exe_dir[] = "-Dexe.dir=";
static char exe_path[] = "-Dexe.path=";
static char exe_realpath[] = "-Dexe.realpath=";

char* ExeDir;       // -Dexe.dir={dir}
char* ExePath;      // -Dexe.path={path}
char* ExeRealPath;  // -Dexe.realpath={realpath}

char* VMLibPath;    // {jre}/bin/../lib/server/libjvm.so

static char ld_library_path[] = "LD_LIBRARY_PATH";
static char ld_lib[] = "/../lib/";


jsize MainArgc;
char** MainArgv;

static char main_class[] = "exe/Main";
static char main_method[] = "main";
static char main_signature[] = "([Ljava/lang/String;)V";

jclass MainClass;
jmethodID MainMethod;
jobjectArray MainArgs;


void Cleanup() {
  free(VMLibPath);
  free(VmArgv);
  free(ExeRealPath);
  free(ExePath);
  if (ExeDir) free(ExeDir);
  // match all malloc(), strdup(), etc.
}

int SetArg(int index, char* argv) {
  jstring arg = (*env)->NewStringUTF(env,argv);
  if ( arg ) {
    (*env)->SetObjectArrayElement(env,MainArgs,index,arg);
    if ((*env)->ExceptionCheck(env) != JNI_TRUE) {
      return YES;
    }
  }
  return NO;
}

int MakeArgs() {
  jclass String = (*env)->FindClass(env,"java/lang/String");
  if (String) {
    MainArgs = (*env)->NewObjectArray(env,MainArgc,String,0);
    if (MainArgs) {
      for (int i = 0; i < MainArgc; i++) {
        if (!SetArg(i,MainArgv[i])) {
          break;
        }
      }
      return YES;
    }
  }
  say("Out of memory");
  return NO;
}

int FindMainClass() {
  MainClass = (*env)->FindClass(env,main_class);
  if (MainClass) {
    return YES;
  }
  say("Could not find Main-Class: %s",main_class);
  return NO;
}

int FindMain() {
  if (FindMainClass()) {
    MainMethod = (*env)->GetStaticMethodID(env,MainClass,main_method,main_signature);
    if (MainMethod)  {
      return YES;
    }
    say("Could not find %s.main()",main_class);
  }
  return NO;
}

void RunMain() {
  if ( FindMain() && MakeArgs() ) {
    (*env)->CallStaticVoidMethod(env,MainClass,MainMethod,MainArgs);
  }
}

void FreeVMLib() {
  int rc = dlclose(VMLib);
  if (rc) {
    say("%s rc=%d",dlerror(),rc);
  }
}

int GetVMLib() {
  VMLib = dlopen(VMLibPath,RTLD_NOW);
  if (VMLib) {
    return YES;
  }
  say(dlerror());
  return NO;
}

int GetVMApi() {
  dlerror();
  *(void**) (&_CreateJavaVM) = dlsym(VMLib,"JNI_CreateJavaVM");
  char* error = dlerror();
  if (!error) {
    return YES;
  }
  say(error);
  return NO;
}

int CreateVM() {
  JavaVMInitArgs args;
  args.version = JNI_VERSION_10;
  args.ignoreUnrecognized = JNI_TRUE;
  args.nOptions = VmArgc;
  args.options = VmArgv;
  jint rc = (*_CreateJavaVM)(&vm,(void**)&env,&args);
  if (!rc) {
    return YES;
  }
  say("JNI_CreateJavaVM rc=%d",rc);
  return NO;
}

int StartVM() {
  return VmArgv && GetVMLib() && GetVMApi() && CreateVM();
}

void StopVM() {
  if ((*env)->ExceptionOccurred(env)) {
    (*env)->ExceptionDescribe(env);
  }
  (*vm)->DestroyJavaVM(vm);
  FreeVMLib();
}

void SetExeVars(char* cmd) {
  char* p = strchr(cmd,'/');
  if (p) {
    if (p == cmd) {
      ExePath = strdup(cmd);
    } else {
      ExePath = resolve(cmd);
    }
    ExeDir = realdir(ExePath);
  } else {
    ExePath = locate(cmd,getenv("PATH"));
  }
  if (ExePath) {
    ExeRealPath = realpath(ExePath,NULL);
  }
}

int SetLDLibPath() {
  if (ExeDir) {
    char* p = concat(2,ExeDir,ld_lib);
    char* lib = realpath(p,NULL);
    if (lib) {
      int r = addenv(ld_library_path,lib);
      free(lib);
      return r;
    }
    free(p);
  }
  return NO;
}

void SetVMLibPath() {
  char s[strlen(ExeRealPath)+1];
  strcpy(s,ExeRealPath);
  VMLibPath = realpath(concat(3,dirname(s),"/../",libjvm_so),NULL);
}

#define _D(P,S) (S = optionString(P,S))

char* optionString(char* prefix, char* suffix) {
  char* s = concat(2,prefix,suffix);
  free(suffix);
  return s;
}

void SetVMOptions(int argc, char* argv[]) {
  int c = argc + (ExeDir ? 3 : 2);
  JavaVMOption *v = alloc( c * sizeof(*v) );
  for (c = 0; c < argc; c++) {
    v[c].optionString = argv[c];
    v[c].extraInfo = 0;
  }
  v[c].optionString = _D(exe_path,ExePath);
  v[c++].extraInfo = 0;
  v[c].optionString = _D(exe_realpath,ExeRealPath);
  v[c++].extraInfo = 0;
  if (ExeDir) {
    v[c].optionString = _D(exe_dir,ExeDir);
    v[c++].extraInfo = 0;
  }
  VmArgc = c;
  VmArgv = v;
}

void SetMainArgs(int argc, char* argv[]) {
  MainArgc = argc;
  MainArgv = argv;
}

int JvmArg(char* p) {
  return strncmp(p,"-D",2) == 0
      || strncmp(p,"-X",2) == 0
      || strncmp(p,"-verbose",8) == 0
      ?  YES : NO;
}

int SortArgs(int argc, char* argv[]) {
  char* a[argc];
  char* b[argc];
  int j = 0, k = 0;

  for (int i = 0; i < argc; i++) {
    char* arg = argv[i];
    if (JvmArg(arg)) {
      a[j++] = arg;
    } else {
      b[k++] = arg;
    }
  }

  int p = 0;
  for (int i = 0; i < j; i++) {
    argv[p++] = a[i];
  }
  for (int i = 0; i < k; i++) {
    argv[p++] = b[i];
  }

  return j;
}

void ParseArgs(int argc, char* argv[]) {
  SetExeVars(argv[0]);
  if (SetLDLibPath()) {
    restart(argc,argv);
  }
  int cmd = SortArgs(argc,argv);
  SetVMLibPath();
  SetVMOptions(cmd,argv);
  SetMainArgs(argc-cmd,argv+cmd);
}

int main(int argc, char* argv[]) {
  ParseArgs(argc,argv);
  if (StartVM()) {
    RunMain();
    StopVM();
  }
  Cleanup();
  return 0;
}

