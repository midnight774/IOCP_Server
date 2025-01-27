#pragma once

#define SAFE_DELETE(p)  if(p)   { delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p)  if(p)   { delete[] p; p = nullptr; }
#define SAFE_RELEASE(p)  if(p)   { p->Release(); p = nullptr; }

#define DECLARE_SINGLETON(Type)    \
private:\
    static Type*    m_pInst;\
public:\
    static Type* GetInst()\
    {\
        if (!m_pInst)\
            m_pInst = new Type;\
        return m_pInst;\
    }\
    static void DestroySingleInst()\
    {\
        SAFE_DELETE(m_pInst);\
    }\
private:\
    Type();\
    ~Type();

#define DEFINITION_SINGLETON(Type) Type* Type::m_pInst = nullptr;
