#ifndef BDIANA_CAPI_H_
#define BDIANA_CAPI_H_


#ifdef __cplusplus
extern "C" {
#endif


#define DIANA_OK 1
#define DIANA_ERROR 99

extern int diana_init(int argc, char** argv);
extern int diana_dealloc();
extern int diana_readSetupFile(const char* setupFilename);
extern int diana_parseAndProcessString(const char* str);


#ifdef __cplusplus
}
#endif


#endif /*BDIANA_CAPI_H_*/

