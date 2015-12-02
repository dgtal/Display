/*
 * CommBase.cpp
 *
 *  Created on: 2015-11-2
 *      Author: andy
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <android/log.h>
#include <CommBaseClass.h>

#define TAG "CCommBase"

CCommBase::CCommBase()
{
	env = NULL;
	m_comBuff = NULL;
	m_guiCallbackWaitForResponse = NULL;
	m_guiCallbackImmediate = NULL;
}

/**
 *	建立C++和Java之间的通讯
 *	获取Java中的回调函数
 */
bool CCommBase::Connect(JNIEnv* pEnv)
{
	if (NULL == pEnv)
		return false;

	/* 分配内存Java和C++之间的数据交互 */
	if (NULL == m_comBuff)
	{
		m_comBuff = new char[0xFFFF];
		if (NULL == m_comBuff)
			return false;
		memset(m_comBuff, 0, 0xFFFF);
	}

	env = pEnv;
	/* 获取Java回掉函数所在的类 */
	m_clazz = env->FindClass("com/TD/Model/Diagnose");

	/* 获取Java回掉函数guiCallbackImmediate */
	if (NULL == m_guiCallbackImmediate && NULL != m_clazz)
	{
		m_guiCallbackImmediate = env->GetStaticMethodID(m_clazz,
				"guiCallbackImmediate", "([B)Z");
		if (NULL == m_guiCallbackImmediate)
		{
			if (NULL != m_comBuff)
			{
				delete []m_comBuff;
				m_comBuff = NULL;
			}
			return false;
		}
	}

	/* 获取Java回掉函数guiCallbackWaitForResponse */
	if (NULL == m_guiCallbackWaitForResponse && NULL != m_clazz)
	{
		m_guiCallbackWaitForResponse = env->GetStaticMethodID(m_clazz,
				"guiCallbackWaitForResponse", "([B)[B");
		if (NULL == m_guiCallbackWaitForResponse)
		{
			if (NULL != m_comBuff)
			{
				delete[] m_comBuff;
				m_comBuff = NULL;
			}
			return false;
		}
	}
	return true;
}

CCommBase::~CCommBase()
{
	Disconnect();
}

void CCommBase::Disconnect()
{
	if (NULL != m_clazz)
	{
		env->DeleteLocalRef(m_clazz);
		m_clazz = NULL;
	}

	if (NULL != m_comBuff)
	{
		delete []m_comBuff;
		m_comBuff = NULL;
	}
}

/**
 *	向UI发送数据后直接返回
 */
bool CCommBase::WriteForImmediate(const char* buff, int nLen)
{
	if (NULL == m_guiCallbackImmediate && NULL == m_clazz)
		return false;
	/* 将C字符串转换为Java的byte类型数组 */
	jbyte* pBuff = (jbyte*) buff;
	jbyteArray arrBuff = env->NewByteArray(nLen);
	env->SetByteArrayRegion(arrBuff, 0, nLen, pBuff);
	bool bRes = env->CallStaticBooleanMethod(m_clazz, m_guiCallbackImmediate,
			arrBuff);

	/* 释放分配的内存 */
	env->ReleaseByteArrayElements(arrBuff, pBuff, 0);
	env->DeleteLocalRef(arrBuff);
	return bRes;
}

/**
 * 	向UI发送数据而且直到UI有交互结果返回才退出
 */
char* CCommBase::WriteForWait(const char* buff, int nLen, int & outLen)
{
	if (NULL == m_clazz && NULL == m_guiCallbackWaitForResponse)
		return NULL;

	/* 将C字符串转换为Java的byte类型数组 */
	jbyte* pBuff = (jbyte*) buff;
	jbyteArray arrBuff = env->NewByteArray(nLen);

	env->SetByteArrayRegion(arrBuff, 0, nLen, pBuff);

	/* 调用Java函数:将数据发送给UI,并等待UI的操作结果 */
	jbyteArray objRes = (jbyteArray) env->CallStaticObjectMethod(m_clazz,
			m_guiCallbackWaitForResponse, arrBuff);

	if (NULL == objRes)
	{
		/* 释放分配的内存 */
		env->ReleaseByteArrayElements(arrBuff, pBuff, 0);
		env->DeleteLocalRef(arrBuff);
		arrBuff = NULL;
		return NULL;
	}

	/* 将UI返回的操作结果从Java byte[]转换成C语言的字符指针 */
	outLen = env->GetArrayLength(objRes);
	memset(m_comBuff, 0, 0xFFFF);
	jbyte* pRes =  env->GetByteArrayElements(objRes, 0);
	memcpy(m_comBuff, (char*)pRes, outLen);
	env->ReleaseByteArrayElements(objRes, pRes, JNI_COMMIT);
	env->DeleteLocalRef(objRes);
	objRes = NULL;
	return m_comBuff;
}

void CCommBase::LogExceptionFromJava()
{
	if (env->ExceptionOccurred() != NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
}

void CCommBase::ThrowExceptionToJava(string strNote)
{
	jclass clazzException = env->FindClass("java/lang/RuntimeException");
	env->ThrowNew(clazzException, strNote.c_str());
}

/*jstring CCommBase::str2jstring(const char* pBuff)
{
	//获取Java String类
	jclass clazz = env->FindClass("java/lang/String");

	//获取Java String类的构造函数String(byte[], String),用于数据转换
	jmethodID ctorID = env->GetMethodID(clazz, "<init>",
			"([BLjava/lang/String;)V");

	//建立byte数组
	jbyteArray arrByte = env->NewByteArray(strlen(pBuff));

	//将char*转换为byte数组
	env->SetByteArrayRegion(arrByte, 0, strlen(pBuff), (jbyte*) pBuff);

	//编码方式，用于将byte转换为String
	jstring encoding = env->NewStringUTF("GB2312");

	return (jstring) env->NewObject(clazz, ctorID, arrByte, encoding);
}

string CCommBase::jstring2str(jstring jstr)
{
	char* pRes = NULL;
	jclass clazz = env->FindClass("java/lang/String");
	jstring encoding = env->NewStringUTF("GB2312");
	jmethodID ctorID = env->GetMethodID(clazz, "getBytes",
			"(Ljava/lang/String;)[B");
	jbyteArray arrByte = (jbyteArray) env->CallObjectMethod(jstr, ctorID,
			encoding);
	jsize len = env->GetArrayLength(arrByte);
	jbyte* bytes = env->GetByteArrayElements(arrByte, JNI_FALSE);
	if (len > 0)
	{
		pRes = (char*) malloc(len + 1);
		memset(pRes, 0, len + 1);
		memcpy(pRes, bytes, len);
		pRes[len] = 0;
	}
	env->ReleaseByteArrayElements(arrByte, bytes, 0);
	string stemp(pRes);
	free(pRes);
	pRes = NULL;
	return stemp;
}*/
