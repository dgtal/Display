/*
 * CommBaseClass.h
 *
 *  Created on: 2015-11-2
 *      Author: andy
 */

#ifndef COMMBASECLASS_H_
#define COMMBASECLASS_H_

#include <jni.h>
#include <string>
using namespace std;

class CCommBase
{
public:
	CCommBase();
	~CCommBase();
	bool Connect(JNIEnv* env);
	void Disconnect();
	bool Read(char* buff, int nSize);
	char* WriteForWait(const char* buff, int nLen, int & outLen);
	bool WriteForImmediate(const char* buff, int nLen);

private:
	void LogExceptionFromJava();
	void ThrowExceptionToJava(string strNote);

private:
	/*jstring str2jstring(const char*);
	string jstring2str(jstring);*/
private:
	jmethodID m_guiCallbackWaitForResponse;
	jmethodID m_guiCallbackImmediate;
	jclass m_clazz;
	JNIEnv* env;
	jbyteArray m_byteData;
	char* m_comBuff;
};

#endif /* COMMBASECLASS_H_ */
